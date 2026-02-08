#include "cam.h"
#include <string.h>  // for memset
#include "ov5640.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/stream.h"

extern uint8_t *cam_ptr;

// Wrapper for init_cam()
static mp_obj_t camera_init_cam() {
    init_cam();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(camera_init_cam_obj, camera_init_cam);


static mp_obj_t camera_frame(void) {
    return mp_obj_new_bytearray_by_ref(CAM_FUL_SIZE * 2, cam_ptr);
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

static mp_obj_t cam_is_buffer_ready(void) {
    return mp_obj_new_bool(buffer_ready);
}
static MP_DEFINE_CONST_FUN_OBJ_0(cam_is_buffer_ready_obj, cam_is_buffer_ready);

static mp_obj_t camera_send_frame_over_eth(mp_obj_t callback) {
    if (!buffer_ready) {
        return mp_const_none;
    }
    mp_call_function_1(callback, mp_obj_new_bytearray_by_ref(CAM_FUL_SIZE * 2, cam_ptr));
    buffer_ready = false;
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

// Send frame data directly to socket from C (ported from Python send_frame_data)
// Args: socket object, is_first_frame (bool)
// Returns: True on success, False on failure
static mp_obj_t camera_send_frame_data_c(mp_obj_t socket_obj, mp_obj_t first_frame_obj) {
    if (!buffer_ready) {
        return mp_const_false;
    }

    int errcode;
    bool is_first_frame = mp_obj_is_true(first_frame_obj);

    // Send boundary header
    const char *boundary;
    size_t boundary_len;
    if (is_first_frame) {
        boundary = boundary_first;
        boundary_len = sizeof(boundary_first) - 1;  // -1 for null terminator
    } else {
        boundary = boundary_subsequent;
        boundary_len = sizeof(boundary_subsequent) - 1;
    }

    // Use mp_stream_write_exactly which returns bytes written (MP_STREAM_ERROR on error)
    mp_uint_t ret = mp_stream_write_exactly(socket_obj, boundary, boundary_len, &errcode);
    if (ret == MP_STREAM_ERROR) {
        return mp_const_false;
    }

    // Send frame data in chunks (16KB each, like Python version)
    const size_t chunk_size = 16384;
    const size_t frame_size = CAM_FUL_SIZE * 2;
    size_t total_sent = 0;

    while (total_sent < frame_size) {
        size_t to_send = frame_size - total_sent;
        if (to_send > chunk_size) {
            to_send = chunk_size;
        }

        ret = mp_stream_write_exactly(socket_obj, cam_ptr + total_sent, to_send, &errcode);
        if (ret == MP_STREAM_ERROR || ret == 0) {
            return mp_const_false;
        }

        total_sent += ret;
    }

    buffer_ready = false;
    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_2(camera_send_frame_data_c_obj, camera_send_frame_data_c);

// Stream loop in C (ported from Python /streamc while loop lines 1219-1236)
// Args: socket object
// Returns: frame count when stream ends (disconnect, error, or KeyboardInterrupt)
static mp_obj_t camera_stream_loop_c(mp_obj_t socket_obj) {
    bool streaming = true;
    bool first_frame = true;
    uint32_t frame_count = 0;
    int errcode;

    while (streaming) {
        // Handle pending signals (KeyboardInterrupt)
        // This will raise MP_OBJ_STOP_ITERATION or similar on Ctrl+C
        mp_handle_pending(true);

        if (buffer_ready) {
            // Send boundary header
            const char *boundary;
            size_t boundary_len;
            if (first_frame) {
                boundary = boundary_first;
                boundary_len = sizeof(boundary_first) - 1;  // -1 for null terminator
            } else {
                boundary = boundary_subsequent;
                boundary_len = sizeof(boundary_subsequent) - 1;
            }

            mp_uint_t ret = mp_stream_write_exactly(socket_obj, boundary, boundary_len, &errcode);
            if (ret == MP_STREAM_ERROR) {
                streaming = false;
                break;
            }

            // Send frame data in chunks (16KB each, like Python version)
            const size_t chunk_size = 16384;
            const size_t frame_size = CAM_FUL_SIZE * 2;
            size_t total_sent = 0;

            while (total_sent < frame_size) {
                size_t to_send = frame_size - total_sent;
                if (to_send > chunk_size) {
                    to_send = chunk_size;
                }

                ret = mp_stream_write_exactly(socket_obj, cam_ptr + total_sent, to_send, &errcode);
                if (ret == MP_STREAM_ERROR || ret == 0) {
                    streaming = false;
                    break;
                }

                total_sent += ret;
            }

            if (!streaming) break;

            buffer_ready = false;
            first_frame = false;
            frame_count++;
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
    { MP_ROM_QSTR(MP_QSTR_is_buffer_ready), MP_ROM_PTR(&cam_is_buffer_ready_obj) },
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
