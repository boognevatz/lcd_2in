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

// W5500 direct buffer access
#include "lib/wiznet5k/Ethernet/wizchip_conf.h"
#include "lib/wiznet5k/Ethernet/socket.h"

// For streaming
#include <stdint.h>
#include <stdbool.h>


// init PIO
static PIO pio_cam = pio0;

// statemachine's pointer
static uint32_t sm_cam; // CAMERA's state machines

// dma channels
static uint32_t DMA_CAM_RD_CH;

static uint8_t cam_buffer[CAM_FUL_SIZE * 2];
uint8_t *cam_ptr = cam_buffer;


uint8_t pin_i2c1_sda = 22; // default on RP2350 touch 2in
uint8_t pin_i2c1_scl = 23; // default on RP2350 touch 2in
uint8_t pin_xclk_pwm = 11; // GPIO11 (camera's xclk(24MHz))


// flag
volatile bool buffer_ready = false;

// Streaming variables
static uint8_t dest_ip[4];
static uint16_t dest_port;
static uint8_t stream_socket_num;




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
    // init DMA
    DMA_CAM_RD_CH = dma_claim_unused_channel(true);
    mp_printf(MP_PYTHON_PRINTER, "setup_dma_for_capture()->DMA_CH= %d\n", (int)DMA_CAM_RD_CH);
    
    // Disable IRQ
    irq_set_enabled(DMA_IRQ_0, false);

     // Configure DMA Channel 0
    dma_channel_config c0 = get_cam_config(pio_cam, sm_cam, DMA_CAM_RD_CH);
    
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_16);
    
    dma_channel_configure(DMA_CAM_RD_CH, &c0,
                          cam_ptr,               // Destination pointer
                          &pio_cam->rxf[sm_cam], // Source pointer
                          sizeof(cam_buffer) / 2,          // Number of transfers
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
    
    dma_channel_set_write_addr(DMA_CAM_RD_CH, cam_ptr, true);
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

/********************************************************************************
function:   Initialize streaming
parameter:  dest_ip - destination IP address, dest_port - destination port
********************************************************************************/
void init_streaming(uint8_t *dest_ip_addr, uint16_t port)
{
    // This function is deprecated - use start_streaming instead
    memcpy(dest_ip, dest_ip_addr, 4);
    dest_port = port;
    // Socket initialization is now handled in Python
}

void start_streaming(uint8_t sock_num)
{
    stream_socket_num = sock_num;
}

/********************************************************************************
function:   Streaming loop (event loop style, no threads)
parameter:
********************************************************************************/
void streaming_loop(void)
{
    // Send multipart boundary and headers first
    const char *boundary_header = "--frame\r\nContent-Type: application/octet-stream\r\n\r\n";
    uint16_t boundary_len = strlen(boundary_header);

    // Wait for buffer space
    while (getSn_TX_FSR(stream_socket_num) < boundary_len) {
        // Spin wait
    }

    uint16_t wr = getSn_TX_WR(stream_socket_num);
    uint32_t addrsel = ((uint32_t)wr << 8) + (WIZCHIP_TXBUF_BLOCK(stream_socket_num) << 3);
    WIZCHIP_WRITE_BUF(addrsel, (uint8_t*)boundary_header, boundary_len);
    setSn_TX_WR(stream_socket_num, (uint16_t)(wr + boundary_len));
    setSn_CR(stream_socket_num, Sn_CR_SEND);

    while (1) {
        // Wait for frame ready
        if (!buffer_ready) {
            continue;  // Busy wait, or optionally sleep a few microseconds
        }
        buffer_ready = false;  // Reset flag

        // Send frame data in chunks
        uint16_t chunk_size = 16384;  // 16KB chunks (fits in W5500 buffer)
        uint32_t total_sent = 0;
        while (total_sent < FRAME_SIZE) {
            uint16_t send_len = (FRAME_SIZE - total_sent > chunk_size) ? chunk_size : (FRAME_SIZE - total_sent);

            // Wait for enough TX buffer space
            while (getSn_TX_FSR(stream_socket_num) < send_len) {
                // Spin wait for buffer space
            }

            // Get TX buffer write pointer
            uint16_t wr = getSn_TX_WR(stream_socket_num);

            // Calculate buffer address for direct write
            uint32_t addrsel = ((uint32_t)wr << 8) + (WIZCHIP_TXBUF_BLOCK(stream_socket_num) << 3);

            // Write data directly to TX buffer
            WIZCHIP_WRITE_BUF(addrsel, cam_ptr + total_sent, send_len);

            // Update write pointer
            setSn_TX_WR(stream_socket_num, (uint16_t)(wr + send_len));

            // Issue SEND command
            setSn_CR(stream_socket_num, Sn_CR_SEND);

            total_sent += send_len;
        }

        // Send boundary for next frame
        const char *frame_boundary = "\r\n--frame\r\nContent-Type: application/octet-stream\r\n\r\n";
        uint16_t frame_boundary_len = strlen(frame_boundary);

        // Wait for buffer space
        while (getSn_TX_FSR(stream_socket_num) < frame_boundary_len) {
            // Spin wait
        }

        wr = getSn_TX_WR(stream_socket_num);
        addrsel = ((uint32_t)wr << 8) + (WIZCHIP_TXBUF_BLOCK(stream_socket_num) << 3);
        WIZCHIP_WRITE_BUF(addrsel, (uint8_t*)frame_boundary, frame_boundary_len);
        setSn_TX_WR(stream_socket_num, (uint16_t)(wr + frame_boundary_len));
        setSn_CR(stream_socket_num, Sn_CR_SEND);
    }
}
