#include "cam.h"
#include <string.h>
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
    "Content-Length: 00153600\r\n";
static const char boundary_prefix_subsequent[] =
    "\r\n--frame\r\n"
    "Content-Type: application/octet-stream\r\n"
    "Content-Length: 00153600\r\n";

// Bucket name lookup: index 0->'A', 1->'B', 2->'C'
static const char bucket_name[] = "ABC";

// --- X-header: dynamic per-frame headers (temperature + bucket diagnostics) ---
//
// Sent as a single buffer after the static boundary prefix.
// Contains X-Temperature, X-Buckets (3 lines), and blank line terminator.
//
// Layout (196 bytes total):
//   Line 0 (21 bytes): X-Temperature: XX.X\r\n
//     15 = temperature value (4 chars: XX.X, zero-padded)
//   Line 1 (55 bytes): X-Buckets: <tx1>,<tx2>,<A>,<B>,<C>\r\n
//     32 = tx_first name  (1 char)
//     34 = tx_second name (1 char)
//     36 = bucket A slot  (12 chars)
//     49 = bucket B slot  (12 chars)
//     62 = bucket C slot  (12 chars)
//   Line 2 (59 bytes): X-Buckets-prev-mid:<A>,<B>,<C>\r\n
//     95 = prev-mid A (12 chars)
//    108 = prev-mid B (12 chars)
//    121 = prev-mid C (12 chars)
//   Line 3 (59 bytes): X-Buckets-prev-end:<A>,<B>,<C>\r\n
//    154 = prev-end A (12 chars)
//    167 = prev-end B (12 chars)
//    180 = prev-end C (12 chars)
//   Line 4 (2 bytes): \r\n (blank line, header terminator)
//
// Each bucket slot is 12 chars fixed:
//   Complete: " F000000000U"  (space + F + 9-digit frame + U/L)
//   Dirty:   "~F000000000U"  (tilde + F + 9-digit frame + U/L)
//   Empty:   "           -"  (11 spaces + dash)
//
// Line 1 uses the same snap[] as the ordering decision,
// so the reported states exactly match what TX observed when choosing order.

#define X_HEADER_LEN        196
#define X_HEADER_SLOT_LEN    12
#define X_HEADER_TEMP        15
#define X_HEADER_TX1_NAME    32
#define X_HEADER_TX2_NAME    34
#define X_HEADER_A_SLOT      36
#define X_HEADER_B_SLOT      49
#define X_HEADER_C_SLOT      62
#define X_HEADER_MID_A       95
#define X_HEADER_MID_B      108
#define X_HEADER_MID_C      121
#define X_HEADER_END_A      154
#define X_HEADER_END_B      167
#define X_HEADER_END_C      180

static const char x_header_template[] =
    "X-Temperature: 36.5\r\n"
    "X-Buckets: A,A,"
    "           -,"
    "           -,"
    "           -"
    "\r\n"
    "X-Buckets-prev-mid:"
    "           -,"
    "           -,"
    "           -"
    "\r\n"
    "X-Buckets-prev-end:"
    "           -,"
    "           -,"
    "           -"
    "\r\n"
    "\r\n";


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
    uint32_t frame = bucket_get_frame(state);
    for (int i = 10; i >= 2; i--) {
        slot[i] = '0' + (frame % 10);
        frame /= 10;
    }
    // Half indicator
    slot[11] = bucket_half_is_upper(state) ? 'U' : 'L';
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
function:   Set MCU temperature from Python.
            camera.set_temperature(365) means 36.5C.
            Called between stream batches, read by C loop when building headers.
********************************************************************************/
static mp_obj_t camera_set_temperature(mp_obj_t val) {
    mcu_temp_x10 = mp_obj_get_int(val);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(camera_set_temperature_obj, camera_set_temperature);


// --- Static stream state (persists across batched stream_loop_c calls) ---

static bool     s_first_frame;
static uint8_t  s_tx_first;
static uint8_t  s_tx_second;
static uint32_t s_prev_mid[3];
static uint32_t s_prev_end[3];
static uint32_t s_frame_count;
static uint32_t s_accum_send_us;
static uint32_t s_accum_total_us;
static uint32_t s_accum_count;
static uint32_t s_t_frame_start;
static char     s_x_header[X_HEADER_LEN];

#define REPORT_INTERVAL 50


/********************************************************************************
function:   Initialize stream state. Call once before the batched stream loop.
            Resets all static state, declares TX intent for startup pair A+B,
            and pre-builds the first x_header.
********************************************************************************/
static mp_obj_t camera_stream_start(void) {
    s_first_frame = true;
    s_frame_count = 0;
    s_tx_first = 0;
    s_tx_second = 1;

    for (int i = 0; i < 3; i++) {
        s_prev_mid[i] = 0;
        s_prev_end[i] = 0;
    }

    s_accum_send_us = 0;
    s_accum_total_us = 0;
    s_accum_count = 0;

    // Declare TX intent for startup pair
    tx_wants[0] = false;
    tx_wants[1] = false;
    tx_wants[2] = false;
    tx_wants[s_tx_first] = true;
    tx_wants[s_tx_second] = true;

    // Pre-build x_header for the first frame
    memcpy(s_x_header, x_header_template, X_HEADER_LEN);
    write_temperature(&s_x_header[X_HEADER_TEMP], mcu_temp_x10);
    s_x_header[X_HEADER_TX1_NAME] = bucket_name[s_tx_first];
    s_x_header[X_HEADER_TX2_NAME] = bucket_name[s_tx_second];
    write_bucket_slot(&s_x_header[X_HEADER_A_SLOT], bucket_state[0]);
    write_bucket_slot(&s_x_header[X_HEADER_B_SLOT], bucket_state[1]);
    write_bucket_slot(&s_x_header[X_HEADER_C_SLOT], bucket_state[2]);
    // prev-mid and prev-end stay as template defaults (empty dashes)

    s_t_frame_start = mp_hal_ticks_us();

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
    write_temperature(&s_x_header[X_HEADER_TEMP], mcu_temp_x10);

    while (streaming && batch_sent < batch_size) {
        mp_handle_pending(true);

        uint32_t t_send_start = mp_hal_ticks_us();

        // --- Send boundary prefix (static) ---
        const char *prefix;
        size_t prefix_len;
        if (s_first_frame) {
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

        // --- Send pre-built x_header ---
        ret = mp_stream_write_exactly(
            socket_obj, s_x_header, X_HEADER_LEN, &errcode);
        if (ret == MP_STREAM_ERROR) {
            streaming = false;
            break;
        }

        // --- Send first half-frame (76,800 bytes) ---
        ret = mp_stream_write_exactly(
            socket_obj, bucket[s_tx_first], HALF_FRAME_BYTES, &errcode);
        if (ret == MP_STREAM_ERROR || ret == 0) {
            streaming = false;
            break;
        }

        // Release first bucket -- camera can now overwrite it
        tx_wants[s_tx_first] = false;

        // Snapshot mid-point: bucket states after 1st half sent
        s_prev_mid[0] = bucket_state[0];
        s_prev_mid[1] = bucket_state[1];
        s_prev_mid[2] = bucket_state[2];

        // --- Send second half-frame (76,800 bytes) ---
        ret = mp_stream_write_exactly(
            socket_obj, bucket[s_tx_second], HALF_FRAME_BYTES, &errcode);
        if (ret == MP_STREAM_ERROR || ret == 0) {
            streaming = false;
            break;
        }

        // Release second bucket
        tx_wants[s_tx_second] = false;

        // Snapshot end-point: bucket states after 2nd half sent
        s_prev_end[0] = bucket_state[0];
        s_prev_end[1] = bucket_state[1];
        s_prev_end[2] = bucket_state[2];

        s_first_frame = false;
        s_frame_count++;
        batch_sent++;

        // --- Timing ---
        uint32_t t_now = mp_hal_ticks_us();
        uint32_t send_us = t_now - t_send_start;
        uint32_t total_us = t_now - s_t_frame_start;

        s_accum_send_us += send_us;
        s_accum_total_us += total_us;
        s_accum_count++;

        if (s_accum_count >= REPORT_INTERVAL) {
            uint32_t avg_send = s_accum_send_us / s_accum_count;
            uint32_t avg_total = s_accum_total_us / s_accum_count;
            uint32_t fps_x10 = (s_accum_count * 10000000UL) / s_accum_total_us;
            mp_printf(&mp_plat_print,
                "STREAM[%lu]: avg send=%lu total=%lu us  fps=%lu.%lu\n",
                (unsigned long)s_frame_count,
                (unsigned long)avg_send,
                (unsigned long)avg_total,
                (unsigned long)(fps_x10 / 10),
                (unsigned long)(fps_x10 % 10));
            s_accum_send_us = 0;
            s_accum_total_us = 0;
            s_accum_count = 0;
        }

        s_t_frame_start = t_now;

        // --- Select next pair ---
        uint8_t just_finished = s_tx_second;
        uint8_t cand_a = (just_finished + 1) % 3;
        uint8_t cand_b = (just_finished + 2) % 3;

        // Snapshot all 3 states, decide order, set tx_wants -- keep tight.
        uint32_t snap[3];
        snap[0] = bucket_state[0];
        snap[1] = bucket_state[1];
        snap[2] = bucket_state[2];

        // Order: Upper half goes first (spec rule).
        bool a_upper = bucket_is_valid(snap[cand_a]) && bucket_half_is_upper(snap[cand_a]);
        bool b_upper = bucket_is_valid(snap[cand_b]) && bucket_half_is_upper(snap[cand_b]);

        if (a_upper && !b_upper) {
            s_tx_first = cand_a;
            s_tx_second = cand_b;
        } else if (b_upper && !a_upper) {
            s_tx_first = cand_b;
            s_tx_second = cand_a;
        } else {
            // Tiebreaker: higher frame number first.
            uint32_t fa = bucket_get_frame(snap[cand_a]);
            uint32_t fb = bucket_get_frame(snap[cand_b]);
            if (fb > fa) {
                s_tx_first = cand_b;
                s_tx_second = cand_a;
            } else {
                s_tx_first = cand_a;
                s_tx_second = cand_b;
            }
        }

        tx_wants[s_tx_first] = true;
        tx_wants[s_tx_second] = true;

        // Pre-build x_header for next frame (uses snap[] from ordering decision)
        memcpy(s_x_header, x_header_template, X_HEADER_LEN);
        write_temperature(&s_x_header[X_HEADER_TEMP], mcu_temp_x10);
        s_x_header[X_HEADER_TX1_NAME] = bucket_name[s_tx_first];
        s_x_header[X_HEADER_TX2_NAME] = bucket_name[s_tx_second];
        write_bucket_slot(&s_x_header[X_HEADER_A_SLOT], snap[0]);
        write_bucket_slot(&s_x_header[X_HEADER_B_SLOT], snap[1]);
        write_bucket_slot(&s_x_header[X_HEADER_C_SLOT], snap[2]);
        write_bucket_slot(&s_x_header[X_HEADER_MID_A], s_prev_mid[0]);
        write_bucket_slot(&s_x_header[X_HEADER_MID_B], s_prev_mid[1]);
        write_bucket_slot(&s_x_header[X_HEADER_MID_C], s_prev_mid[2]);
        write_bucket_slot(&s_x_header[X_HEADER_END_A], s_prev_end[0]);
        write_bucket_slot(&s_x_header[X_HEADER_END_B], s_prev_end[1]);
        write_bucket_slot(&s_x_header[X_HEADER_END_C], s_prev_end[2]);
    }

    // On disconnect, release all protection
    if (!streaming) {
        tx_wants[0] = false;
        tx_wants[1] = false;
        tx_wants[2] = false;
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
