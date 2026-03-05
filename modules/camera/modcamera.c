#include "cam.h"
#include <string.h>  // for memset
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
// Followed by the dynamic X-Buckets header line + blank line terminator.
// Content-Length is zero-padded to fixed width (8 digits).
// X-Temperature is a placeholder (update when sensor is wired up).
static const char boundary_prefix_first[] =
    "--frame\r\n"
    "Content-Type: application/octet-stream\r\n"
    "Content-Length: 00153600\r\n"
    "X-Temperature: 36.5\r\n";
static const char boundary_prefix_subsequent[] =
    "\r\n--frame\r\n"
    "Content-Type: application/octet-stream\r\n"
    "Content-Length: 00153600\r\n"
    "X-Temperature: 36.5\r\n";

// Bucket name lookup: index 0->'A', 1->'B', 2->'C'
static const char bucket_name[] = "ABC";

// --- X-Buckets diagnostic headers (fixed-length, filled per frame) ---
//
// Three header lines + blank line terminator, sent as a single buffer.
//
// Line 1: X-Buckets: <tx1>,<tx2>,<A>,<B>,<C>          (current snapshot)
// Line 2: X-Buckets-prev-mid: <A>,<B>,<C>             (prev frame after 1st half sent)
// Line 3: X-Buckets-prev-end: <A>,<B>,<C>             (prev frame after 2nd half sent)
// Line 4: blank line (header terminator)
//
// Each slot is 12 chars fixed:
//   Complete: " F000000000U"  (space + F + 9-digit padded frame + U/L)
//   Dirty:   "~F000000000U"  (tilde + F + 9-digit padded frame + U/L)
//   Empty:   "           -"  (11 spaces + dash)
//
// Frame number: 29 bits -> max 536870911 -> 9 decimal digits.
//
// Temporal separation:
//   Line 1 = snapshot at pair-selection time (coherent with ordering decision)
//   Line 2 = snapshot from previous frame, after first half-frame was sent
//   Line 3 = snapshot from previous frame, after second half-frame was sent
//   Line 1 uses the same snap[] as the ordering decision,
//   so the reported states exactly match what TX observed when choosing order.
//
// Positions (byte offsets in combined 175-byte buffer):
//   Line 1 (55 bytes):
//     11 = tx_first bucket name  (1 char: A/B/C)
//     13 = tx_second bucket name (1 char: A/B/C)
//     15 = bucket A slot         (12 chars)
//     28 = bucket B slot         (12 chars)
//     41 = bucket C slot         (12 chars)
//   Line 2 (59 bytes, starts at offset 55):
//     74 = prev-mid bucket A     (12 chars)
//     87 = prev-mid bucket B     (12 chars)
//    100 = prev-mid bucket C     (12 chars)
//   Line 3 (59 bytes, starts at offset 114):
//    133 = prev-end bucket A     (12 chars)
//    146 = prev-end bucket B     (12 chars)
//    159 = prev-end bucket C     (12 chars)
//   Line 4 (2 bytes): \r\n at 173-174

#define X_HEADER_LEN        175
#define X_HEADER_SLOT_LEN    12
#define X_HEADER_TX1_NAME    11
#define X_HEADER_TX2_NAME    13
#define X_HEADER_A_SLOT      15
#define X_HEADER_B_SLOT      28
#define X_HEADER_C_SLOT      41
#define X_HEADER_MID_A       74
#define X_HEADER_MID_B       87
#define X_HEADER_MID_C      100
#define X_HEADER_END_A      133
#define X_HEADER_END_B      146
#define X_HEADER_END_C      159

static const char x_header_template[] =
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
function:   Stream loop in C -- three-bucket system implementation.

            TX NEVER IDLES. Continuously sends pairs of half-frames.
            At startup, sends A+B (may contain garbage -- receiver discards).
            After each pair, selects the two buckets NOT equal to the one just
            finished sending. Orders them: Upper half first, Lower half second.

            Protection: TX sets tx_wants[b] = true for both pair members at
            selection time, and clears each after finishing its send. The camera
            ISR reads tx_wants[] to skip protected buckets.

            Args: socket object
            Returns: frame count when stream ends (disconnect, error, or
                     KeyboardInterrupt)

            TIMING: Prints per-pair timing breakdown every 50 pairs.
********************************************************************************/
static mp_obj_t camera_stream_loop_c(mp_obj_t socket_obj) {
    bool streaming = true;
    bool first_frame = true;
    uint32_t frame_count = 0;
    int errcode;

    // X-Buckets diagnostic header buffer (filled per frame, sent after prefix)
    char x_header[X_HEADER_LEN];

    // Previous frame snapshots: captured at mid-point and end-point of the
    // previous iteration, reported in the next frame's headers.
    // Initialized to 0 (empty) so first frame shows "           -" for prev.
    uint32_t prev_mid[3] = {0, 0, 0};
    uint32_t prev_end[3] = {0, 0, 0};

    // Timing accumulators (reset every 50 frames)
    uint32_t accum_send_us = 0;
    uint32_t accum_total_us = 0;
    uint32_t accum_count = 0;
    const uint32_t REPORT_INTERVAL = 50;

    // Initial pair: buckets 0 and 1 (A+B), per spec startup rule.
    // At startup no data exists, so order doesn't matter.
    // The spec says: "At startup: TX selects A+B, because camera starts
    //                 writing in A->B order and no written data exists yet."
    uint8_t tx_first = 0;
    uint8_t tx_second = 1;

    // Declare TX intent for startup pair
    tx_wants[tx_first] = true;
    tx_wants[tx_second] = true;

    // Pre-build x_header for the first frame (startup pair A,B).
    // At startup, prev_mid/prev_end are zero (empty) and bucket states
    // reflect initial setup (bucket 0 dirty, others empty).
    memcpy(x_header, x_header_template, X_HEADER_LEN);
    x_header[X_HEADER_TX1_NAME] = bucket_name[tx_first];
    x_header[X_HEADER_TX2_NAME] = bucket_name[tx_second];
    write_bucket_slot(&x_header[X_HEADER_A_SLOT], bucket_state[0]);
    write_bucket_slot(&x_header[X_HEADER_B_SLOT], bucket_state[1]);
    write_bucket_slot(&x_header[X_HEADER_C_SLOT], bucket_state[2]);
    write_bucket_slot(&x_header[X_HEADER_MID_A], prev_mid[0]);
    write_bucket_slot(&x_header[X_HEADER_MID_B], prev_mid[1]);
    write_bucket_slot(&x_header[X_HEADER_MID_C], prev_mid[2]);
    write_bucket_slot(&x_header[X_HEADER_END_A], prev_end[0]);
    write_bucket_slot(&x_header[X_HEADER_END_B], prev_end[1]);
    write_bucket_slot(&x_header[X_HEADER_END_C], prev_end[2]);

    uint32_t t_frame_start = mp_hal_ticks_us();

    while (streaming) {
        mp_handle_pending(true);

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

        // --- Send pre-built X-Buckets diagnostic headers ---
        // x_header was built at pair-selection time (end of previous iteration,
        // or before the loop for the first frame), so Line 1 is coherent
        // with the ordering decision.
        ret = mp_stream_write_exactly(
            socket_obj, x_header, X_HEADER_LEN, &errcode);
        if (ret == MP_STREAM_ERROR) {
            streaming = false;
            break;
        }

        // --- Send first half-frame (76,800 bytes) ---
        // The W5500 driver internally chunks this into 16KB TX buffer writes
        // with SENDOK fast-path between chunks (no redundant buffer polling)
        ret = mp_stream_write_exactly(
            socket_obj, bucket[tx_first], HALF_FRAME_BYTES, &errcode);
        if (ret == MP_STREAM_ERROR || ret == 0) {
            streaming = false;
            break;
        }

        // Release first bucket -- camera can now overwrite it
        tx_wants[tx_first] = false;

        // Snapshot mid-point: bucket states after 1st half sent
        prev_mid[0] = bucket_state[0];
        prev_mid[1] = bucket_state[1];
        prev_mid[2] = bucket_state[2];

        // --- Send second half-frame (76,800 bytes) ---
        ret = mp_stream_write_exactly(
            socket_obj, bucket[tx_second], HALF_FRAME_BYTES, &errcode);
        if (ret == MP_STREAM_ERROR || ret == 0) {
            streaming = false;
            break;
        }

        // Release second bucket
        tx_wants[tx_second] = false;

        // Snapshot end-point: bucket states after 2nd half sent
        prev_end[0] = bucket_state[0];
        prev_end[1] = bucket_state[1];
        prev_end[2] = bucket_state[2];

        first_frame = false;
        frame_count++;

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
        // The two buckets that are NOT just_finished (spec: "TX always selects
        // the two buckets that are NOT the bucket TX just finished sending").
        // The selection is deterministic. TX only controls the ORDER.
        uint8_t just_finished = tx_second;
        uint8_t cand_a = (just_finished + 1) % 3;
        uint8_t cand_b = (just_finished + 2) % 3;

        // Snapshot all 3 states, decide order, set tx_wants -- keep tight.
        // No IRQ disable: the window is ~20 instructions (~130ns at 150 MHz).
        // Occasional ISR between reads is tolerable (receiver handles mismatches).
        uint32_t snap[3];
        snap[0] = bucket_state[0];
        snap[1] = bucket_state[1];
        snap[2] = bucket_state[2];

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
            uint32_t fa = bucket_get_frame(snap[cand_a]);
            uint32_t fb = bucket_get_frame(snap[cand_b]);
            if (fb > fa) {
                tx_first = cand_b;
                tx_second = cand_a;
            } else {
                tx_first = cand_a;
                tx_second = cand_b;
            }
        }

        tx_wants[tx_first] = true;
        tx_wants[tx_second] = true;

        // --- Pre-build x_header for the next iteration ---
        // Line 1 uses snap[] from the ordering decision above,
        // so the reported states are coherent with the ordering decision.
        // Lines 2-3 use prev_mid/prev_end from the sends just completed.
        memcpy(x_header, x_header_template, X_HEADER_LEN);
        x_header[X_HEADER_TX1_NAME] = bucket_name[tx_first];
        x_header[X_HEADER_TX2_NAME] = bucket_name[tx_second];
        write_bucket_slot(&x_header[X_HEADER_A_SLOT], snap[0]);
        write_bucket_slot(&x_header[X_HEADER_B_SLOT], snap[1]);
        write_bucket_slot(&x_header[X_HEADER_C_SLOT], snap[2]);
        write_bucket_slot(&x_header[X_HEADER_MID_A], prev_mid[0]);
        write_bucket_slot(&x_header[X_HEADER_MID_B], prev_mid[1]);
        write_bucket_slot(&x_header[X_HEADER_MID_C], prev_mid[2]);
        write_bucket_slot(&x_header[X_HEADER_END_A], prev_end[0]);
        write_bucket_slot(&x_header[X_HEADER_END_B], prev_end[1]);
        write_bucket_slot(&x_header[X_HEADER_END_C], prev_end[2]);
    }

    // Cleanup: release all protection on exit
    tx_wants[0] = false;
    tx_wants[1] = false;
    tx_wants[2] = false;

    return mp_obj_new_int(frame_count);
}
static MP_DEFINE_CONST_FUN_OBJ_1(camera_stream_loop_c_obj, camera_stream_loop_c);


// Define module globals
static const mp_rom_map_elem_t camera_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init_cam), MP_ROM_PTR(&camera_init_cam_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_i2c_pins), MP_ROM_PTR(&camera_set_i2c_pins_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pwm_pin), MP_ROM_PTR(&camera_set_pwm_pin_obj) },
    { MP_ROM_QSTR(MP_QSTR_start_cam), MP_ROM_PTR(&camera_start_cam_obj) },
    { MP_ROM_QSTR(MP_QSTR_stream_loop_c), MP_ROM_PTR(&camera_stream_loop_c_obj) },
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
