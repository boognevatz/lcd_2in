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
#include "py/runtime.h"

#include "shared/runtime/mpirq.h"



// init PIO
static PIO pio_cam = pio0;

// statemachine's pointer
static uint32_t sm_cam; // CAMERA's state machines

// dma channels
static uint32_t DMA_CAM_RD_CH;

// private functions and buffers
uint8_t *cam_ptr = NULL;  // pointer of camera buffer


// flag
volatile bool buffer_ready = false;

/********************************************************************************
function:   Camera initialization
parameter:
********************************************************************************/
void init_cam()
{
    // Free existing buffer if re-initing
    if (cam_ptr != NULL) {
        m_free(cam_ptr);
        cam_ptr = NULL;
    }
    

    // Initialize CAMERA
    set_pwm_freq_kHz(37000, PIN_PWM);
    sleep_ms(50);
    sccb_init(I2C1_SDA, I2C1_SCL); // sda,scl=(gp26,gp27). see 'sccb_if.c' and 'cam.h'
    sleep_ms(50);

    // import gc; gc.mem_free() = 460144
    // buffer of camera data is LCD_2IN_WIDTH * LCD_2IN_HEIGHT * 2 bytes (RGB565 = 16 bits = 2 bytes)
    mp_printf(MP_PYTHON_PRINTER, "before malloc\n");
    cam_ptr = (uint8_t *)m_malloc(CAM_FUL_SIZE * 2);  //320*240*2 =153600
    //320*240*2 =153600
    mp_printf(MP_PYTHON_PRINTER, "after malloc %d\n", CAM_FUL_SIZE * 2);
    memset(cam_ptr, 0, CAM_FUL_SIZE * 2);
    (void)cam_ptr;
}



/********************************************************************************
function:   Configuring DMA
parameter:
********************************************************************************/
void config_cam_buffer()
{
    // init DMA
    DMA_CAM_RD_CH = dma_claim_unused_channel(true);
    mp_printf(MP_PYTHON_PRINTER, "DMA_CH= %d\n", (int)DMA_CAM_RD_CH);
    
    // Disable IRQ
    irq_set_enabled(DMA_IRQ_0, false);
    mp_printf(MP_PYTHON_PRINTER, "irq_set_enabled\n");

     // Configure DMA Channel 0
    dma_channel_config c0 = get_cam_config(pio_cam, sm_cam, DMA_CAM_RD_CH);
    mp_printf(MP_PYTHON_PRINTER, "dma_channel_config c0= %d\n", (int)&c0);
    
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_16);
    mp_printf(MP_PYTHON_PRINTER, "channel_config_set_transfer_data_size\n");
    
    dma_channel_configure(DMA_CAM_RD_CH, &c0,
                          cam_ptr,               // Destination pointer
                          &pio_cam->rxf[sm_cam], // Source pointer
                          CAM_FUL_SIZE,          // Number of transfers
                          false                  // Don't Start yet
    );
    mp_printf(MP_PYTHON_PRINTER, "dma_channel_configure cam_ptr: %d\n", (int)cam_ptr);
    
    // IRQ settings
    dma_channel_set_irq0_enabled(DMA_CAM_RD_CH, true);
    mp_printf(MP_PYTHON_PRINTER, "dma_channel_set_irq0_enabled:\n");
    
    //irq_set_exclusive_handler(DMA_IRQ_0, cam_handler);
    irq_add_shared_handler(DMA_IRQ_0, cam_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    mp_printf(MP_PYTHON_PRINTER, "irq_add_shared_handler: cam_handler\n");
    
    // irq_set_enabled(DMA_IRQ_0, true);
    // dma_channel_start(DMA_CAM_RD_CH); // Start DMA transfer
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
