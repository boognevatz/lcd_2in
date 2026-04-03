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
#include "py/obj.h"

// JPEG buffer size: 160KB per buffer (2x320KB total, matches stable RGB565 era)
#define JPEG_BUF_SIZE 163840

// Double buffering - DMA writes to one buffer while Python reads from other
extern uint8_t *cam_dma_write_buf;    // DMA writes here (do not read from Python)
extern uint8_t *cam_python_read_buf;  // Python reads here (safe, not being written)
extern volatile bool buffer_ready;
extern volatile bool read_in_progress; // Set by Python to protect read buffer

// JPEG frame size tracking
extern volatile uint32_t jpeg_frame_size;     // Size of last completed JPEG frame
extern volatile uint32_t jpeg_write_size;      // Size being written by DMA

// Legacy pointer for compatibility (points to read buffer)
extern uint8_t *cam_ptr;

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
void init_cam_with_registers(const uint16_t custom_regs[][2], uint16_t num_regs);
void start_cam();
void free_cam();
void set_pwm_freq_kHz(uint32_t freq_khz, uint8_t gpio_num);
void read_cam_data_blocking(uint8_t *buffer, size_t length);
dma_channel_config get_cam_config(PIO pio, uint32_t sm, uint32_t dma_chan);
void setup_dma_for_capture();

// Double buffer control - call from Python
void cam_start_read(void);   // Call before reading frame - locks read buffer
void cam_end_read(void);     // Call after reading frame - allows buffer swap
uint8_t* cam_get_read_buffer(void);  // Get pointer to safe read buffer
uint32_t cam_get_frame_size(void);   // Get current readable frame's JPEG size

// VSYNC frame boundary mode + capture diagnostics
void cam_set_vsync_end_on_rising(bool enabled);
bool cam_get_vsync_end_on_rising(void);
void cam_reset_diag_stats(void);
uint32_t cam_get_vsync_rise_count(void);
uint32_t cam_get_vsync_fall_count(void);
uint32_t cam_get_vsync_frame_end_count(void);
uint32_t cam_get_last_capture_size(void);
int32_t cam_get_last_soi_pos(void);
int32_t cam_get_last_eoi_pos(void);
uint32_t cam_get_last_sample_nonzero(void);
const uint8_t* cam_get_capture_head(void); // first 32 bytes of last capture
uint32_t cam_get_last_sample_ff(void);
uint32_t cam_get_last_sample_len(void);

// ISR-level FPS counter (frames per second * 10, e.g., 125 = 12.5fps)
uint32_t cam_get_fps_x10(void);

// ISR duration in microseconds (for bottleneck diagnosis)
uint32_t cam_get_isr_duration_us(void);

// Combined stream info — one call instead of multiple Python→C round-trips
void cam_get_stream_info(uint32_t *out_frame_size, int32_t *out_soi_pos,
                         int32_t *out_eoi_pos, bool *out_ready,
                         uint32_t *out_vsync_count, uint32_t *out_fps_x10,
                         uint32_t *out_isr_us);

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
