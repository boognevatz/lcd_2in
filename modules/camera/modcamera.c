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


static mp_obj_t camera_frame(void) {
    // Return a 2-tuple: (first_half_bytearray, second_half_bytearray)
    mp_obj_t halves[2] = {
        mp_obj_new_bytearray_by_ref(HALF_FRAME_BYTES, bucket[frame_first_idx]),
        mp_obj_new_bytearray_by_ref(HALF_FRAME_BYTES, bucket[frame_second_idx]),
    };
    return mp_obj_new_tuple(2, halves);
}
static MP_DEFINE_CONST_FUN_OBJ_0(camera_frame_obj, camera_frame);


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

static mp_obj_t cam_is_frame_ready(void) {
    return mp_obj_new_bool(frame_ready);
}
static MP_DEFINE_CONST_FUN_OBJ_0(cam_is_frame_ready_obj, cam_is_frame_ready);

static mp_obj_t camera_send_frame_over_eth(mp_obj_t callback) {
    if (!frame_ready) {
        return mp_const_none;
    }
    cam_start_read();
    // Pass a 2-tuple of half-frame bytearrays to the callback
    mp_obj_t halves[2] = {
        mp_obj_new_bytearray_by_ref(HALF_FRAME_BYTES, bucket[frame_first_idx]),
        mp_obj_new_bytearray_by_ref(HALF_FRAME_BYTES, bucket[frame_second_idx]),
    };
    mp_call_function_1(callback, mp_obj_new_tuple(2, halves));
    cam_end_read();
    frame_ready = false;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(camera_send_frame_over_eth_obj, camera_send_frame_over_eth);

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

// Send frame data directly to socket from C (3-bucket half-frame)
// Args: socket object, is_first_frame (bool)
// Returns: True on success, False on failure, None if no frame ready
static mp_obj_t camera_send_frame_data_c(mp_obj_t socket_obj, mp_obj_t first_frame_obj) {
    if (!frame_ready) {
        return mp_const_none;
    }

    int errcode;
    bool is_first_frame = mp_obj_is_true(first_frame_obj);

    const char *boundary;
    size_t boundary_len;
    if (is_first_frame) {
        boundary = boundary_first;
        boundary_len = sizeof(boundary_first) - 1;
    } else {
        boundary = boundary_subsequent;
        boundary_len = sizeof(boundary_subsequent) - 1;
    }

    cam_start_read();

    // Snapshot bucket indices (ISR won't update them while read_in_progress)
    uint8_t first_idx = frame_first_idx;
    uint8_t second_idx = frame_second_idx;

    mp_uint_t ret = mp_stream_write_exactly(socket_obj, boundary, boundary_len, &errcode);
    if (ret == MP_STREAM_ERROR) {
        cam_end_read();
        frame_ready = false;
        return mp_const_false;
    }

    const size_t chunk_size = 16384;

    // Send first half-frame bucket
    size_t half_sent = 0;
    while (half_sent < HALF_FRAME_BYTES) {
        size_t to_send = HALF_FRAME_BYTES - half_sent;
        if (to_send > chunk_size) {
            to_send = chunk_size;
        }

        ret = mp_stream_write_exactly(socket_obj, bucket[first_idx] + half_sent, to_send, &errcode);
        if (ret == MP_STREAM_ERROR || ret == 0) {
            cam_end_read();
            frame_ready = false;
            return mp_const_false;
        }

        half_sent += ret;
    }

    // Send second half-frame bucket
    half_sent = 0;
    while (half_sent < HALF_FRAME_BYTES) {
        size_t to_send = HALF_FRAME_BYTES - half_sent;
        if (to_send > chunk_size) {
            to_send = chunk_size;
        }

        ret = mp_stream_write_exactly(socket_obj, bucket[second_idx] + half_sent, to_send, &errcode);
        if (ret == MP_STREAM_ERROR || ret == 0) {
            cam_end_read();
            frame_ready = false;
            return mp_const_false;
        }

        half_sent += ret;
    }

    cam_end_read();
    frame_ready = false;
    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_2(camera_send_frame_data_c_obj, camera_send_frame_data_c);

// Stream loop in C (3-bucket half-frame)
// Args: socket object
// Returns: frame count when stream ends (disconnect, error, or KeyboardInterrupt)
//
// OPTIMIZATION: Send each half-frame (76,800 bytes) as a single mp_stream_write_exactly() call
// instead of 5x 16KB chunks. The W5500 driver's tx_service state machine handles internal
// 16KB TX buffer chunking with a SENDOK fast-path that skips redundant getSn_TX_FSR() polls.
// This reduces per-frame call overhead from 10 calls to 2 calls.
//
// TIMING: Prints per-frame timing breakdown every 50 frames:
//   wait_ms: time waiting for frame_ready (camera blanking + any frame sync delay)
//   send_ms: time for boundary + both halves (SPI + SENDOK waits)
//   total_ms: full frame period
static mp_obj_t camera_stream_loop_c(mp_obj_t socket_obj) {
    bool streaming = true;
    bool first_frame = true;
    uint32_t frame_count = 0;
    int errcode;

    // Timing accumulators (reset every 50 frames)
    uint32_t accum_wait_us = 0;
    uint32_t accum_send_us = 0;
    uint32_t accum_total_us = 0;
    uint32_t accum_count = 0;
    const uint32_t REPORT_INTERVAL = 50;

    uint32_t t_frame_start = mp_hal_ticks_us();

    while (streaming) {
        mp_handle_pending(true);

        if (frame_ready) {
            uint32_t t_got_frame = mp_hal_ticks_us();
            uint32_t wait_us = t_got_frame - t_frame_start;

            const char *boundary;
            size_t boundary_len;
            if (first_frame) {
                boundary = boundary_first;
                boundary_len = sizeof(boundary_first) - 1;
            } else {
                boundary = boundary_subsequent;
                boundary_len = sizeof(boundary_subsequent) - 1;
            }

            cam_start_read();

            // Snapshot bucket indices
            uint8_t first_idx = frame_first_idx;
            uint8_t second_idx = frame_second_idx;

            // Send boundary header
            mp_uint_t ret = mp_stream_write_exactly(socket_obj, boundary, boundary_len, &errcode);
            if (ret == MP_STREAM_ERROR) {
                cam_end_read();
                frame_ready = false;
                streaming = false;
                break;
            }

            // Send first half-frame as single 76,800 byte write
            // The W5500 driver internally chunks this into 16KB TX buffer writes
            // with SENDOK fast-path between chunks (no redundant buffer polling)
            ret = mp_stream_write_exactly(socket_obj, bucket[first_idx], HALF_FRAME_BYTES, &errcode);
            if (ret == MP_STREAM_ERROR || ret == 0) {
                cam_end_read();
                frame_ready = false;
                streaming = false;
                break;
            }

            // Send second half-frame as single 76,800 byte write
            ret = mp_stream_write_exactly(socket_obj, bucket[second_idx], HALF_FRAME_BYTES, &errcode);
            if (ret == MP_STREAM_ERROR || ret == 0) {
                cam_end_read();
                frame_ready = false;
                streaming = false;
                break;
            }

            cam_end_read();
            frame_ready = false;

            uint32_t t_send_done = mp_hal_ticks_us();
            uint32_t send_us = t_send_done - t_got_frame;
            uint32_t total_us = t_send_done - t_frame_start;

            first_frame = false;
            frame_count++;

            // Accumulate timing
            accum_wait_us += wait_us;
            accum_send_us += send_us;
            accum_total_us += total_us;
            accum_count++;

            if (accum_count >= REPORT_INTERVAL) {
                uint32_t avg_wait = accum_wait_us / accum_count;
                uint32_t avg_send = accum_send_us / accum_count;
                uint32_t avg_total = accum_total_us / accum_count;
                uint32_t fps_x10 = (accum_count * 10000000UL) / accum_total_us;
                mp_printf(&mp_plat_print,
                    "STREAM[%lu]: avg wait=%lu send=%lu total=%lu us  fps=%lu.%lu\n",
                    (unsigned long)frame_count,
                    (unsigned long)avg_wait,
                    (unsigned long)avg_send,
                    (unsigned long)avg_total,
                    (unsigned long)(fps_x10 / 10),
                    (unsigned long)(fps_x10 % 10));
                accum_wait_us = 0;
                accum_send_us = 0;
                accum_total_us = 0;
                accum_count = 0;
            }

            // Start timing next frame from here
            t_frame_start = t_send_done;
        }
    }

    return mp_obj_new_int(frame_count);
}
static MP_DEFINE_CONST_FUN_OBJ_1(camera_stream_loop_c_obj, camera_stream_loop_c);


// Define module globals
static const mp_rom_map_elem_t camera_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init_cam), MP_ROM_PTR(&camera_init_cam_obj) },
    { MP_ROM_QSTR(MP_QSTR_frame), MP_ROM_PTR(&camera_frame_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_i2c_pins), MP_ROM_PTR(&camera_set_i2c_pins_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pwm_pin), MP_ROM_PTR(&camera_set_pwm_pin_obj) },
    { MP_ROM_QSTR(MP_QSTR_start_cam), MP_ROM_PTR(&camera_start_cam_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_frame_ready), MP_ROM_PTR(&cam_is_frame_ready_obj) },
    { MP_ROM_QSTR(MP_QSTR_send_frame_over_eth), MP_ROM_PTR(&camera_send_frame_over_eth_obj) },
    { MP_ROM_QSTR(MP_QSTR_send_frame_data_c), MP_ROM_PTR(&camera_send_frame_data_c_obj) },
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
