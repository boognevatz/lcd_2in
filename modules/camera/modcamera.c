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

// Boundary headers for MJPEG streaming (matches Python version in /streamc)
static const char boundary_first[] = "--frame\r\nContent-Type: application/octet-stream\r\n\r\n";
static const char boundary_subsequent[] = "\r\n--frame\r\nContent-Type: application/octet-stream\r\n\r\n";


/********************************************************************************
function:   Determine if a bucket holds (or is receiving) an Upper half-frame.
            Used for TX pair ordering: Upper always goes first.

            With the consolidated bucket_info[] state, both complete and dirty
            (in-progress) states encode the half type in the Half bit (bit 2).
            So we only need: is the bucket valid AND is it upper?
********************************************************************************/
static bool bucket_has_upper(uint8_t b)
{
    uint32_t info = bucket_info[b];
    // Both complete and dirty states encode the half type in the Half bit,
    // so we just need: is valid AND is upper.
    return bucket_is_valid(info) && bucket_half_is_upper(info);
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

    uint32_t t_frame_start = mp_hal_ticks_us();

    while (streaming) {
        mp_handle_pending(true);

        uint32_t t_send_start = mp_hal_ticks_us();

        // --- Send boundary header ---
        const char *boundary;
        size_t boundary_len;
        if (first_frame) {
            boundary = boundary_first;
            boundary_len = sizeof(boundary_first) - 1;
        } else {
            boundary = boundary_subsequent;
            boundary_len = sizeof(boundary_subsequent) - 1;
        }

        mp_uint_t ret = mp_stream_write_exactly(
            socket_obj, boundary, boundary_len, &errcode);
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

        // --- Send second half-frame (76,800 bytes) ---
        ret = mp_stream_write_exactly(
            socket_obj, bucket[tx_second], HALF_FRAME_BYTES, &errcode);
        if (ret == MP_STREAM_ERROR || ret == 0) {
            streaming = false;
            break;
        }

        // Release second bucket
        tx_wants[tx_second] = false;

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
        // Rule: the two buckets that are NOT tx_second (the one just finished)
        // "TX always selects the two buckets that are NOT the bucket TX just
        //  finished sending (i.e., not the Lower-half bucket of the old pair)."
        uint8_t just_finished = tx_second;
        uint8_t cand_a = (just_finished + 1) % 3;
        uint8_t cand_b = (just_finished + 2) % 3;

        // Check if both candidates are complete and from the same frame
        uint32_t info_a = bucket_info[cand_a];
        uint32_t info_b = bucket_info[cand_b];
        bool a_complete = bucket_is_complete(info_a);
        bool b_complete = bucket_is_complete(info_b);
        bool matched = a_complete && b_complete &&
                       (bucket_get_frame(info_a) == bucket_get_frame(info_b));

        if (matched) {
            // Matched pair from same frame -- order: Upper first
            if (bucket_half_is_upper(info_a)) {
                tx_first = cand_a;
                tx_second = cand_b;
            } else {
                tx_first = cand_b;
                tx_second = cand_a;
            }
        } else {
            // No matched pair -- fall back to Upper-first heuristic
            if (bucket_has_upper(cand_a)) {
                tx_first = cand_a;
                tx_second = cand_b;
            } else if (bucket_has_upper(cand_b)) {
                tx_first = cand_b;
                tx_second = cand_a;
            } else {
                tx_first = cand_a;
                tx_second = cand_b;
            }
        }

        // Declare TX intent for the new pair.
        // The ISR will see these and skip protected buckets.
        tx_wants[tx_first] = true;
        tx_wants[tx_second] = true;
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
