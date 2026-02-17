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

#include <stdint.h>
#include <stdbool.h>

// init PIO
static PIO pio_cam = pio0;

// statemachine's pointer
static uint32_t sm_cam; // CAMERA's state machines

// 3 half-frame buckets (76,800 bytes each = 230,400 total)
static uint8_t bucket_mem_0[HALF_FRAME_BYTES] __attribute__((aligned(4)));
static uint8_t bucket_mem_1[HALF_FRAME_BYTES] __attribute__((aligned(4)));
static uint8_t bucket_mem_2[HALF_FRAME_BYTES] __attribute__((aligned(4)));
uint8_t *bucket[3] = { bucket_mem_0, bucket_mem_1, bucket_mem_2 };

// DMA channels for half-frame chaining
static uint32_t DMA_CH_A;
static uint32_t DMA_CH_B;

// Rotation counter: increments every half-frame completion
static volatile uint32_t write_pos = 0;

// Frame state
volatile bool frame_ready = false;
volatile uint8_t frame_first_idx = 0;
volatile uint8_t frame_second_idx = 1;
volatile bool read_in_progress = false;

uint8_t pin_i2c1_sda = 22; // default on RP2350 touch 2in
uint8_t pin_i2c1_scl = 23; // default on RP2350 touch 2in
uint8_t pin_xclk_pwm = 11; // GPIO11 (camera's xclk(24MHz))






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

    mp_printf(MP_PYTHON_PRINTER, "initialize CAMERA\n");
    mp_printf(MP_PYTHON_PRINTER, "set camera XCLK (pwm) pin: %d\n", pin_xclk_pwm);
    mp_printf(MP_PYTHON_PRINTER, "call set_pwm_freq_kHz(pwm) before init_cam() to change it\n");
    set_pwm_freq_kHz(37000, pin_xclk_pwm); // XCLK
    sleep_ms(50);
    mp_printf(MP_PYTHON_PRINTER, "set camera I2C pins: SDA: %d, SCL: %d\n", (int)pin_i2c1_sda, (int)pin_i2c1_scl);
    mp_printf(MP_PYTHON_PRINTER, "call set_i2c_pins(sda,scl) before init_cam() to change it\n");

    sccb_init(pin_i2c1_sda, pin_i2c1_scl); // sda,scl=(gp26,gp27). see 'sccb_if.c' and 'cam.h'
    sleep_ms(50);

}



void setup_dma_for_capture()
{
    // Claim two DMA channels for half-frame chaining
    DMA_CH_A = dma_claim_unused_channel(true);
    DMA_CH_B = dma_claim_unused_channel(true);
    mp_printf(MP_PYTHON_PRINTER, "setup_dma_for_capture()-> DMA_CH_A=%d DMA_CH_B=%d\n",
              (int)DMA_CH_A, (int)DMA_CH_B);

    irq_set_enabled(DMA_IRQ_0, false);

    // Configure CH_A: transfers one half-frame, then chains to CH_B
    dma_channel_config c_a = get_cam_config(pio_cam, sm_cam, DMA_CH_A);
    channel_config_set_transfer_data_size(&c_a, DMA_SIZE_16);
    channel_config_set_chain_to(&c_a, DMA_CH_B);
    dma_channel_configure(DMA_CH_A, &c_a,
                          bucket[0],              // write to bucket[0]
                          &pio_cam->rxf[sm_cam],  // read from PIO RX FIFO
                          HALF_FRAME_XFERS,       // 38,400 x 16-bit transfers
                          false);                 // don't start yet

    // Configure CH_B: transfers one half-frame, then chains to CH_A
    dma_channel_config c_b = get_cam_config(pio_cam, sm_cam, DMA_CH_B);
    channel_config_set_transfer_data_size(&c_b, DMA_SIZE_16);
    channel_config_set_chain_to(&c_b, DMA_CH_A);
    dma_channel_configure(DMA_CH_B, &c_b,
                          bucket[1],              // write to bucket[1]
                          &pio_cam->rxf[sm_cam],  // read from PIO RX FIFO
                          HALF_FRAME_XFERS,       // 38,400 x 16-bit transfers
                          false);                 // don't start yet

    // Reset rotation state
    write_pos = 0;
    frame_ready = false;
    read_in_progress = false;

    // Enable IRQ on both channels
    dma_channel_set_irq0_enabled(DMA_CH_A, true);
    dma_channel_set_irq0_enabled(DMA_CH_B, true);

    irq_add_shared_handler(DMA_IRQ_0, cam_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    mp_printf(MP_PYTHON_PRINTER, "irq_add_shared_handler: cam_handler (3-bucket)\n");

    irq_set_enabled(DMA_IRQ_0, true);
    dma_channel_start(DMA_CH_A); // Start first half-frame capture
}

/********************************************************************************
function:   DMA interrupt processing function (3-bucket half-frame rotation)
parameter:
********************************************************************************/
static void handle_half_complete(uint32_t completed_ch)
{
    uint8_t completed_bucket = write_pos % 3;
    write_pos++;

    // Reconfigure this channel's write address for 2 positions ahead in rotation.
    // It won't be triggered again until the other channel finishes its half,
    // so we have ~61ms before this address is used.
    dma_channel_set_write_addr(completed_ch,
                               bucket[(completed_bucket + 2) % 3], false);

    // Every 2nd half-frame completes a full frame
    if ((write_pos & 1) == 0) {
        if (!read_in_progress) {
            frame_second_idx = completed_bucket;
            frame_first_idx = (completed_bucket + 2) % 3;
            frame_ready = true;
        }
        // else: frame dropped silently (send loop busy)
    }
}

void cam_handler(void)
{
    uint32_t ints = dma_hw->ints0;
    if (ints & (1u << DMA_CH_A)) {
        dma_hw->ints0 = 1u << DMA_CH_A;
        handle_half_complete(DMA_CH_A);
    }
    if (ints & (1u << DMA_CH_B)) {
        dma_hw->ints0 = 1u << DMA_CH_B;
        handle_half_complete(DMA_CH_B);
    }
}

/********************************************************************************
function:   Frame read control functions for Python (protects read buckets)
********************************************************************************/
void cam_start_read(void)
{
    read_in_progress = true;
}

void cam_end_read(void)
{
    read_in_progress = false;
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
    // Use the lowest data pin as the base for PIO, and 8 pins for D0-D7
    uint32_t cam_base_pin = g_cam_pinmap.d[0];
    uint32_t cam_num_pins = 11;
    uint32_t offset_cam = pio_add_program(pio_cam, &picampinos_program);
    picampinos_program_init(pio_cam, sm_cam, offset_cam, cam_base_pin, cam_num_pins);
    // Enable the state machine and clear the FIFO
    pio_sm_set_enabled(pio_cam, sm_cam, false);
    pio_sm_clear_fifos(pio_cam, sm_cam);
    pio_sm_restart(pio_cam, sm_cam);
    pio_sm_set_enabled(pio_cam, sm_cam, true);

    // Setting the X and Y registers
    pio_sm_put_blocking(pio_cam, sm_cam, 0);                  // X=0 : reserved
    pio_sm_put_blocking(pio_cam, sm_cam, (CAM_FUL_SIZE - 1)); // Y: total words in an image
    mp_printf(MP_PYTHON_PRINTER, "start_cam finished, camera started\n");
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
            tight_loop_contents(); // wait for data
        }
    }
}

/********************************************************************************
function:   Release the camera
parameter:
********************************************************************************/
void free_cam()
{
    // Disable IRQ settings
    irq_set_enabled(DMA_IRQ_0, false);
    dma_channel_set_irq0_enabled(DMA_CH_A, false);
    dma_channel_set_irq0_enabled(DMA_CH_B, false);
    dma_channel_abort(DMA_CH_A);
    dma_channel_abort(DMA_CH_B);
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

    // config
    pwm_slice_config = pwm_get_default_config();
    pwm_config_set_wrap(&pwm_slice_config, period);

    // set clk div
    pwm_config_set_clkdiv(&pwm_slice_config, 1);

    // set PWM start
    pwm_init(pwm0_slice_num, &pwm_slice_config, true);
    pwm_set_gpio_level(gpio_num, (pwm_slice_config.top * 0.50)); // duty:50%
}

