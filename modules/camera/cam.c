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

#include "pico/time.h"

// Timeout constants for flow control
#define TX_WAIT_TIMEOUT_MS      5000    // Max time to wait for TX buffer space
#define SENDOK_TIMEOUT_MS       3000    // Max time to wait for SENDOK
#define TX_POLL_INTERVAL_US     100     // Microseconds between TX_FSR checks

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

// Helper to get current time in milliseconds
static inline uint32_t get_time_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

// Check if timeout has elapsed
static inline bool timeout_elapsed(uint32_t start_ms, uint32_t timeout_ms) {
    return (get_time_ms() - start_ms) >= timeout_ms;
}

// Helper to pause camera DMA during W5500 transfers
static void pause_camera_dma(void) {
    // Disable camera DMA IRQ to prevent interference
    dma_channel_set_irq0_enabled(DMA_CAM_RD_CH, false);
    // Abort any in-progress camera DMA
    dma_channel_abort(DMA_CAM_RD_CH);
}

// Helper to resume camera DMA
static void resume_camera_dma(void) {
    // Re-enable camera DMA IRQ
    dma_channel_set_irq0_enabled(DMA_CAM_RD_CH, true);
    // Restart camera DMA
    dma_channel_set_write_addr(DMA_CAM_RD_CH, cam_ptr, true);
}

/********************************************************************************
function:   Streaming loop with proper W5500 flow control
parameter:
********************************************************************************/
void streaming_loop(void)
{
    // CRITICAL: Pause camera DMA FIRST, before any SPI or printf operations
    // This prevents DMA conflicts between camera and W5500/USB
    pause_camera_dma();
    
    mp_printf(MP_PYTHON_PRINTER, "Camera DMA paused, starting streaming setup\n");
    
    // Send multipart boundary and headers first
    const char *boundary_header = "--frame\r\nContent-Type: application/octet-stream\r\n\r\n";
    uint16_t boundary_len = strlen(boundary_header);
    const char *frame_boundary = "\r\n--frame\r\nContent-Type: application/octet-stream\r\n\r\n";
    uint16_t frame_boundary_len = strlen(frame_boundary);
    
    // Get TX buffer size (should be 16384 bytes)
    uint16_t tx_buffer_size = getSn_TxMAX(stream_socket_num);
    uint16_t tx_buffer_mask = tx_buffer_size - 1;  // For wraparound
    
    mp_printf(MP_PYTHON_PRINTER, "Streaming on socket %d, TX buffer: %d bytes\n", 
              stream_socket_num, tx_buffer_size);

    // NEW: Verify W5500 socket configuration
    uint8_t socket_mode = getSn_MR(stream_socket_num);
    uint16_t rx_buffer_size = getSn_RxMAX(stream_socket_num);
    uint16_t mss = getSn_MSSR(stream_socket_num);
    
    mp_printf(MP_PYTHON_PRINTER, "Socket %d config: Mode=0x%02x, TX=%d, RX=%d, MSS=%d\n",
              stream_socket_num, socket_mode, tx_buffer_size, rx_buffer_size, mss);
    
    // Verify we're in TCP mode
    if ((socket_mode & 0x0F) != Sn_MR_TCP) {
        mp_printf(MP_PYTHON_PRINTER, "ERROR: Socket not in TCP mode!\n");
        resume_camera_dma();
        return;
    }

    // CRITICAL FIX: Clear any pending interrupts from Python's header send
    // before we start our streaming loop. Python sends the HTTP header via
    // cl.send() which may leave a pending SENDOK that we need to acknowledge.
    uint8_t pending_ir = getSn_IR(stream_socket_num);
    if (pending_ir & (Sn_IR_SENDOK | Sn_IR_TIMEOUT)) {
        mp_printf(MP_PYTHON_PRINTER, "Clearing pending IR flags from Python: 0x%02x\n", pending_ir);
        setSn_IR(stream_socket_num, pending_ir & (Sn_IR_SENDOK | Sn_IR_TIMEOUT));
    }
    
    // Wait for TX buffer to be fully free (Python's HTTP header send completed)
    uint32_t python_drain_start = get_time_ms();
    while (getSn_TX_FSR(stream_socket_num) < tx_buffer_size) {
        if (timeout_elapsed(python_drain_start, 2000)) {
            mp_printf(MP_PYTHON_PRINTER, "TIMEOUT: Python TX not draining (FSR=%d/%d)\n",
                      getSn_TX_FSR(stream_socket_num), tx_buffer_size);
            resume_camera_dma();
            return;
        }
        uint8_t sr = getSn_SR(stream_socket_num);
        if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT) {
            mp_printf(MP_PYTHON_PRINTER, "Socket disconnected while waiting for Python TX drain\n");
            resume_camera_dma();
            return;
        }
        sleep_ms(1);
    }
    mp_printf(MP_PYTHON_PRINTER, "Python header drained, TX_FSR=%d\n", getSn_TX_FSR(stream_socket_num));

    // Send initial boundary and headers (with timeout)
    uint32_t tx_wait_start = get_time_ms();
    while (getSn_TX_FSR(stream_socket_num) < boundary_len) {
        if (timeout_elapsed(tx_wait_start, TX_WAIT_TIMEOUT_MS)) {
            mp_printf(MP_PYTHON_PRINTER, "TIMEOUT waiting for TX buffer space for initial boundary\n");
            resume_camera_dma();
            return;
        }
        uint8_t sr = getSn_SR(stream_socket_num);
        if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT) {
            mp_printf(MP_PYTHON_PRINTER, "Socket disconnected before initial boundary send\n");
            resume_camera_dma();
            return;
        }
        sleep_us(TX_POLL_INTERVAL_US);
    }
    
    uint16_t wr = getSn_TX_WR(stream_socket_num);
    uint16_t offset = wr & tx_buffer_mask;
    
    // Handle wraparound for boundary header
    if (offset + boundary_len > tx_buffer_size) {
        uint16_t first_part = tx_buffer_size - offset;
        uint16_t second_part = boundary_len - first_part;
        
        uint32_t addrsel = ((uint32_t)offset << 8) + (WIZCHIP_TXBUF_BLOCK(stream_socket_num) << 3);
        WIZCHIP_WRITE_BUF(addrsel, (uint8_t*)boundary_header, first_part);
        
        addrsel = (WIZCHIP_TXBUF_BLOCK(stream_socket_num) << 3);
        WIZCHIP_WRITE_BUF(addrsel, (uint8_t*)boundary_header + first_part, second_part);
    } else {
        uint32_t addrsel = ((uint32_t)offset << 8) + (WIZCHIP_TXBUF_BLOCK(stream_socket_num) << 3);
        WIZCHIP_WRITE_BUF(addrsel, (uint8_t*)boundary_header, boundary_len);
    }
    
    setSn_TX_WR(stream_socket_num, (uint16_t)(wr + boundary_len));
    setSn_CR(stream_socket_num, Sn_CR_SEND);
    
    // Wait for SEND command to complete
    while (getSn_CR(stream_socket_num)) {
        tight_loop_contents();
    }
    
    // Wait for SENDOK WITH TIMEOUT (this was hanging before!)
    uint32_t header_sendok_start = get_time_ms();
    while (!(getSn_IR(stream_socket_num) & Sn_IR_SENDOK)) {
        // Check for W5500 timeout flag
        if (getSn_IR(stream_socket_num) & Sn_IR_TIMEOUT) {
            mp_printf(MP_PYTHON_PRINTER, "W5500 TIMEOUT during initial header send!\n");
            setSn_IR(stream_socket_num, Sn_IR_TIMEOUT);
            resume_camera_dma();
            return;
        }
        
        uint8_t sr = getSn_SR(stream_socket_num);
        if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT) {
            mp_printf(MP_PYTHON_PRINTER, "Socket disconnected during initial header send (state=0x%02x)\n", sr);
            resume_camera_dma();
            return;
        }
        
        // Add timeout to prevent infinite hang
        if (timeout_elapsed(header_sendok_start, SENDOK_TIMEOUT_MS)) {
            mp_printf(MP_PYTHON_PRINTER, "TIMEOUT waiting for initial SENDOK! IR=0x%02x, FSR=%d\n",
                      getSn_IR(stream_socket_num), getSn_TX_FSR(stream_socket_num));
            resume_camera_dma();
            return;
        }
        
        sleep_us(TX_POLL_INTERVAL_US);
    }
    setSn_IR(stream_socket_num, Sn_IR_SENDOK);
    
    mp_printf(MP_PYTHON_PRINTER, "Initial C boundary header sent, starting frame loop\n");
    
    uint32_t frame_count = 0;
    
    // Since camera DMA is paused, we'll use the last captured frame in cam_buffer
    // For testing: just stream the static buffer content to verify W5500 DMA works
    mp_printf(MP_PYTHON_PRINTER, "Using static frame buffer (camera DMA paused)\n");

    while (1) {
        // Check socket state
        uint8_t sr = getSn_SR(stream_socket_num);
        if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT) {
            mp_printf(MP_PYTHON_PRINTER, "Socket disconnected, state: 0x%02x\n", sr);
            resume_camera_dma();  // Restore camera DMA before returning
            return;
        }
        
        // No longer waiting for buffer_ready since camera DMA is paused
        // We'll just send the current buffer content as a static frame
        // Add a small delay to control frame rate (~30fps = 33ms per frame)
        sleep_ms(33);
        
        frame_count++;
        
        // Diagnostic: Log buffer state at start of each frame
        if (frame_count % 10 == 1) {  // Every 10 frames
            mp_printf(MP_PYTHON_PRINTER, "Frame %d: TX_FSR=%d, TX_WR=%d, TX_RD=%d, RX_RSR=%d\n",
                      frame_count,
                      getSn_TX_FSR(stream_socket_num),
                      getSn_TX_WR(stream_socket_num),
                      getSn_TX_RD(stream_socket_num),
                      getSn_RX_RSR(stream_socket_num));
        }

        // Send frame data in chunks with proper flow control
        // Use smaller chunks to allow TCP ACKs to arrive between sends
        // 2KB allows ~8 chunks per TX buffer, giving more opportunities for ACK processing
        uint16_t chunk_size = 8192;  // 8kB -> 2kB chunks for safety
        uint32_t total_sent = 0;
        
        while (total_sent < FRAME_SIZE) {
            uint16_t remaining = FRAME_SIZE - total_sent;
            uint16_t send_len = (remaining > chunk_size) ? chunk_size : remaining;


            // Quick check for TX buffer space - if not available, skip this frame
            uint16_t free_space = getSn_TX_FSR(stream_socket_num);
            if (free_space < send_len) {
                uint32_t quick_wait_start = get_time_ms();
       
                // Wait up to 100ms for buffer space
                while (free_space < send_len && !timeout_elapsed(quick_wait_start, 100)) {
                    sr = getSn_SR(stream_socket_num);
                    if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT) {
                        mp_printf(MP_PYTHON_PRINTER, "Socket disconnected\n");
                        resume_camera_dma();
                        return;
                    }
                    sleep_us(500);
                    free_space = getSn_TX_FSR(stream_socket_num);
                }
       
                // If still no space, skip remaining frame data and try next frame
                if (free_space < send_len) {
                    mp_printf(MP_PYTHON_PRINTER, "Skipping frame %d - TX buffer busy\n", frame_count);
                    break;  // Break inner while loop, continue to next frame
                }
            }

            // Get current write pointer and calculate offset
            wr = getSn_TX_WR(stream_socket_num);
            offset = wr & tx_buffer_mask;
            
            // Handle buffer wraparound
            if (offset + send_len > tx_buffer_size) {
                // Split transfer across buffer boundary
                uint16_t first_part = tx_buffer_size - offset;
                uint16_t second_part = send_len - first_part;
                
                // Write first part (to end of buffer)
                uint32_t addrsel = ((uint32_t)offset << 8) + (WIZCHIP_TXBUF_BLOCK(stream_socket_num) << 3);
                WIZCHIP_WRITE_BUF(addrsel, cam_ptr + total_sent, first_part);
                
                // Write second part (from start of buffer)
                addrsel = (WIZCHIP_TXBUF_BLOCK(stream_socket_num) << 3);
                WIZCHIP_WRITE_BUF(addrsel, cam_ptr + total_sent + first_part, second_part);
            } else {
                // No wraparound needed
                uint32_t addrsel = ((uint32_t)offset << 8) + (WIZCHIP_TXBUF_BLOCK(stream_socket_num) << 3);
                WIZCHIP_WRITE_BUF(addrsel, cam_ptr + total_sent, send_len);
            }

            // Update write pointer
            setSn_TX_WR(stream_socket_num, (uint16_t)(wr + send_len));

            // Issue SEND command
            setSn_CR(stream_socket_num, Sn_CR_SEND);
            
            // Wait for SEND command to complete
            while (getSn_CR(stream_socket_num)) {
                tight_loop_contents();
            }
            
            // Wait for SENDOK interrupt WITH TIMEOUT
            uint32_t sendok_start = get_time_ms();
            while (!(getSn_IR(stream_socket_num) & Sn_IR_SENDOK)) {
                // Check for W5500 timeout flag
                if (getSn_IR(stream_socket_num) & Sn_IR_TIMEOUT) {
                    mp_printf(MP_PYTHON_PRINTER, "W5500 TIMEOUT flag set!\n");
                    setSn_IR(stream_socket_num, Sn_IR_TIMEOUT);
                    resume_camera_dma();
                    return;
                }
            
                // Check socket state
                sr = getSn_SR(stream_socket_num);
                if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT) {
                    mp_printf(MP_PYTHON_PRINTER, "Socket disconnected during frame send\n");
                    resume_camera_dma();
                    return;
                }
            
                // Check for our timeout
                if (timeout_elapsed(sendok_start, SENDOK_TIMEOUT_MS)) {
                    mp_printf(MP_PYTHON_PRINTER, "TIMEOUT waiting for SENDOK! IR=0x%02x\n",
                              getSn_IR(stream_socket_num));
                    resume_camera_dma();
                    return;
                }
            
                sleep_us(TX_POLL_INTERVAL_US);
            }



            // Clear SENDOK interrupt
            setSn_IR(stream_socket_num, Sn_IR_SENDOK);

            total_sent += send_len;
        }

        // Send boundary for next frame (with timeout)
        uint32_t boundary_wait_start = get_time_ms();
        while (getSn_TX_FSR(stream_socket_num) < frame_boundary_len) {
            if (timeout_elapsed(boundary_wait_start, TX_WAIT_TIMEOUT_MS)) {
                mp_printf(MP_PYTHON_PRINTER, "TIMEOUT waiting for TX buffer for boundary\n");
                resume_camera_dma();
                return;
            }
            sr = getSn_SR(stream_socket_num);
            if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT) {
                mp_printf(MP_PYTHON_PRINTER, "Socket disconnected before boundary\n");
                resume_camera_dma();
                return;
            }
            sleep_us(TX_POLL_INTERVAL_US);
        }

        wr = getSn_TX_WR(stream_socket_num);
        offset = wr & tx_buffer_mask;
        
        // Handle wraparound for frame boundary
        if (offset + frame_boundary_len > tx_buffer_size) {
            uint16_t first_part = tx_buffer_size - offset;
            uint16_t second_part = frame_boundary_len - first_part;
            
            uint32_t addrsel = ((uint32_t)offset << 8) + (WIZCHIP_TXBUF_BLOCK(stream_socket_num) << 3);
            WIZCHIP_WRITE_BUF(addrsel, (uint8_t*)frame_boundary, first_part);
            
            addrsel = (WIZCHIP_TXBUF_BLOCK(stream_socket_num) << 3);
            WIZCHIP_WRITE_BUF(addrsel, (uint8_t*)frame_boundary + first_part, second_part);
        } else {
            uint32_t addrsel = ((uint32_t)offset << 8) + (WIZCHIP_TXBUF_BLOCK(stream_socket_num) << 3);
            WIZCHIP_WRITE_BUF(addrsel, (uint8_t*)frame_boundary, frame_boundary_len);
        }
        
        setSn_TX_WR(stream_socket_num, (uint16_t)(wr + frame_boundary_len));
        setSn_CR(stream_socket_num, Sn_CR_SEND);
        
        // Wait for SEND command to complete
        while (getSn_CR(stream_socket_num)) {
            tight_loop_contents();
        }

        // Wait for SENDOK interrupt WITH TIMEOUT
        uint32_t sendok_start = get_time_ms();
        while (!(getSn_IR(stream_socket_num) & Sn_IR_SENDOK)) {
            // Check for W5500 timeout flag
            if (getSn_IR(stream_socket_num) & Sn_IR_TIMEOUT) {
                mp_printf(MP_PYTHON_PRINTER, "W5500 TIMEOUT flag set!\n");
                setSn_IR(stream_socket_num, Sn_IR_TIMEOUT);
                resume_camera_dma();
                return;
            }

            // Check socket state
            sr = getSn_SR(stream_socket_num);
            if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT) {
                mp_printf(MP_PYTHON_PRINTER, "Socket disconnected during boundary send\n");
                resume_camera_dma();
                return;
            }

            // Check for our timeout
            if (timeout_elapsed(sendok_start, SENDOK_TIMEOUT_MS)) {
                mp_printf(MP_PYTHON_PRINTER, "TIMEOUT waiting for SENDOK! IR=0x%02x\n",
                          getSn_IR(stream_socket_num));
                resume_camera_dma();
                return;
            }

            sleep_us(TX_POLL_INTERVAL_US);
        }
        setSn_IR(stream_socket_num, Sn_IR_SENDOK);
    }
}


