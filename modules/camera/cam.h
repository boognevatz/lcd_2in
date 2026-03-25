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
#define BUCKET_TAG_SIZE       12                                 // 12-byte tag (time_us + bucket tag + tx states)
#define TAGGED_HALF_FRAME_BYTES (BUCKET_TAG_SIZE + HALF_FRAME_BYTES) // 76,812

extern uint8_t *bucket[3];              // 3 half-frame buckets

// --- Three-bucket system shared state ---

// 32-bit atomic bucket state layout:
//   Bits 31-3: Frame number (29 bits)
//   Bit 2:     Half (1=UPPER, 0=LOWER)
//   Bit 1:     Dirty (1 = camera writing in progress)
//   Bit 0:     Valid (1 = valid data present, 0 = empty)
#define BUCKET_VALID_MASK   0x00000001U
#define BUCKET_DIRTY_MASK   0x00000002U
#define BUCKET_HALF_MASK    0x00000004U
#define BUCKET_FRAME_SHIFT  3

static inline uint32_t bucket_get_halfframe(volatile uint32_t s)   {
    return (s >> BUCKET_FRAME_SHIFT) * 2 + ((s & BUCKET_HALF_MASK) ? 0 : 1);
}
static inline bool     bucket_is_valid(volatile uint32_t s)       { return (s & BUCKET_VALID_MASK) != 0; }
static inline bool     bucket_is_dirty(volatile uint32_t s)       { return (s & BUCKET_DIRTY_MASK) != 0; }
static inline bool     bucket_half_is_upper(volatile uint32_t s)  { return (s & BUCKET_HALF_MASK) != 0; }
static inline bool     bucket_is_complete(volatile uint32_t s)    { return bucket_is_valid(s) && !bucket_is_dirty(s); }

static inline uint32_t bucket_make_empty(void) {
    return 0;
}

static inline uint32_t bucket_make_complete(uint32_t frame, bool is_upper) {
    return (frame << BUCKET_FRAME_SHIFT) | BUCKET_VALID_MASK |
           (is_upper ? BUCKET_HALF_MASK : 0);
}

static inline uint32_t bucket_make_dirty(uint32_t frame, bool is_upper) {
    return (frame << BUCKET_FRAME_SHIFT) | BUCKET_VALID_MASK | BUCKET_DIRTY_MASK |
           (is_upper ? BUCKET_HALF_MASK : 0);
}

// Camera counter: increments every half-frame
// frame = counter / 2, half = counter % 2 (0=UPPER, 1=LOWER)
extern volatile uint32_t cam_counter;

// Bucket state array: [0]=A, [1]=B, [2]=C
extern volatile uint32_t bucket_state[3];

// Per-bucket TX state (three_bucket_system.md terminology)
// Written by TX main thread, read+upgraded by ISR.
#define BUCKET_TX_STATE_FREE     0   // "-"      Not in TX pair. Freely overwritable.
#define BUCKET_TX_STATE_TXP      1   // "TXP"    TX sending, camera done.   PROTECTED.
#define BUCKET_TX_STATE_PD       2   // "PD"     Queued partner, camera done. PROTECTED.
#define BUCKET_TX_STATE_TX_REC   3   // "TX REC" TX sending, camera writing. Not protected.
#define BUCKET_TX_STATE_TX       4   // "TX"     TX sending stale/garbage.   Not protected.
#define BUCKET_TX_STATE_QUEUED   5   // Queued partner, camera still writing. Not protected.
                                     // (spec shows this as "REC" in the bucket column;
                                     //  ISR needs the distinction to transition -> PD)

extern volatile uint8_t  bucket_tx_state[3];
extern volatile uint32_t bucket_tx_min_counter[3]; // ISR only upgrades TX state if
                                                    // completed counter >= this value
extern volatile uint8_t  bucket_cemented_next_target[3];
extern volatile int8_t   cam_hint_next;
extern volatile int8_t   cam_hint_next_next;
extern volatile int8_t   cam_hint_next_next_next;
extern volatile int32_t  mcu_temp_x10;          // MCU temperature x10 (365 = 36.5C)

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
