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
#include <math.h>
#include <string.h>
#include "cam.h"
#include "py/mpprint.h"
#include "py/runtime.h"

#include "shared/runtime/mpirq.h"
#include "ov5640.h"


// init PIO
static PIO pio_cam = pio0;

// statemachine's pointer
static uint32_t sm_cam; // CAMERA's state machines

// dma channels
static uint32_t DMA_CAM_RD_CH;

uint8_t *cam_ptr = NULL;

uint8_t pin_i2c1_sda = 22; // default on RP2350 touch 2in
uint8_t pin_i2c1_scl = 23; // default on RP2350 touch 2in
uint8_t pin_xclk_pwm = 11; // GPIO11 (camera's xclk(24MHz))


// flag
volatile bool buffer_ready = false;




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



/********************************************************************************
function:   Configuring DMA
parameter:
********************************************************************************/
void config_cam_buffer(mp_obj_t buf_obj)
{
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_obj, &bufinfo, MP_BUFFER_RW);
    cam_ptr = bufinfo.buf;
    size_t len = bufinfo.len;

    // init DMA
    DMA_CAM_RD_CH = dma_claim_unused_channel(true);
    mp_printf(MP_PYTHON_PRINTER, "config_cam_buffer()->DMA_CH= %d\n", (int)DMA_CAM_RD_CH);
    
    // Disable IRQ
    irq_set_enabled(DMA_IRQ_0, false);

     // Configure DMA Channel 0
    dma_channel_config c0 = get_cam_config(pio_cam, sm_cam, DMA_CAM_RD_CH);
    
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_16);
    
    dma_channel_configure(DMA_CAM_RD_CH, &c0,
                          cam_ptr,               // Destination pointer
                          &pio_cam->rxf[sm_cam], // Source pointer
                          len,          // Number of transfers
                          false                  // Don't Start yet
    );
    
    // IRQ settings
    dma_channel_set_irq0_enabled(DMA_CAM_RD_CH, true);
    
    //irq_set_exclusive_handler(DMA_IRQ_0, cam_handler); //NOT WORKING, micropython has IRQ already!
    irq_add_shared_handler(DMA_IRQ_0, cam_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    mp_printf(MP_PYTHON_PRINTER, "irq_add_shared_handler: cam_handler\n");
    
    irq_set_enabled(DMA_IRQ_0, true);
    dma_channel_start(DMA_CAM_RD_CH); // Start DMA transfer
}

/********************************************************************************
function:   DMA interrupt processing function
parameter:
********************************************************************************/
void cam_handler(void)
{
    buffer_ready = true;
    dma_hw->ints0 = 1u << DMA_CAM_RD_CH;  // clear the interrupt flag
    
    // uint32_t triggered_dma = dma_hw->ints0;
    // if (triggered_dma & (1u << DMA_CAM_RD_CH))
    // {
    //     buffer_ready = true;
    //     // Clear interrupt flag
    //     dma_hw->ints0 = 1u << DMA_CAM_RD_CH;
    // }

    // Reset DMA write address to capture the next frame
    // reset the DMA initial write address
    dma_channel_set_write_addr(DMA_CAM_RD_CH, cam_ptr, true);
}

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
// PWDN  GPIO 21
// SDA   GPIO 22
// SCL   GPIO 23

// NEW HARDWARE
// D0    GPIO 19
// D1    GPIO 18
// D2    GPIO 17
// D3    GPIO 16
// D4    GPIO 15
// D5    GPIO 14
// D6    GPIO 13
// D7    GPIO 12
//  D8   GPIO 11
//  D9   GPIO 10
// VSYNC GPIO 9
// HREF  GPIO 8
// XCLK  GPIO 7
// PCLK  GPIO 6
// SDA   GPIO 24
// SCL   GPIO 25

/********************************************************************************
function:   Start the camera
parameter:
********************************************************************************/
void start_cam()
{
    uint32_t offset_cam = pio_add_program(pio_cam, &picampinos_program);
    // uint32_t sm = 0; 
    picampinos_program_init(pio_cam, sm_cam, offset_cam, CAM_BASE_PIN, 11); // VSYNC,HREF,PCLK,D[2:9] : total 11 pins
    // Enable the state machine and clear the FIFO
    pio_sm_set_enabled(pio_cam, sm_cam, false);
    pio_sm_clear_fifos(pio_cam, sm_cam);
    pio_sm_restart(pio_cam, sm_cam);
    pio_sm_set_enabled(pio_cam, sm_cam, true);

    // Setting the X and Y registers
    pio_sm_put_blocking(pio_cam, sm_cam, 0);                  // X=0 : reserved
    pio_sm_put_blocking(pio_cam, sm_cam, (CAM_FUL_SIZE - 1)); // Y: total words in an image
    mp_printf(MP_PYTHON_PRINTER, "start_cam finished, camera started\n");
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

    // config
    pwm_slice_config = pwm_get_default_config();
    pwm_config_set_wrap(&pwm_slice_config, period);

    // set clk div
    pwm_config_set_clkdiv(&pwm_slice_config, 1);

    // set PWM start
    pwm_init(pwm0_slice_num, &pwm_slice_config, true);
    pwm_set_gpio_level(gpio_num, (pwm_slice_config.top * 0.50)); // duty:50%
}
