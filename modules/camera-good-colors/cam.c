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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cam.h"
#include "py/mpprint.h"
#include "picampinos.pio.h"
#include "ov5640.h"

// ============================================================================
// INLINE PIO program: packs TWO pixels per 32-bit FIFO push.
// Defined here in cam.c to bypass any pioasm/.pio.h include path issues.
//
// ISR left-shift, 4x "in pins,8" per push:
//   ISR = HH1_LL1_HH2_LL2  (two RGB565 pixels in one 32-bit word)
// In LE memory: [LL2, HH2, LL1, HH1]
//   → word[0] (bytes 0,1) = pixel_N+1, word[1] (bytes 2,3) = pixel_N
// JS must swap each pixel pair to correct display order.
//
// X register counts pixel PAIRS (total_pixels / 2 - 1)
// Y register stores the pair count for reload each frame
// ============================================================================
static const uint16_t cam_pio_packed_instructions[] = {
    0x6020, //  0: out    x, 32           ; X = 0 (reserved)
    0x6040, //  1: out    y, 32           ; Y = pixel_pairs count
    0x2028, //  2: wait   0 pin, 8        ; wait VSYNC=0
    0x20a8, //  3: wait   1 pin, 8        ; wait VSYNC=1 (frame start)
    0xa022, //  4: mov    x, y            ; x = pixel_pairs
    0x2029, //  5: wait   0 pin, 9        ; wait HREF=0 (line sync)
    0x20a9, //  6: wait   1 pin, 9        ; wait HREF=1 (line active)
    // --- Pixel N (first of pair) ---
    0x20aa, //  7: wait   1 pin, 10       ; PCLK=1
    0x4008, //  8: in     pins, 8         ; HH1 → ISR
    0x202a, //  9: wait   0 pin, 10       ; PCLK=0
    0x20aa, // 10: wait   1 pin, 10       ; PCLK=1
    0x4008, // 11: in     pins, 8         ; LL1 → ISR
    0x202a, // 12: wait   0 pin, 10       ; PCLK=0
    // --- Pixel N+1 (second of pair) ---
    0x20aa, // 13: wait   1 pin, 10       ; PCLK=1
    0x4008, // 14: in     pins, 8         ; HH2 → ISR
    0x202a, // 15: wait   0 pin, 10       ; PCLK=0
    0x20aa, // 16: wait   1 pin, 10       ; PCLK=1
    0x4008, // 17: in     pins, 8         ; LL2 → ISR (32 bits full)
    0x202a, // 18: wait   0 pin, 10       ; PCLK=0
    // --- Push and loop ---
    0xa042, // 19: nop (mov y,y)          ; autopush already fired at 4th in
    0x0046, // 20: jmp    x--, 6          ; loop pixel pairs
    0x2029, // 21: wait   0 pin, 9        ; wait HREF=0 (end of line)
    0x0004, // 22: jmp    4               ; next frame (reload x)
    0x0002, // 23: jmp    2               ; wait next VSYNC
};

#define CAM_PIO_PACKED_LEN 24
#define CAM_PIO_PACKED_WRAP_TARGET 0
#define CAM_PIO_PACKED_WRAP 23

static const struct pio_program cam_pio_packed_program = {
    .instructions = cam_pio_packed_instructions,
    .length = CAM_PIO_PACKED_LEN,
    .origin = -1,
    .pio_version = 1,
#if PICO_PIO_VERSION > 0
    .used_gpio_ranges = 0x0
#endif
};

// Custom init that uses OUR wrap values (not the .pio.h ones)
static inline void cam_pio_packed_init(PIO pio, uint32_t sm, uint32_t offset,
                                        uint32_t in_base, uint32_t in_pin_num)
{
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + CAM_PIO_PACKED_WRAP_TARGET,
                           offset + CAM_PIO_PACKED_WRAP);
    sm_config_set_set_pins(&c, in_base, in_pin_num);
    sm_config_set_in_pins(&c, in_base);
    sm_config_set_in_shift(&c, false, true, 31);   // left shift, autopush at 31 bits (avoids 32→0 encoding bug on RP2350)
    sm_config_set_out_shift(&c, false, true, 32);   // left shift, auto-pull
    for (uint32_t i = 0; i < in_pin_num; i++) {
        pio_gpio_init(pio, in_base + i);
    }
    pio_sm_set_consecutive_pindirs(pio, sm, in_base, in_pin_num, false);
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

// Double buffering: two separate buffers for tear-free capture
// MUST be 4-byte aligned for DMA_SIZE_32 transfers (RP2350 requires natural alignment)
static uint8_t cam_buffer_a[CAM_FUL_SIZE * 2] __attribute__((aligned(4)));
static uint8_t cam_buffer_b[CAM_FUL_SIZE * 2] __attribute__((aligned(4)));

// Pointers for double buffering
uint8_t *cam_dma_write_buf = cam_buffer_a;    // DMA writes to this buffer
uint8_t *cam_python_read_buf = cam_buffer_b;  // Python reads from this buffer (safe)

// Legacy pointer for compatibility - points to read buffer
uint8_t *cam_ptr = cam_buffer_b;

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
function:   Camera initialization
parameter:
********************************************************************************/
void init_cam()
{
    set_pwm_freq_kHz(37000, pin_xclk_pwm);
    sleep_ms(50);
    sccb_init(pin_i2c1_sda, pin_i2c1_scl);
    sleep_ms(50);
}

void init_cam_with_registers(const uint16_t custom_regs[][2], uint16_t num_regs)
{
    set_pwm_freq_kHz(37000, pin_xclk_pwm);
    sleep_ms(50);
    sccb_init_with_registers(pin_i2c1_sda, pin_i2c1_scl, custom_regs, num_regs);
    sleep_ms(50);
}



void setup_dma_for_capture()
{
    DMA_CAM_RD_CH = dma_claim_unused_channel(true);
    irq_set_enabled(DMA_IRQ_0, false);

    dma_channel_config c0 = get_cam_config(pio_cam, sm_cam, DMA_CAM_RD_CH);
    dma_channel_configure(DMA_CAM_RD_CH, &c0,
                          cam_dma_write_buf,
                          &pio_cam->rxf[sm_cam],
                          CAM_FUL_SIZE / 2,
                          false);

    dma_channel_set_irq0_enabled(DMA_CAM_RD_CH, true);
    irq_add_shared_handler(DMA_IRQ_0, cam_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    irq_set_enabled(DMA_IRQ_0, true);
    dma_channel_start(DMA_CAM_RD_CH);
}

/********************************************************************************
function:   DMA interrupt processing function
            Implements double buffering with read protection
parameter:
********************************************************************************/
void cam_handler(void)
{
    // Clear the interrupt flag first
    dma_hw->ints0 = 1u << DMA_CAM_RD_CH;

    if (!read_in_progress) {
        // Safe to swap buffers - Python is not currently reading
        uint8_t *temp = cam_dma_write_buf;
        cam_dma_write_buf = cam_python_read_buf;
        cam_python_read_buf = temp;

        // Update legacy pointer for compatibility
        cam_ptr = cam_python_read_buf;

        // Signal that a new frame is ready
        buffer_ready = true;
    }
    // else: Python is reading, don't swap - drop this frame to protect read buffer

    // Restart DMA to write buffer (either swapped or same if read in progress)
    dma_channel_set_write_addr(DMA_CAM_RD_CH, cam_dma_write_buf, true);
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
    read_in_progress = false;
}

uint8_t* cam_get_read_buffer(void)
{
    return cam_python_read_buf;
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
    uint32_t cam_num_pins = 11;
    uint32_t pixel_pairs = CAM_FUL_SIZE / 2;

    uint32_t offset_cam = pio_add_program(pio_cam, &cam_pio_packed_program);
    cam_pio_packed_init(pio_cam, sm_cam, offset_cam, cam_base_pin, cam_num_pins);
    pio_sm_put_blocking(pio_cam, sm_cam, 0);
    pio_sm_put_blocking(pio_cam, sm_cam, (pixel_pairs - 1));

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
    irq_set_enabled(DMA_IRQ_0, false);
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
