#include "cam.h"
#include <string.h>
#include <stdint.h>
#include "ov5640.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/mphal.h"


// Wrapper for init_cam()
static mp_obj_t camera_init_cam() {
    init_cam();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(camera_init_cam_obj, camera_init_cam);


static mp_obj_t camera_set_i2c_pins(mp_obj_t sda_obj, mp_obj_t scl_obj) {
    set_i2c_pins(mp_obj_get_int(sda_obj), mp_obj_get_int(scl_obj));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(camera_set_i2c_pins_obj, camera_set_i2c_pins);

static mp_obj_t camera_set_pwm_pin(mp_obj_t pwm_obj) {
    set_pwm_pin(mp_obj_get_int(pwm_obj));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(camera_set_pwm_pin_obj, camera_set_pwm_pin);


// Wrapper for start_cam()
static mp_obj_t camera_start_cam() {
    start_cam();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(camera_start_cam_obj, camera_start_cam);


// Wrapper for ov5640_set_data_order()
static mp_obj_t camera_set_data_order(mp_obj_t reverse_obj) {
    ov5640_set_data_order(mp_obj_is_true(reverse_obj));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(camera_set_data_order_obj, camera_set_data_order);

// Python wrapper for cam_set_data_pins
static mp_obj_t camera_set_data_pins(mp_obj_t pins_in) {
    size_t len;
    mp_obj_t *items;
    mp_obj_get_array(pins_in, &len, &items);
    if (len != 8) {
        mp_raise_ValueError(MP_ERROR_TEXT("set_data_pins requires a list of 8 pins"));
    }
    for (size_t i = 0; i < 8; ++i) {
        g_cam_pinmap.d[i] = mp_obj_get_int(items[i]);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(camera_set_data_pins_obj, camera_set_data_pins);

// Python wrapper for cam_set_control_pins (3 positional arguments: vsync, href, pclk)
static mp_obj_t camera_set_control_pins(mp_obj_t vsync, mp_obj_t href, mp_obj_t pclk) {
    g_cam_pinmap.vsync = mp_obj_get_int(vsync);
    g_cam_pinmap.href  = mp_obj_get_int(href);
    g_cam_pinmap.pclk  = mp_obj_get_int(pclk);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(camera_set_control_pins_obj, camera_set_control_pins);

// Python wrapper for cam_set_xclk_pin (1 positional argument: xclk)
static mp_obj_t camera_set_xclk_pin(mp_obj_t xclk) {
    g_cam_pinmap.xclk = mp_obj_get_int(xclk);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(camera_set_xclk_pin_obj, camera_set_xclk_pin);

// Boundary prefix for MJPEG-style multipart streaming.
// Followed by the dynamic x_header buffer (temperature + diagnostics + blank line).
// Content-Length is zero-padded to fixed width (8 digits).
static const char boundary_prefix_first[] =
    "--frame\r\n"
    "Content-Type: application/octet-stream\r\n"
    "Content-Length: 00153624\r\n";
static const char boundary_prefix_subsequent[] =
    "\r\n--frame\r\n"
    "Content-Type: application/octet-stream\r\n"
    "Content-Length: 00153624\r\n";

// Bucket name lookup: index 0->'A', 1->'B', 2->'C'
static const char bucket_name[] = "ABC";

// --- X-header: dynamic per-frame headers (temperature + bucket diagnostics) ---
//
// Sent as individual buffers after the static boundary prefix.
// Each header string is independently managed to simplify adding new ones.

#define X_HEADER_SLOT_LEN    12

static char x_header_temperature[] = "X-Temperature: 00.0\r\n";
#define X_HEADER_TEMP_VAL_OFFSET 15

static char x_header_ext_temp[] = "X-Ext-Temp: -999,-999,-999,-999,-999,-999\r\n";
#define X_HEADER_EXT_T1_OFFSET 12
#define X_HEADER_EXT_T2_OFFSET 17
#define X_HEADER_EXT_T3_OFFSET 22
#define X_HEADER_EXT_T4_OFFSET 27
#define X_HEADER_EXT_T5_OFFSET 32
#define X_HEADER_EXT_T6_OFFSET 37

static char x_header_barometer[] = "X-Barometer: +000C,0.000bar\r\n";
#define X_HEADER_BARO_TEMP_OFFSET 13
#define X_HEADER_BARO_PRESS_OFFSET 19

static char x_header_buckets_prev_mid[] = "X-Buckets-prev-mid:           -,           -,           -\r\n";
#define X_HEADER_MID_A_OFFSET 19
#define X_HEADER_MID_B_OFFSET 32
#define X_HEADER_MID_C_OFFSET 45
static char x_header_buckets_prev_mid_time[] = "X-Buckets-prev-mid-time: 0000000000000\r\n";
#define X_HEADER_MID_TIME_OFFSET 25

static char x_header_buckets_prev_end[] = "X-Buckets-prev-end:           -,           -,           -\r\n";
#define X_HEADER_END_A_OFFSET 19
#define X_HEADER_END_B_OFFSET 32
#define X_HEADER_END_C_OFFSET 45
static char x_header_buckets_prev_end_time[] = "X-Buckets-prev-end-time: 0000000000000\r\n";
#define X_HEADER_END_TIME_OFFSET 25

static char x_header_buckets[] = "X-Buckets: A,A,           -,           -,           -\r\n";
#define X_HEADER_TX1_OFFSET 11
#define X_HEADER_TX2_OFFSET 13
#define X_HEADER_BUCKET_A_OFFSET 15
#define X_HEADER_BUCKET_B_OFFSET 28
#define X_HEADER_BUCKET_C_OFFSET 41
static char x_header_buckets_time[] = "X-Buckets-time: 0000000000000\r\n";
#define X_HEADER_BUCKETS_TIME_OFFSET 16

#define X_HEADER_TX_SLOT_LEN 7
static char x_header_buckets_tx[] = "X-Buckets-tx:   FREE ,   FREE ,   FREE \r\n";
#define X_HEADER_TX_A_OFFSET 15
#define X_HEADER_TX_B_OFFSET 23
#define X_HEADER_TX_C_OFFSET 31

static const char x_header_terminator[] = "\r\n";


/********************************************************************************
function:   Write a 12-char fixed-width bucket state into a slot buffer.
            Pure char[] manipulation, no snprintf.

            Complete: " F000000004U"  (space prefix)
            Dirty:   "~F000000004U"  (tilde prefix)
            Empty:   "           -"  (11 spaces + dash)
********************************************************************************/
static void write_bucket_slot(char *slot, uint32_t state)
{
    if (!bucket_is_valid(state)) {
        memcpy(slot, "           -", X_HEADER_SLOT_LEN);
        return;
    }
    // Dirty or complete prefix
    slot[0] = bucket_is_dirty(state) ? '~' : ' ';
    slot[1] = 'F';
    // 9-digit zero-padded frame number (positions 2-10)
    uint32_t frame = bucket_get_halfframe(state) / 2;
    for (int i = 10; i >= 2; i--) {
        slot[i] = '0' + (frame % 10);
        frame /= 10;
    }
    // Half indicator
    slot[11] = bucket_half_is_upper(state) ? 'U' : 'L';
}

static void write_u32_grouped(char *slot, uint32_t value)
{
    char digits[10];
    int len = 0;
    if (value == 0) {
        digits[len++] = '0';
    } else {
        while (value > 0 && len < (int)sizeof(digits)) {
            digits[len++] = '0' + (value % 10);
            value /= 10;
        }
    }

    char out[13];
    int out_i = 12;
    int group = 0;
    for (int i = 0; i < 13; i++) {
        out[i] = ' ';
    }
    for (int i = 0; i < len && out_i >= 0; i++) {
        if (group == 3) {
            out[out_i--] = ' ';
            group = 0;
        }
        out[out_i--] = digits[i];
        group++;
    }

    memcpy(slot, out, sizeof(out));
}

static void write_tx_state_slot(char *slot, uint8_t state)
{
    const char *label = "FREE";
    switch (state) {
        case BUCKET_TX_STATE_TXP:
            label = "TXP";
            break;
        case BUCKET_TX_STATE_PD:
            label = "PD";
            break;
        case BUCKET_TX_STATE_TX_REC:
            label = "TX_REC";
            break;
        case BUCKET_TX_STATE_TX:
            label = "TX";
            break;
        case BUCKET_TX_STATE_QUEUED:
            label = "QUEUED";
            break;
        default:
            label = "FREE";
            break;
    }
    size_t len = strlen(label);
    memset(slot, ' ', X_HEADER_TX_SLOT_LEN);
    memcpy(slot, label, len < X_HEADER_TX_SLOT_LEN ? len : X_HEADER_TX_SLOT_LEN);
}

static inline uint32_t pack_tx_states(void)
{
    return (uint32_t)(bucket_tx_state[0] & 0xff) |
           ((uint32_t)(bucket_tx_state[1] & 0xff) << 8) |
           ((uint32_t)(bucket_tx_state[2] & 0xff) << 16);
}


/********************************************************************************
function:   Write 4-char fixed-width temperature (XX.X) into x_header.
            Input: temp_x10 = temperature * 10  (365 = 36.5C)
            Output: "36.5" at slot, zero-padded, clamped to 00.0-99.9
********************************************************************************/
static void write_temperature(char *slot, int32_t temp_x10)
{
    if (temp_x10 < 0) temp_x10 = 0;
    if (temp_x10 > 999) temp_x10 = 999;
    uint32_t t = (uint32_t)temp_x10;
    slot[3] = '0' + (t % 10);  // tenths
    t /= 10;
    slot[2] = '.';
    slot[1] = '0' + (t % 10);  // ones
    t /= 10;
    slot[0] = '0' + (t % 10);  // tens
}


/********************************************************************************
function:   Write 4-char fixed-width temperature integer into x_header.
            Input: t = temperature in degrees (e.g., 34, -10, or -999 for missing)
            Output: "+034", "-010", or "-999" at slot
********************************************************************************/
static void write_ext_temp_val(char *slot, int32_t t)
{
    if (t < -999) t = -999;
    if (t > 9999) t = 9999;
    
    if (t < 0) {
        t = -t;
        slot[0] = '-';
    } else {
        slot[0] = '+';
    }
    
    // 3-digit zero-padded absolute value
    uint32_t abs_t = (uint32_t)t;
    slot[3] = '0' + (abs_t % 10);
    abs_t /= 10;
    slot[2] = '0' + (abs_t % 10);
    abs_t /= 10;
    slot[1] = '0' + (abs_t % 10);
}


/********************************************************************************
function:   Write 5-char fixed-width pressure float into x_header.
            Input: p = pressure in bar * 1000 (e.g., 1013 for 1.013 bar, 0 for 0.000 bar)
            Output: "1.013", "0.000" at slot
********************************************************************************/
static void write_baro_press_val(char *slot, int32_t p)
{
    if (p < 0) p = 0;
    if (p > 9999) p = 9999;
    
    // 4-digit zero-padded value with decimal
    uint32_t abs_p = (uint32_t)p;
    slot[4] = '0' + (abs_p % 10);
    abs_p /= 10;
    slot[3] = '0' + (abs_p % 10);
    abs_p /= 10;
    slot[2] = '0' + (abs_p % 10);
    abs_p /= 10;
    slot[1] = '.';
    slot[0] = '0' + (abs_p % 10);
}


/********************************************************************************
function:   Set MCU temperature from Python.
            camera.set_temperature(365) means 36.5C.
            Called between stream batches, read by C loop when building headers.
********************************************************************************/
static mp_obj_t camera_set_temperature(mp_obj_t val) {
    mcu_temp_x10 = mp_obj_get_int(val);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(camera_set_temperature_obj, camera_set_temperature);


/********************************************************************************
function:   Set 6 external temperatures from Python.
            camera.set_ext_temperatures(t1, t2, t3, t4, t5, t6)
            Values: integers (-999 to 9999), -999 means missing
********************************************************************************/
static mp_obj_t camera_set_ext_temperatures(size_t n_args, const mp_obj_t *args) {
    write_ext_temp_val(&x_header_ext_temp[X_HEADER_EXT_T1_OFFSET], mp_obj_get_int(args[0]));
    write_ext_temp_val(&x_header_ext_temp[X_HEADER_EXT_T2_OFFSET], mp_obj_get_int(args[1]));
    write_ext_temp_val(&x_header_ext_temp[X_HEADER_EXT_T3_OFFSET], mp_obj_get_int(args[2]));
    write_ext_temp_val(&x_header_ext_temp[X_HEADER_EXT_T4_OFFSET], mp_obj_get_int(args[3]));
    write_ext_temp_val(&x_header_ext_temp[X_HEADER_EXT_T5_OFFSET], mp_obj_get_int(args[4]));
    write_ext_temp_val(&x_header_ext_temp[X_HEADER_EXT_T6_OFFSET], mp_obj_get_int(args[5]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(camera_set_ext_temperatures_obj, 6, 6, camera_set_ext_temperatures);


/********************************************************************************
function:   Set barometer data from Python.
            camera.set_barometer(temp, pressure)
            temp: integer (-999 to 9999), -999 means missing
            pressure: integer (bar * 1000), e.g., 1013 for 1.013 bar
********************************************************************************/
static mp_obj_t camera_set_barometer(mp_obj_t temp, mp_obj_t press) {
    write_ext_temp_val(&x_header_barometer[X_HEADER_BARO_TEMP_OFFSET], mp_obj_get_int(temp));
    int32_t p = mp_obj_get_int(press);
    if (mp_obj_get_int(temp) == -999) {
        memcpy(&x_header_barometer[X_HEADER_BARO_PRESS_OFFSET], "-.---", 5);
    } else {
        write_baro_press_val(&x_header_barometer[X_HEADER_BARO_PRESS_OFFSET], p);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(camera_set_barometer_obj, camera_set_barometer);


// --- Static stream state (persists across batched stream_loop_c calls) ---

static bool     first_frame;
static uint8_t  tx_first;
static uint8_t  tx_second;
static uint32_t prev_mid[3];
static uint32_t prev_end[3];
static uint32_t prev_mid_time_us;
static uint32_t prev_end_time_us;
static uint32_t buckets_time_us;
static uint32_t last_sent_half_frame;
static int8_t   wait_upper_bucket;
static uint32_t frame_count;
static uint32_t accum_send_us;
static uint32_t accum_total_us;
static uint32_t accum_count;
static uint32_t t_frame_start;

#define REPORT_INTERVAL 50

// 50% transfer mark: at this byte offset within a half-frame send we
// release the *current* bucket's protection and, if the two OTHER buckets
// form the pattern "upper half done + lower half in progress", protect
// the upper-half bucket so the camera cannot accidentally overwrite it.
#define FIFTY_PERCENT_BYTES (TAGGED_HALF_FRAME_BYTES / 2)  // ~38,406

// ******************************************************************************
// function:   Apply the 50%-mark protection swap.
//
//             sending_bucket : the bucket currently being TX'd (to release).
//             The two "other" buckets are (sending_bucket+1)%3 and
//             (sending_bucket+2)%3.
//
//             Step 1 — Release sending_bucket from protected state.
//             Step 2 — Inspect the other two:
//                      if  one is complete+upper  AND  the other is dirty+lower
//                      → mark the completed-upper one as PD (protected).
// ******************************************************************************
static void apply_50pct_protection(uint8_t sending_bucket)
{
    // Step 1: un-protect the bucket we are currently sending.
    bucket_tx_state[sending_bucket] = BUCKET_TX_STATE_FREE;

    // Step 2: look at the other two buckets.
    uint8_t o1 = (sending_bucket + 1) % 3;
    uint8_t o2 = (sending_bucket + 2) % 3;

    uint32_t s1 = bucket_state[o1];
    uint32_t s2 = bucket_state[o2];

    bool s1_upper_done  = bucket_is_complete(s1) && bucket_half_is_upper(s1);
    bool s1_lower_dirty = bucket_is_dirty(s1)    && !bucket_half_is_upper(s1);
    bool s2_upper_done  = bucket_is_complete(s2) && bucket_half_is_upper(s2);
    bool s2_lower_dirty = bucket_is_dirty(s2)    && !bucket_half_is_upper(s2);

    if (s1_upper_done && s2_lower_dirty) {
        // o1 has the completed upper half — protect it
        bucket_tx_state[o1] = BUCKET_TX_STATE_PD;
    } else if (s2_upper_done && s1_lower_dirty) {
        // o2 has the completed upper half — protect it
        bucket_tx_state[o2] = BUCKET_TX_STATE_PD;
    }
}


/********************************************************************************
function:   Initialize stream state. Call once before the batched stream loop.
            Resets all static state, declares TX intent for startup pair A+B,
            and pre-builds the first x_header.
********************************************************************************/
static mp_obj_t camera_stream_start(void) {
    first_frame = true;
    frame_count = 0;
    tx_first = 0;
    tx_second = 1;

    for (int i = 0; i < 3; i++) {
        prev_mid[i] = 0;
        prev_end[i] = 0;
    }
    prev_mid_time_us = 0;
    prev_end_time_us = 0;
    buckets_time_us = 0;

    accum_send_us = 0;
    accum_total_us = 0;
    accum_count = 0;
    last_sent_half_frame = 0;
    wait_upper_bucket = -1;

    // Declare TX intent for startup pair
    bucket_tx_state[0] = BUCKET_TX_STATE_FREE;
    bucket_tx_state[1] = BUCKET_TX_STATE_FREE;
    bucket_tx_state[2] = BUCKET_TX_STATE_FREE;
    
    // At startup, no data exists yet, so both are QUEUED (waiting for camera)
    // When camera catches up, ISR will upgrade them to TX_REC, then TXP/PD.
    bucket_tx_state[tx_first] = BUCKET_TX_STATE_QUEUED;
    bucket_tx_state[tx_second] = BUCKET_TX_STATE_QUEUED;
    cam_hint_next = tx_first;
    cam_hint_next_next = tx_second;

    // Pre-build x_headers for the first frame
    write_temperature(&x_header_temperature[X_HEADER_TEMP_VAL_OFFSET], mcu_temp_x10);
    x_header_buckets[X_HEADER_TX1_OFFSET] = bucket_name[tx_first];
    x_header_buckets[X_HEADER_TX2_OFFSET] = bucket_name[tx_second];
    write_bucket_slot(&x_header_buckets[X_HEADER_BUCKET_A_OFFSET], bucket_state[0]);
    write_bucket_slot(&x_header_buckets[X_HEADER_BUCKET_B_OFFSET], bucket_state[1]);
    write_bucket_slot(&x_header_buckets[X_HEADER_BUCKET_C_OFFSET], bucket_state[2]);
    // prev-mid and prev-end: explicitly zero out with empty dashes
    write_bucket_slot(&x_header_buckets_prev_mid[X_HEADER_MID_A_OFFSET], 0);
    write_bucket_slot(&x_header_buckets_prev_mid[X_HEADER_MID_B_OFFSET], 0);
    write_bucket_slot(&x_header_buckets_prev_mid[X_HEADER_MID_C_OFFSET], 0);
    write_bucket_slot(&x_header_buckets_prev_end[X_HEADER_END_A_OFFSET], 0);
    write_bucket_slot(&x_header_buckets_prev_end[X_HEADER_END_B_OFFSET], 0);
    write_bucket_slot(&x_header_buckets_prev_end[X_HEADER_END_C_OFFSET], 0);
    write_u32_grouped(&x_header_buckets_prev_mid_time[X_HEADER_MID_TIME_OFFSET], prev_mid_time_us);
    write_u32_grouped(&x_header_buckets_prev_end_time[X_HEADER_END_TIME_OFFSET], prev_end_time_us);
    write_u32_grouped(&x_header_buckets_time[X_HEADER_BUCKETS_TIME_OFFSET], buckets_time_us);
    write_tx_state_slot(&x_header_buckets_tx[X_HEADER_TX_A_OFFSET], bucket_tx_state[0]);
    write_tx_state_slot(&x_header_buckets_tx[X_HEADER_TX_B_OFFSET], bucket_tx_state[1]);
    write_tx_state_slot(&x_header_buckets_tx[X_HEADER_TX_C_OFFSET], bucket_tx_state[2]);

    t_frame_start = mp_hal_ticks_us();

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(camera_stream_start_obj, camera_stream_start);


/********************************************************************************
function:   Batched stream loop -- sends up to batch_size frames, then returns.

            TX NEVER IDLES within a batch. Between batches, Python gets control
            to read sensors, update temperature, etc. (~50us gap, invisible).

            Protection and ordering state persist across batches via statics.
            Call camera.stream_start() once before the first batch.

            Args: socket object, batch size (int)
            Returns: number of frames sent in this batch.
                     Less than batch_size means disconnect/error.
********************************************************************************/
static mp_obj_t camera_stream_loop_c(mp_obj_t socket_obj, mp_obj_t batch_obj) {
    uint32_t batch_size = mp_obj_get_int(batch_obj);
    uint32_t batch_sent = 0;
    bool streaming = true;
    int errcode;

    // Patch temperature into pre-built x_header from Python's latest reading
    write_temperature(&x_header_temperature[X_HEADER_TEMP_VAL_OFFSET], mcu_temp_x10);

    while (streaming && batch_sent < batch_size) {
        mp_handle_pending(true);

        if (wait_upper_bucket >= 0) {
            while (true) {
                uint32_t s = bucket_state[wait_upper_bucket];
                if (bucket_is_dirty(s) && bucket_half_is_upper(s) &&
                    bucket_get_halfframe(s) > last_sent_half_frame) {
                    break;
                }
                mp_handle_pending(true);
            }
            wait_upper_bucket = -1;
        }

        uint32_t t_send_start = mp_hal_ticks_us();

        // --- Send boundary prefix (static) ---
        const char *prefix;
        size_t prefix_len;
        if (first_frame) {
            prefix = boundary_prefix_first;
            prefix_len = sizeof(boundary_prefix_first) - 1;
        } else {
            prefix = boundary_prefix_subsequent;
            prefix_len = sizeof(boundary_prefix_subsequent) - 1;
        }

        mp_uint_t ret = mp_stream_write_exactly(
            socket_obj, prefix, prefix_len, &errcode);
        if (ret == MP_STREAM_ERROR) {
            streaming = false;
            break;
        }

        // --- Send pre-built X-headers individually ---
        ret = mp_stream_write_exactly(socket_obj, x_header_temperature, sizeof(x_header_temperature) - 1, &errcode);
        if (ret == MP_STREAM_ERROR) { streaming = false; break; }

        ret = mp_stream_write_exactly(socket_obj, x_header_ext_temp, sizeof(x_header_ext_temp) - 1, &errcode);
        if (ret == MP_STREAM_ERROR) { streaming = false; break; }

        ret = mp_stream_write_exactly(socket_obj, x_header_barometer, sizeof(x_header_barometer) - 1, &errcode);
        if (ret == MP_STREAM_ERROR) { streaming = false; break; }

        ret = mp_stream_write_exactly(socket_obj, x_header_buckets_prev_mid, sizeof(x_header_buckets_prev_mid) - 1, &errcode);
        if (ret == MP_STREAM_ERROR) { streaming = false; break; }

        ret = mp_stream_write_exactly(socket_obj, x_header_buckets_prev_mid_time, sizeof(x_header_buckets_prev_mid_time) - 1, &errcode);
        if (ret == MP_STREAM_ERROR) { streaming = false; break; }

        ret = mp_stream_write_exactly(socket_obj, x_header_buckets_prev_end, sizeof(x_header_buckets_prev_end) - 1, &errcode);
        if (ret == MP_STREAM_ERROR) { streaming = false; break; }

        ret = mp_stream_write_exactly(socket_obj, x_header_buckets_prev_end_time, sizeof(x_header_buckets_prev_end_time) - 1, &errcode);
        if (ret == MP_STREAM_ERROR) { streaming = false; break; }

        ret = mp_stream_write_exactly(socket_obj, x_header_buckets, sizeof(x_header_buckets) - 1, &errcode);
        if (ret == MP_STREAM_ERROR) { streaming = false; break; }

        ret = mp_stream_write_exactly(socket_obj, x_header_buckets_time, sizeof(x_header_buckets_time) - 1, &errcode);
        if (ret == MP_STREAM_ERROR) { streaming = false; break; }

        ret = mp_stream_write_exactly(socket_obj, x_header_buckets_tx, sizeof(x_header_buckets_tx) - 1, &errcode);
        if (ret == MP_STREAM_ERROR) { streaming = false; break; }

        ret = mp_stream_write_exactly(socket_obj, x_header_terminator, sizeof(x_header_terminator) - 1, &errcode);
        if (ret == MP_STREAM_ERROR) { streaming = false; break; }

        // --- Send first half-frame (12-byte tag + 76,800 bytes) ---
        uint32_t upper_time_us = mp_hal_ticks_us();
        uint32_t upper_tag = bucket_state[tx_first];
        uint32_t *upper_words = (uint32_t *)bucket[tx_first];
        uint32_t upper_tx_states = pack_tx_states();
        upper_words[0] = upper_time_us;
        upper_words[1] = upper_tag;
        upper_words[2] = upper_tx_states;
        ret = mp_stream_write_exactly(
            socket_obj, bucket[tx_first], TAGGED_HALF_FRAME_BYTES, &errcode);
        if (ret == MP_STREAM_ERROR || ret == 0) {
            streaming = false;
            break;
        }
        last_sent_half_frame = bucket_get_halfframe(upper_tag);

        // Release first bucket -- camera can now overwrite it
        bucket_tx_state[tx_first] = BUCKET_TX_STATE_FREE;

        // Upgrade second bucket: it is no longer the queued partner (QUEUED/PD),
        // it is now actively being sent. If it was QUEUED (still writing), it
        // becomes TX_REC. If it was PD (camera done), it becomes TXP.
        if (bucket_tx_state[tx_second] == BUCKET_TX_STATE_QUEUED) {
            bucket_tx_state[tx_second] = BUCKET_TX_STATE_TX_REC;
        } else if (bucket_tx_state[tx_second] == BUCKET_TX_STATE_PD) {
            bucket_tx_state[tx_second] = BUCKET_TX_STATE_TXP;
        }

        // Snapshot mid-point: bucket states after 1st half sent
        prev_mid[0] = bucket_state[0];
        prev_mid[1] = bucket_state[1];
        prev_mid[2] = bucket_state[2];
        prev_mid_time_us = mp_hal_ticks_us();

        // --- Send second half-frame (12-byte tag + 76,800 bytes) ---
        // Split at 50%: send first 50%, apply protection swap, send rest.
        uint32_t lower_time_us = mp_hal_ticks_us();
        uint32_t lower_tag = bucket_state[tx_second];
        uint32_t *lower_words = (uint32_t *)bucket[tx_second];
        uint32_t lower_tx_states = pack_tx_states();
        lower_words[0] = lower_time_us;
        lower_words[1] = lower_tag;
        lower_words[2] = lower_tx_states;

        // Phase 1: send first 50% of the half-frame
        ret = mp_stream_write_exactly(
            socket_obj, bucket[tx_second], FIFTY_PERCENT_BYTES, &errcode);
        if (ret == MP_STREAM_ERROR || ret == 0) {
            streaming = false;
            break;
        }

        // --- 50% mark: release current bucket, conditionally protect upper ---
        apply_50pct_protection(tx_second);

        // Phase 2: send remaining 50%
        ret = mp_stream_write_exactly(
            socket_obj,
            bucket[tx_second] + FIFTY_PERCENT_BYTES,
            TAGGED_HALF_FRAME_BYTES - FIFTY_PERCENT_BYTES,
            &errcode);
        if (ret == MP_STREAM_ERROR || ret == 0) {
            streaming = false;
            break;
        }
        last_sent_half_frame = bucket_get_halfframe(lower_tag);

        // Snapshot end-point: bucket states after 2nd half sent
        prev_end[0] = bucket_state[0];
        prev_end[1] = bucket_state[1];
        prev_end[2] = bucket_state[2];
        prev_end_time_us = mp_hal_ticks_us();

        first_frame = false;
        frame_count++;
        batch_sent++;

        // --- Timing ---
        uint32_t t_now = mp_hal_ticks_us();
        uint32_t send_us = t_now - t_send_start;
        uint32_t total_us = t_now - t_frame_start;

        accum_send_us += send_us;
        accum_total_us += total_us;
        accum_count++;

        if (accum_count >= REPORT_INTERVAL) {
            uint32_t avg_send = accum_send_us / accum_count;
            uint32_t avg_total = accum_total_us / accum_count;
            uint32_t fps_x10 = (accum_count * 10000000UL) / accum_total_us;
            mp_printf(&mp_plat_print,
                "STREAM[%lu]: avg send=%lu total=%lu us  fps=%lu.%lu\n",
                (unsigned long)frame_count,
                (unsigned long)avg_send,
                (unsigned long)avg_total,
                (unsigned long)(fps_x10 / 10),
                (unsigned long)(fps_x10 % 10));
            accum_send_us = 0;
            accum_total_us = 0;
            accum_count = 0;
        }

        t_frame_start = t_now;

        // --- Select next pair ---
        uint8_t just_finished = tx_second;
        uint8_t cand_a = (just_finished + 1) % 3;
        uint8_t cand_b = (just_finished + 2) % 3;

        // Snapshot all 3 states, decide order, set tx_wants -- keep tight.
        uint32_t snap[3];
        snap[0] = bucket_state[0];
        snap[1] = bucket_state[1];
        snap[2] = bucket_state[2];
        buckets_time_us = mp_hal_ticks_us();

        // Order: Upper half goes first (spec rule).
        bool a_upper = bucket_is_valid(snap[cand_a]) && bucket_half_is_upper(snap[cand_a]);
        bool b_upper = bucket_is_valid(snap[cand_b]) && bucket_half_is_upper(snap[cand_b]);

        if (a_upper && !b_upper) {
            tx_first = cand_a;
            tx_second = cand_b;
        } else if (b_upper && !a_upper) {
            tx_first = cand_b;
            tx_second = cand_a;
        } else {
            // Tiebreaker: higher frame number first.
            uint32_t fa = bucket_get_halfframe(snap[cand_a]);
            uint32_t fb = bucket_get_halfframe(snap[cand_b]);
            if (fb > fa) {
                tx_first = cand_b;
                tx_second = cand_a;
            } else {
                tx_first = cand_a;
                tx_second = cand_b;
            }
        }

        cam_hint_next = tx_first;
        cam_hint_next_next = tx_second;

        uint32_t hfa = bucket_get_halfframe(snap[cand_a]);
        uint32_t hfb = bucket_get_halfframe(snap[cand_b]);
        bool a_dirty_lower = bucket_is_dirty(snap[cand_a]) && !bucket_half_is_upper(snap[cand_a]);
        bool b_dirty_lower = bucket_is_dirty(snap[cand_b]) && !bucket_half_is_upper(snap[cand_b]);
        if (a_dirty_lower && hfb <= last_sent_half_frame) {
            wait_upper_bucket = cand_b;
        } else if (b_dirty_lower && hfa <= last_sent_half_frame) {
            wait_upper_bucket = cand_a;
        }
        
        if (bucket_is_dirty(snap[tx_first])) {
            bucket_tx_state[tx_first] = BUCKET_TX_STATE_TX_REC;
        } else if (bucket_is_complete(snap[tx_first])) {
            bucket_tx_state[tx_first] = BUCKET_TX_STATE_TXP;
        } else {
            bucket_tx_state[tx_first] = BUCKET_TX_STATE_TX;
        }

        if (bucket_is_dirty(snap[tx_second])) {
            bucket_tx_state[tx_second] = BUCKET_TX_STATE_QUEUED;
        } else if (bucket_is_complete(snap[tx_second])) {
            bucket_tx_state[tx_second] = BUCKET_TX_STATE_PD;
        } else {
            bucket_tx_state[tx_second] = BUCKET_TX_STATE_QUEUED; // fallback
        }
        
        // Release the departing bucket that was just finished
        bucket_tx_state[just_finished] = BUCKET_TX_STATE_FREE;

        // Pre-build x_headers for next frame (uses snap[] from ordering decision)
        write_temperature(&x_header_temperature[X_HEADER_TEMP_VAL_OFFSET], mcu_temp_x10);
        x_header_buckets[X_HEADER_TX1_OFFSET] = bucket_name[tx_first];
        x_header_buckets[X_HEADER_TX2_OFFSET] = bucket_name[tx_second];
        write_bucket_slot(&x_header_buckets[X_HEADER_BUCKET_A_OFFSET], snap[0]);
        write_bucket_slot(&x_header_buckets[X_HEADER_BUCKET_B_OFFSET], snap[1]);
        write_bucket_slot(&x_header_buckets[X_HEADER_BUCKET_C_OFFSET], snap[2]);
        write_bucket_slot(&x_header_buckets_prev_mid[X_HEADER_MID_A_OFFSET], prev_mid[0]);
        write_bucket_slot(&x_header_buckets_prev_mid[X_HEADER_MID_B_OFFSET], prev_mid[1]);
        write_bucket_slot(&x_header_buckets_prev_mid[X_HEADER_MID_C_OFFSET], prev_mid[2]);
        write_bucket_slot(&x_header_buckets_prev_end[X_HEADER_END_A_OFFSET], prev_end[0]);
        write_bucket_slot(&x_header_buckets_prev_end[X_HEADER_END_B_OFFSET], prev_end[1]);
        write_bucket_slot(&x_header_buckets_prev_end[X_HEADER_END_C_OFFSET], prev_end[2]);
        write_u32_grouped(&x_header_buckets_prev_mid_time[X_HEADER_MID_TIME_OFFSET], prev_mid_time_us);
        write_u32_grouped(&x_header_buckets_prev_end_time[X_HEADER_END_TIME_OFFSET], prev_end_time_us);
        write_u32_grouped(&x_header_buckets_time[X_HEADER_BUCKETS_TIME_OFFSET], buckets_time_us);
        write_tx_state_slot(&x_header_buckets_tx[X_HEADER_TX_A_OFFSET], bucket_tx_state[0]);
        write_tx_state_slot(&x_header_buckets_tx[X_HEADER_TX_B_OFFSET], bucket_tx_state[1]);
        write_tx_state_slot(&x_header_buckets_tx[X_HEADER_TX_C_OFFSET], bucket_tx_state[2]);
    }

    // On disconnect, release all protection
    if (!streaming) {
        bucket_tx_state[0] = BUCKET_TX_STATE_FREE;
        bucket_tx_state[1] = BUCKET_TX_STATE_FREE;
        bucket_tx_state[2] = BUCKET_TX_STATE_FREE;
        cam_hint_next = -1;
        cam_hint_next_next = -1;
    }

    return mp_obj_new_int(batch_sent);
}
static MP_DEFINE_CONST_FUN_OBJ_2(camera_stream_loop_c_obj, camera_stream_loop_c);


// Define module globals
static const mp_rom_map_elem_t camera_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init_cam), MP_ROM_PTR(&camera_init_cam_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_i2c_pins), MP_ROM_PTR(&camera_set_i2c_pins_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pwm_pin), MP_ROM_PTR(&camera_set_pwm_pin_obj) },
    { MP_ROM_QSTR(MP_QSTR_start_cam), MP_ROM_PTR(&camera_start_cam_obj) },
    { MP_ROM_QSTR(MP_QSTR_stream_start), MP_ROM_PTR(&camera_stream_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stream_loop_c), MP_ROM_PTR(&camera_stream_loop_c_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_temperature), MP_ROM_PTR(&camera_set_temperature_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_ext_temperatures), MP_ROM_PTR(&camera_set_ext_temperatures_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_barometer), MP_ROM_PTR(&camera_set_barometer_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_data_order), MP_ROM_PTR(&camera_set_data_order_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_data_pins), MP_ROM_PTR(&camera_set_data_pins_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_control_pins), MP_ROM_PTR(&camera_set_control_pins_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_xclk_pin), MP_ROM_PTR(&camera_set_xclk_pin_obj) },
};
static MP_DEFINE_CONST_DICT(camera_module_globals, camera_module_globals_table);

// Define module
const mp_obj_module_t camera_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&camera_module_globals,
};

// Register the module to make it available in Python
MP_REGISTER_MODULE(MP_QSTR_camera, camera_module);
