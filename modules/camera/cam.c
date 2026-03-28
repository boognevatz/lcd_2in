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
#include "hardware/timer.h"

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

// 3 half-frame buckets (76,820 bytes each: 20-byte tag + 76,800 pixel data)
static uint8_t bucket_mem_0[TAGGED_HALF_FRAME_BYTES] __attribute__((aligned(4)));
static uint8_t bucket_mem_1[TAGGED_HALF_FRAME_BYTES] __attribute__((aligned(4)));
static uint8_t bucket_mem_2[TAGGED_HALF_FRAME_BYTES] __attribute__((aligned(4)));
uint8_t *bucket[3] = { bucket_mem_0, bucket_mem_1, bucket_mem_2 };

// DMA channels for half-frame chaining
static uint32_t DMA_CH_A;
static uint32_t DMA_CH_B;

// --- Three-bucket system state (32-bit atomic) ---

// Per-bucket state packed into single uint32_t (see cam.h for layout)
volatile uint32_t bucket_state[3] = {0, 0, 0};

// Camera counter: increments every half-frame
// frame = cam_counter / 2, is_upper = (cam_counter % 2) == 0
volatile uint32_t cam_counter = 0;

// TX intent (TX writes, ISR reads+upgrades)
volatile uint8_t bucket_tx_state[3] = {BUCKET_TX_STATE_FREE, BUCKET_TX_STATE_FREE, BUCKET_TX_STATE_FREE};
volatile uint32_t bucket_tx_min_counter[3] = {0, 0, 0};
volatile uint8_t bucket_tx_next_cemented[3] = {0, 0, 0};
volatile int8_t cam_hint_next = -1;
volatile int8_t cam_hint_next_next = -1;
volatile int8_t cam_hint_next_next_next = -1;
volatile int32_t mcu_temp_x10 = 365;            // default 36.5C until Python updates

// Internal ISR tracking: which bucket each DMA channel targets
// ch_target[0] = CH_A's target bucket, ch_target[1] = CH_B's target bucket
static volatile uint8_t ch_target[2] = {0, 1};

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
                          bucket[0] + BUCKET_TAG_SIZE, // write past 20-byte tag
                          &pio_cam->rxf[sm_cam],  // read from PIO RX FIFO
                          HALF_FRAME_XFERS,       // 38,400 x 16-bit transfers
                          false);                 // don't start yet

    // Configure CH_B: transfers one half-frame, then chains to CH_A
    dma_channel_config c_b = get_cam_config(pio_cam, sm_cam, DMA_CH_B);
    channel_config_set_transfer_data_size(&c_b, DMA_SIZE_16);
    channel_config_set_chain_to(&c_b, DMA_CH_A);
    dma_channel_configure(DMA_CH_B, &c_b,
                          bucket[1] + BUCKET_TAG_SIZE, // write past 20-byte tag
                          &pio_cam->rxf[sm_cam],  // read from PIO RX FIFO
                          HALF_FRAME_XFERS,       // 38,400 x 16-bit transfers
                          false);                 // don't start yet

    // Reset three-bucket system state
    for (int i = 0; i < 3; i++) {
        bucket_state[i] = bucket_make_empty();
        bucket_tx_state[i] = BUCKET_TX_STATE_FREE;
        bucket_tx_min_counter[i] = 0;
        bucket_tx_next_cemented[i] = 0;
    }
    cam_hint_next = -1;
    cam_hint_next_next = -1;
    cam_hint_next_next_next = -1;
    cam_counter = 0;
    ch_target[0] = 0;       // CH_A starts at bucket 0
    ch_target[1] = 1;       // CH_B starts at bucket 1

    // Mark bucket 0 as dirty (being written)
    // cam_counter=0 → frame=0, half=(0%2)=0 → UPPER, so is_upper=true
    bucket_state[0] = bucket_make_dirty(0, true);
    ((uint32_t *)bucket[0])[1] = bucket_state[0];
    // Keep cemented-next-target update paired with dirty update.
    bucket_tx_next_cemented[0] = 0;
    bucket_tx_next_cemented[1] = 1; // CH_B is preconfigured as next target
    bucket_tx_next_cemented[2] = 0;

    // Enable IRQ on both channels
    dma_channel_set_irq0_enabled(DMA_CH_A, true);
    dma_channel_set_irq0_enabled(DMA_CH_B, true);

    irq_add_shared_handler(DMA_IRQ_0, cam_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    mp_printf(MP_PYTHON_PRINTER, "irq_add_shared_handler: cam_handler (3-bucket skip)\n");

    irq_set_enabled(DMA_IRQ_0, true);
    dma_channel_start(DMA_CH_A); // Start first half-frame capture
}

/********************************************************************************
function:   Protection check for camera skip logic.
            A bucket is protected when it is TXP or PD (spec terminology).
            TX REC, TX, QUEUED, NONE — all unprotected.
********************************************************************************/
static inline bool is_protected(uint8_t b)
{
    uint8_t s = bucket_tx_state[b];
    return s == BUCKET_TX_STATE_TXP || s == BUCKET_TX_STATE_PD;
}

/********************************************************************************
function:   DMA interrupt handler -- 3-bucket system with skip logic.

            When a DMA channel finishes one half-frame:
            1. Captures current cam_counter state (frame + half)
            2. Marks completed bucket as complete (atomic 32-bit write)
            3. Increments cam_counter
            4. Marks other channel's target as dirty (atomic 32-bit write)
            5. Chooses next write target using round-robin with skip
            6. Reconfigures the completing channel for its next firing

            Skip rule (from three_bucket_system.md):
            - Round-robin A->B->C->A...
            - Skip protected buckets (BTS_TXP or BTS_PD)
            - If both candidates protected, camera trapped on one bucket
********************************************************************************/
static void handle_half_complete(uint32_t completed_ch)
{
    // Identify which DMA channel completed and which bucket it wrote to
    uint8_t ch_idx = (completed_ch == DMA_CH_A) ? 0 : 1;
    uint8_t completed = ch_target[ch_idx];

    // The other DMA channel has already started via hardware chaining.
    // It is writing to its pre-configured target.
    uint8_t other_idx = 1 - ch_idx;
    uint8_t other_target = ch_target[other_idx];

    // --- Capture current counter state BEFORE incrementing ---
    uint32_t old_counter = cam_counter;
    uint32_t old_frame = old_counter / 2;
    bool old_half_is_upper = (old_counter % 2) == 0;

    // --- Update completed bucket state (single atomic write) ---
    bucket_state[completed] = bucket_make_complete(old_frame, old_half_is_upper);

    // --- Stamp bucket tag into the tag word ---
    // This lets the client verify which frame/half the bucket actually contains.
    ((uint32_t *)bucket[completed])[1] = bucket_state[completed];

    // --- Stamp all 3 camera hints into the tag (4th uint32_t, bytes 12-15) ---
    // Encoding: 2 bits per hint (0=A, 1=B, 2=C, 3=none)
    //   bits 1:0 = cam_hint_next
    //   bits 3:2 = cam_hint_next_next
    //   bits 5:4 = cam_hint_next_next_next
    //   bits 11:6 = transmit hints (set by modcamera.c)
    //   bits 31:12 = reserved (0)
    {
        uint32_t h1 = (cam_hint_next >= 0 && cam_hint_next < 3)
                          ? (uint32_t)cam_hint_next : 3u;
        uint32_t h2 = (cam_hint_next_next >= 0 && cam_hint_next_next < 3)
                          ? (uint32_t)cam_hint_next_next : 3u;
        uint32_t h3 = (cam_hint_next_next_next >= 0 && cam_hint_next_next_next < 3)
                          ? (uint32_t)cam_hint_next_next_next : 3u;
        ((uint32_t *)bucket[completed])[3] = h1 | (h2 << 2) | (h3 << 4);
    }

    // --- Stamp ISR completion timestamp (5th uint32_t, bytes 16-19) ---
    ((uint32_t *)bucket[completed])[4] = time_us_32();

    // --- Advance camera counter ---
    cam_counter++;

    // --- Apply spec protection upgrades upon completion ---
    // Only upgrade if this completion is for a half-frame the TX thread
    // actually cares about (old_counter >= the minimum counter set by TX
    // when it assigned QUEUED/TX_REC). Without this, a stale completion
    // from an earlier frame triggers a spurious upgrade to PD/TXP.
    if (bucket_tx_state[completed] == BUCKET_TX_STATE_TX_REC &&
        old_counter >= bucket_tx_min_counter[completed]) {
        bucket_tx_state[completed] = BUCKET_TX_STATE_TXP;
    } else if (bucket_tx_state[completed] == BUCKET_TX_STATE_QUEUED &&
               old_counter >= bucket_tx_min_counter[completed]) {
        bucket_tx_state[completed] = BUCKET_TX_STATE_PD;
    }

    // --- Decide next write target for this (completing) channel ---
    // This channel will fire AFTER the other channel finishes other_target.
    // Natural round-robin: the bucket after other_target.
    // Skip if protected (TX wants it AND it has valid data).

    // With 3 buckets, the two candidates (excluding other_target) are:
    uint8_t cand_a = (other_target + 1) % 3;  // Natural round-robin next
    uint8_t cand_b = (other_target + 2) % 3;  // Alternative

    uint8_t chosen;
    int8_t hint1 = cam_hint_next;
    int8_t hint2 = cam_hint_next_next;
    int8_t hint3 = cam_hint_next_next_next;

    // Strict queue semantics:
    // - Consume (pop) only a hint that was actually used.
    // - If the head hint is blocked by protection, keep it pending and use
    //   fallback for this ISR tick.
    // - We intentionally do NOT check != completed or != other_target for
    //   hints: they are explicit directives from TX.
    bool used_hint = false;
    if (hint1 >= 0 && hint1 < 3 && !is_protected((uint8_t)hint1)) {
        chosen = (uint8_t)hint1;
        used_hint = true;
        cam_hint_next = hint2;
        cam_hint_next_next = hint3;
        cam_hint_next_next_next = -1;
    } else if (hint1 < 0 &&
               hint2 >= 0 && hint2 < 3 &&
               !is_protected((uint8_t)hint2)) {
        chosen = (uint8_t)hint2;
        used_hint = true;
        cam_hint_next = hint3;
        cam_hint_next_next = -1;
        cam_hint_next_next_next = -1;
    } else if (hint1 < 0 && hint2 < 0 &&
               hint3 >= 0 && hint3 < 3 &&
               !is_protected((uint8_t)hint3)) {
        chosen = (uint8_t)hint3;
        used_hint = true;
        cam_hint_next = -1;
        cam_hint_next_next = -1;
        cam_hint_next_next_next = -1;
    }

    if (!used_hint &&
        (cam_hint_next < 0 || cam_hint_next >= 3) &&
        (cam_hint_next_next < 0 || cam_hint_next_next >= 3) &&
        (cam_hint_next_next_next < 0 || cam_hint_next_next_next >= 3)) {
        cam_hint_next = -1;
        cam_hint_next_next = -1;
        cam_hint_next_next_next = -1;
    }

    if (used_hint) {
        // chosen is already resolved from hints.
    } else if (cand_a != completed && !is_protected(cand_a)) {
        // Natural next is available and not the just-completed bucket
        chosen = cand_a;
    } else if (cand_b != completed && !is_protected(cand_b)) {
        // Alternative is available and not the just-completed bucket
        chosen = cand_b;
    } else if (!is_protected(cand_a)) {
        // cand_a == completed but not protected — allow reuse as last resort
        chosen = cand_a;
    } else if (!is_protected(cand_b)) {
        // cand_b == completed but not protected — allow reuse as last resort
        chosen = cand_b;
    } else {
        // Both protected -- camera is trapped on one bucket.
        // Write to other_target (same bucket as the other channel).
        // By the time this channel fires, the other channel will have finished,
        // so there's no DMA conflict.
        chosen = other_target;
    }

    // --- Update other channel's target state and cemented-next-target ---
    // Keep these paired: when dirty target advances, next target advances too.
    uint32_t new_counter = cam_counter;
    uint32_t new_frame = new_counter / 2;
    bool new_half_is_upper = (new_counter % 2) == 0;
    bucket_state[other_target] = bucket_make_dirty(new_frame, new_half_is_upper);
    ((uint32_t *)bucket[other_target])[1] = bucket_state[other_target];

    bucket_tx_next_cemented[0] = 0;
    bucket_tx_next_cemented[1] = 0;
    bucket_tx_next_cemented[2] = 0;
    bucket_tx_next_cemented[chosen] = 1;

    // --- Configure this channel for its next write ---
    ch_target[ch_idx] = chosen;
    dma_channel_set_write_addr(completed_ch, bucket[chosen] + BUCKET_TAG_SIZE, false);
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
