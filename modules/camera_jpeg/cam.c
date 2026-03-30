/*****************************************************************************
* | File      	:   cam.c
* | Author      :   Waveshare team
* | Function    :   CAM function interface
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2025-03-13
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of theex Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
******************************************************************************/

#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cam.h"
#include "py/mpprint.h"
#include "picampinos.pio.h"
#include "ov5640.h"

// ============================================================================
// INLINE PIO program for JPEG capture with manual push.
//
// HREF-gated capture: PCLK is ungated (0x4740=0x21), so PIO must check HREF
// to avoid sampling garbage during HREF-low gaps. The jmp_pin loop captures
// 4 bytes per word while HREF stays high, then wraps to wait for next HREF.
//
// ISR RIGHT-shift, NO autopush (RP2350 errata RP2350-E6).
// Right-shift ensures correct byte order in little-endian ARM memory:
//   captured bytes B0,B1,B2,B3 → memory addr+0=B0, addr+1=B1, addr+2=B2, addr+3=B3
//
// Manual push every 4 bytes into one 32-bit word.
// Frame boundaries detected by VSYNC GPIO interrupt on CPU.
// ============================================================================
static const uint16_t cam_pio_packed_instructions[] = {
    //     .wrap_target
    0x20a9, //  0: wait   1 pin, 9        ; wait HREF high
    0x202a, //  1: wait   0 pin, 10       ; sync: ensure PCLK is low before first sample
    0x20aa, //  2: wait   1 pin, 10       ; PCLK high - byte 1
    0x4008, //  3: in     pins, 8
    0x202a, //  4: wait   0 pin, 10
    0x20aa, //  5: wait   1 pin, 10       ; PCLK high - byte 2
    0x4008, //  6: in     pins, 8
    0x202a, //  7: wait   0 pin, 10
    0x20aa, //  8: wait   1 pin, 10       ; PCLK high - byte 3
    0x4008, //  9: in     pins, 8
    0x202a, // 10: wait   0 pin, 10
    0x20aa, // 11: wait   1 pin, 10       ; PCLK high - byte 4
    0x4008, // 12: in     pins, 8
    0x202a, // 13: wait   0 pin, 10
    0x8020, // 14: push   noblock
    0x00c2, // 15: jmp    pin, 2          ; loop while HREF high (skip sync on continuation)
    //     .wrap
};

#define CAM_PIO_PACKED_LEN 16
#define CAM_PIO_PACKED_WRAP_TARGET 0
#define CAM_PIO_PACKED_WRAP 15

static const struct pio_program cam_pio_packed_program = {
    .instructions = cam_pio_packed_instructions,
    .length = CAM_PIO_PACKED_LEN,
    .origin = -1,
    .pio_version = 1,
#if PICO_PIO_VERSION > 0
    .used_gpio_ranges = 0x0
#endif
};

// Custom init for JPEG capture PIO program (HREF-gated via jmp_pin)
static inline void cam_pio_packed_init(PIO pio, uint32_t sm, uint32_t offset,
                                        uint32_t in_base, uint32_t in_pin_num,
                                        uint32_t href_pin, uint32_t pclk_pin)
{
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + CAM_PIO_PACKED_WRAP_TARGET,
                           offset + CAM_PIO_PACKED_WRAP);
    sm_config_set_set_pins(&c, in_base, in_pin_num);
    sm_config_set_in_pins(&c, in_base);
    sm_config_set_jmp_pin(&c, href_pin);
    sm_config_set_in_shift(&c, true, false, 32);    // RIGHT shift, autopush DISABLED (manual push)
    for (uint32_t i = 0; i < in_pin_num; i++) {
        pio_gpio_init(pio, in_base + i);
    }
    pio_gpio_init(pio, href_pin);
    pio_gpio_init(pio, pclk_pin);
    pio_sm_set_consecutive_pindirs(pio, sm, in_base, in_pin_num, false);
    pio_sm_set_consecutive_pindirs(pio, sm, href_pin, 1, false);
    pio_sm_set_consecutive_pindirs(pio, sm, pclk_pin, 1, false);
    sm_config_set_clkdiv(&c, 1);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}
// ============================================================================

// init PIO
static PIO pio_cam = pio0;

// statemachine's pointer
static uint32_t sm_cam; // CAMERA's state machines

// dma channels
static uint32_t DMA_CAM_RD_CH;
static bool dma_irq_handler_installed = false;
static bool vsync_raw_irq_handler_installed = false;

// Double buffering: two separate JPEG buffers
// MUST be 4-byte aligned for DMA_SIZE_32 transfers (RP2350 requires natural alignment)
static uint8_t cam_buffer_a[JPEG_BUF_SIZE] __attribute__((aligned(4)));
static uint8_t cam_buffer_b[JPEG_BUF_SIZE] __attribute__((aligned(4)));

// Pointers for double buffering
uint8_t *cam_dma_write_buf = cam_buffer_a;    // DMA writes to this buffer
uint8_t *cam_python_read_buf = cam_buffer_b;  // Python reads from this buffer (safe)

// Legacy pointer for compatibility - points to read buffer
uint8_t *cam_ptr = cam_buffer_b;

// JPEG frame size tracking
static uint32_t frame_size_a = 0;
static uint32_t frame_size_b = 0;
volatile uint32_t jpeg_frame_size = 0;    // Readable frame's size
volatile uint32_t jpeg_write_size = 0;     // Write buffer's size (in progress)

// VSYNC + capture diagnostics (read via Python wrappers)
static volatile bool vsync_end_on_rising = true;  // JPEG mode needs rising edge for frame boundaries
static volatile uint32_t vsync_rise_count = 0;
static volatile uint32_t vsync_fall_count = 0;
static volatile uint32_t vsync_frame_end_count = 0;
static volatile int32_t jpeg_last_soi_pos = -1;
static volatile int32_t jpeg_last_eoi_pos = -1;
static volatile uint32_t jpeg_last_sample_nonzero = 0;
static volatile uint32_t jpeg_last_sample_ff = 0;
static volatile uint32_t jpeg_last_sample_len = 0;
static uint8_t cam_capture_head[32]; // first 32 bytes of last capture

uint8_t pin_i2c1_sda = 22; // default on RP2350 touch 2in
uint8_t pin_i2c1_scl = 23; // default on RP2350 touch 2in
uint8_t pin_xclk_pwm = 11; // GPIO11 (camera's xclk(24MHz))


// flags
volatile bool buffer_ready = false;
volatile bool read_in_progress = false;  // Set by Python to protect read buffer




void set_i2c_pins(uint8_t sda, uint8_t scl) {
    pin_i2c1_sda = sda;
    pin_i2c1_scl = scl;
}

void set_pwm_pin(uint8_t pwm) {
    pin_xclk_pwm = pwm;
}

/********************************************************************************
function:   Shared frame finalization path
            Aborts DMA, swaps buffers, restarts capture. Minimal ISR work.
********************************************************************************/
static void finalize_capture_and_restart(void) {
    // ULTRA-FAST: abort → swap → restart FIRST, then analyze.
    // This ensures PIO/DMA restart within microseconds to catch next frame's SOI.

    // 1. Abort DMA and stop PIO
    dma_channel_abort(DMA_CAM_RD_CH);
    pio_sm_set_enabled(pio_cam, sm_cam, false);

    // 2. Record captured size
    uint32_t dma_remaining = dma_channel_hw_addr(DMA_CAM_RD_CH)->transfer_count;
    uint32_t write_offset = (JPEG_BUF_SIZE / 4 - dma_remaining) * 4;

    // 3. Swap buffers IMMEDIATELY (before any analysis)
    uint8_t *captured_buf = cam_dma_write_buf;  // save pointer to just-captured data
    if (write_offset > 0 && !read_in_progress) {
        if (cam_dma_write_buf == cam_buffer_a) {
            frame_size_a = write_offset;
        } else {
            frame_size_b = write_offset;
        }
        uint8_t *temp = cam_dma_write_buf;
        cam_dma_write_buf = cam_python_read_buf;
        cam_python_read_buf = temp;
        cam_ptr = cam_python_read_buf;
        jpeg_frame_size = write_offset;
        buffer_ready = true;
    }

    // 4. Restart PIO + DMA into new write buffer IMMEDIATELY
    pio_sm_clear_fifos(pio_cam, sm_cam);
    pio_sm_exec(pio_cam, sm_cam, pio_encode_mov(pio_isr, pio_null));
    dma_channel_set_write_addr(DMA_CAM_RD_CH, cam_dma_write_buf, false);
    dma_channel_set_trans_count(DMA_CAM_RD_CH, JPEG_BUF_SIZE / 4, true);
    pio_sm_restart(pio_cam, sm_cam);
    pio_sm_set_enabled(pio_cam, sm_cam, true);

    // 5. NOW analyze the captured buffer (safe — it's the read buffer now)
    jpeg_write_size = write_offset;
    uint32_t head_len = (write_offset < 32) ? write_offset : 32;
    memcpy(cam_capture_head, captured_buf, head_len);

    uint32_t sample_len = (write_offset < 4096) ? write_offset : 4096;
    jpeg_last_sample_len = sample_len;
    uint32_t ff_count = 0, nz_count = 0;
    for (uint32_t i = 0; i < sample_len; i++) {
        if (captured_buf[i] != 0) nz_count++;
        if (captured_buf[i] == 0xFF) ff_count++;
    }
    jpeg_last_sample_nonzero = nz_count;
    jpeg_last_sample_ff = ff_count;

    jpeg_last_soi_pos = -1;
    jpeg_last_eoi_pos = -1;
    for (uint32_t i = 0; i + 1 < write_offset; i++) {
        if (captured_buf[i] == 0xFF) {
            if (captured_buf[i+1] == 0xD8 && jpeg_last_soi_pos == -1) {
                jpeg_last_soi_pos = (int32_t)i;
            }
            if (captured_buf[i+1] == 0xD9) {
                jpeg_last_eoi_pos = (int32_t)i;
            }
        }
    }
}

/********************************************************************************
function:   VSYNC GPIO interrupt handler
            Uses selected VSYNC edge as frame end.
********************************************************************************/
static void vsync_handler(uint gpio, uint32_t events) {
    (void)gpio;

    if (events & GPIO_IRQ_EDGE_RISE) {
        vsync_rise_count++;
    }
    if (events & GPIO_IRQ_EDGE_FALL) {
        vsync_fall_count++;
    }

    const bool selected_edge =
        (vsync_end_on_rising && (events & GPIO_IRQ_EDGE_RISE)) ||
        (!vsync_end_on_rising && (events & GPIO_IRQ_EDGE_FALL));
    if (!selected_edge) {
        return;
    }

    vsync_frame_end_count++;
    finalize_capture_and_restart();
}

// Raw GPIO IRQ handler for VSYNC edge detection. This avoids dependence on the
// single callback slot used by gpio_set_irq_enabled_with_callback().
static void vsync_raw_irq_handler(void)
{
    uint32_t events = gpio_get_irq_event_mask(g_cam_pinmap.vsync);
    uint32_t edge_events = events & (GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL);
    if (edge_events == 0) {
        return;
    }
    gpio_acknowledge_irq(g_cam_pinmap.vsync, edge_events);
    vsync_handler(g_cam_pinmap.vsync, edge_events);
}

/********************************************************************************
function:   DMA fallback frame-end handler
            Used when VSYNC interrupt path is not firing.
********************************************************************************/
static void dma_frame_end_handler(void)
{
    uint32_t mask = (1u << DMA_CAM_RD_CH);
    if ((dma_hw->ints0 & mask) == 0) {
        return;
    }

    // Clear DMA IRQ first
    dma_hw->ints0 = mask;

    // Count as frame-end event (source is DMA fallback, not VSYNC)
    vsync_frame_end_count++;
    finalize_capture_and_restart();
}

/********************************************************************************
function:   Camera initialization
parameter:
********************************************************************************/
void init_cam()
{
    printf("cam: JPEG buffer size=%d bytes x2\n", JPEG_BUF_SIZE);
    set_pwm_freq_kHz(37000, pin_xclk_pwm);
    sleep_ms(50);
    sccb_init(pin_i2c1_sda, pin_i2c1_scl);
    sleep_ms(50);
}

void init_cam_with_registers(const uint16_t custom_regs[][2], uint16_t num_regs)
{
    printf("cam: JPEG buffer size=%d bytes x2\n", JPEG_BUF_SIZE);
    set_pwm_freq_kHz(37000, pin_xclk_pwm);
    sleep_ms(50);
    sccb_init_with_registers(pin_i2c1_sda, pin_i2c1_scl, custom_regs, num_regs);
    sleep_ms(50);
}



void setup_dma_for_capture()
{
    DMA_CAM_RD_CH = dma_claim_unused_channel(true);

    dma_channel_config c0 = get_cam_config(pio_cam, sm_cam, DMA_CAM_RD_CH);
    dma_channel_configure(DMA_CAM_RD_CH, &c0,
                          cam_dma_write_buf,
                          &pio_cam->rxf[sm_cam],
                          JPEG_BUF_SIZE / 4,
                          false);

    // Set up VSYNC GPIO interrupt instead of DMA completion interrupt.
    // Use a raw handler so this module remains independent of shared callback slots.
    if (!vsync_raw_irq_handler_installed) {
        gpio_add_raw_irq_handler(g_cam_pinmap.vsync, vsync_raw_irq_handler);
        vsync_raw_irq_handler_installed = true;
    }
    gpio_set_irq_enabled(g_cam_pinmap.vsync, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    irq_set_enabled(IO_IRQ_BANK0, true);

    // DMA completion fallback: if VSYNC edges are missing, finalize frame on full buffer.
    dma_channel_set_irq0_enabled(DMA_CAM_RD_CH, true);
    dma_hw->ints0 = (1u << DMA_CAM_RD_CH);  // clear stale pending flag
    if (!dma_irq_handler_installed) {
        irq_add_shared_handler(DMA_IRQ_0, dma_frame_end_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
        irq_set_enabled(DMA_IRQ_0, true);
        dma_irq_handler_installed = true;
    }

    dma_channel_start(DMA_CAM_RD_CH);
}

/********************************************************************************
function:   Double buffer control functions for Python
********************************************************************************/
void cam_start_read(void)
{
    read_in_progress = true;
}

void cam_end_read(void)
{
    buffer_ready = false;       // Must clear BEFORE releasing lock to avoid race with VSYNC ISR
    read_in_progress = false;
}

uint8_t* cam_get_read_buffer(void)
{
    return cam_python_read_buf;
}

uint32_t cam_get_frame_size(void)
{
    return jpeg_frame_size;
}

void cam_set_vsync_end_on_rising(bool enabled)
{
    vsync_end_on_rising = enabled;
}

bool cam_get_vsync_end_on_rising(void)
{
    return vsync_end_on_rising;
}

void cam_reset_diag_stats(void)
{
    vsync_rise_count = 0;
    vsync_fall_count = 0;
    vsync_frame_end_count = 0;
    jpeg_write_size = 0;
    jpeg_last_soi_pos = -1;
    jpeg_last_eoi_pos = -1;
    jpeg_last_sample_nonzero = 0;
    jpeg_last_sample_ff = 0;
    jpeg_last_sample_len = 0;
}

uint32_t cam_get_vsync_rise_count(void)
{
    return vsync_rise_count;
}

uint32_t cam_get_vsync_fall_count(void)
{
    return vsync_fall_count;
}

uint32_t cam_get_vsync_frame_end_count(void)
{
    return vsync_frame_end_count;
}

uint32_t cam_get_last_capture_size(void)
{
    return jpeg_write_size;
}

int32_t cam_get_last_soi_pos(void)
{
    return jpeg_last_soi_pos;
}

int32_t cam_get_last_eoi_pos(void)
{
    return jpeg_last_eoi_pos;
}

uint32_t cam_get_last_sample_nonzero(void)
{
    return jpeg_last_sample_nonzero;
}

uint32_t cam_get_last_sample_ff(void)
{
    return jpeg_last_sample_ff;
}

uint32_t cam_get_last_sample_len(void)
{
    return jpeg_last_sample_len;
}

const uint8_t* cam_get_capture_head(void)
{
    return cam_capture_head;
}



// BIG HEAD
// D0    GPIO 0
// D1    GPIO 1
// D2    GPIO 2
// D3    GPIO 3
// D4    GPIO 4
// D5    GPIO 5
// D6    GPIO 6
// D7    GPIO 7
// VSYNC GPIO 8
// HREF  GPIO 9
// PCLK  GPIO 10
// XCLK  GPIO 11
// PWDN  GPIO 14
// SDA   GPIO 22
// SCL   GPIO 23



// Global pinmap instance, initialized to default (old hardware)
cam_pinmap_t g_cam_pinmap = {
    .d = {0, 1, 2, 3, 4, 5, 6, 7},
    .vsync = 8,
    .href = 9,
    .pclk = 10,
    .xclk = 11
};

void cam_set_pinmap(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7, uint8_t vsync, uint8_t href, uint8_t pclk, uint8_t xclk) {
    g_cam_pinmap.d[0] = d0;
    g_cam_pinmap.d[1] = d1;
    g_cam_pinmap.d[2] = d2;
    g_cam_pinmap.d[3] = d3;
    g_cam_pinmap.d[4] = d4;
    g_cam_pinmap.d[5] = d5;
    g_cam_pinmap.d[6] = d6;
    g_cam_pinmap.d[7] = d7;
    g_cam_pinmap.vsync = vsync;
    g_cam_pinmap.href = href;
    g_cam_pinmap.pclk = pclk;
    g_cam_pinmap.xclk = xclk;
}

/********************************************************************************
function:   Start the camera
parameter:
********************************************************************************/
void start_cam()
{
    uint32_t cam_base_pin = g_cam_pinmap.d[0];
    uint32_t cam_num_pins = 8;

    uint32_t offset_cam = pio_add_program(pio_cam, &cam_pio_packed_program);
    cam_pio_packed_init(pio_cam, sm_cam, offset_cam, cam_base_pin, cam_num_pins, g_cam_pinmap.href, g_cam_pinmap.pclk);

    // Keep VSYNC on SIO for reliable GPIO edge detection and diagnostics.
    gpio_init(g_cam_pinmap.vsync);
    gpio_set_dir(g_cam_pinmap.vsync, GPIO_IN);
    gpio_disable_pulls(g_cam_pinmap.vsync);

    // No TX FIFO writes needed — new PIO program doesn't use OUT/TX

    setup_dma_for_capture();
}

/********************************************************************************
function:   Reading camera data
parameter:
********************************************************************************/
void read_cam_data_blocking(uint8_t *buffer, size_t length)
{
    size_t index = 0;
    while (index < length)
    {
        if (!pio_sm_is_rx_fifo_empty(pio_cam, sm_cam))
        {
            uint16_t dat = pio_sm_get(pio_cam, sm_cam);
            buffer[index++] = (dat >> 8) & 0xFF;
            buffer[index++] = dat & 0xFF;
        }
        else
        {
            tight_loop_contents();
        }
    }
}

/********************************************************************************
function:   Release the camera
parameter:
********************************************************************************/
void free_cam()
{
    // Disable VSYNC GPIO interrupt
    gpio_set_irq_enabled(g_cam_pinmap.vsync, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
    if (vsync_raw_irq_handler_installed) {
        gpio_remove_raw_irq_handler(g_cam_pinmap.vsync, vsync_raw_irq_handler);
        vsync_raw_irq_handler_installed = false;
    }
    dma_channel_set_irq0_enabled(DMA_CAM_RD_CH, false);
    dma_channel_abort(DMA_CAM_RD_CH);
}

/********************************************************************************
function:   Camera dma config
parameter:
********************************************************************************/
dma_channel_config get_cam_config(PIO pio, uint32_t sm, uint32_t dma_chan)
{
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));
    return c;
}

/********************************************************************************
function:   Configure PWM to provide clock for the camera
parameter:
********************************************************************************/
void set_pwm_freq_kHz(uint32_t freq_khz, uint8_t gpio_num)
{
    uint32_t pwm0_slice_num;
    uint32_t period;
    static pwm_config pwm_slice_config;
    uint32_t system_clk_khz = clock_get_hz(clk_sys) / 1000;
    period = system_clk_khz / freq_khz - 1;
    if (period < 1)
        period = 1;

    gpio_set_function(gpio_num, GPIO_FUNC_PWM);
    pwm0_slice_num = pwm_gpio_to_slice_num(gpio_num);
    pwm_slice_config = pwm_get_default_config();
    pwm_config_set_wrap(&pwm_slice_config, period);
    pwm_config_set_clkdiv(&pwm_slice_config, 1);
    pwm_init(pwm0_slice_num, &pwm_slice_config, true);
    pwm_set_gpio_level(gpio_num, (pwm_slice_config.top * 0.50));
}
