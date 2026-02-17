/*****************************************************************************
* | File      	:   cam.h
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
#include <stdio.h>
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "LCD_2in.h"
#include "py/obj.h"

// camera buffer size
// 240x320, RGB565 picture needs 240x320x2 bytes of buffers.
#define CAM_FUL_SIZE (LCD_2IN_HEIGHT * LCD_2IN_WIDTH)                 

// 3-bucket half-frame DMA chaining
#define FRAME_BYTES       (CAM_FUL_SIZE * 2)                    // 153,600
#define HALF_FRAME_BYTES  (FRAME_BYTES / 2)                     // 76,800
#define HALF_FRAME_XFERS  (HALF_FRAME_BYTES / sizeof(uint16_t)) // 38,400

extern uint8_t *bucket[3];              // 3 half-frame buckets
extern volatile bool frame_ready;       // true when a complete frame is available
extern volatile uint8_t frame_first_idx;  // bucket index of frame's first half
extern volatile uint8_t frame_second_idx; // bucket index of frame's second half
extern volatile bool read_in_progress;  // set by Python to protect read buckets

#define USE_100BASE_FX (false)

#define SYS_CLK_IN_KHZ (150000) // 192000 ~ 250000 (if you use sfp, SYS_CLK_KHZ must be just 250000)
#define CAM_BASE_PIN (0)        // GP1  (camera module needs 11pin)

extern uint8_t pin_i2c1_sda;
extern uint8_t pin_i2c1_scl;
extern uint8_t pin_xclk_pwm;
void set_i2c_pins(uint8_t sda, uint8_t scl);
void set_pwm_pin(uint8_t pwm);

// high layer APIs
void init_cam();
void start_cam();
void free_cam();
void set_pwm_freq_kHz(uint32_t freq_khz, uint8_t gpio_num);
void read_cam_data_blocking(uint8_t *buffer, size_t length);
dma_channel_config get_cam_config(PIO pio, uint32_t sm, uint32_t dma_chan);
void cam_handler();
void setup_dma_for_capture();

// Frame read control - call from Python
void cam_start_read(void);   // Call before reading frame - protects read buckets
void cam_end_read(void);     // Call after reading frame - allows frame updates

// Camera pin mapping struct for runtime configuration
typedef struct {
    uint8_t d[8]; // D0-D7
    uint8_t vsync;
    uint8_t href;
    uint8_t pclk;
    uint8_t xclk;
} cam_pinmap_t;

// Global pinmap instance
extern cam_pinmap_t g_cam_pinmap;
