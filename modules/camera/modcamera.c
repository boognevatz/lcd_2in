#include "cam.h"
#include <string.h>  // for memset
#include "ov5640.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/stream.h"

// Double buffer pointers from cam.c
extern uint8_t *cam_python_read_buf;

// Boundary headers for MJPEG streaming (matches Python version)
static const char boundary_first[] = "--frame\r\nContent-Type: application/octet-stream\r\n\r\n";
static const char boundary_subsequent[] = "\r\n--frame\r\nContent-Type: application/octet-stream\r\n\r\n";

// Wrapper for init_cam()
static mp_obj_t camera_init_cam() {
    init_cam();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(camera_init_cam_obj, camera_init_cam);

// Wrapper for init_cam_with_registers() - accepts list of (reg, value) tuples
static mp_obj_t camera_init_cam_with_registers(mp_obj_t regs_list) {
    size_t len;
    mp_obj_t *items;
    mp_obj_get_array(regs_list, &len, &items);
    
    if (len == 0 || len > 32) {
        mp_raise_ValueError(MP_ERROR_TEXT("Register list must have 1-32 entries"));
    }
    
    uint16_t custom_regs[32][2];
    
    for (size_t i = 0; i < len; i++) {
        size_t pair_len;
        mp_obj_t *pair_items;
        mp_obj_get_array(items[i], &pair_len, &pair_items);
        
        if (pair_len != 2) {
            mp_raise_ValueError(MP_ERROR_TEXT("Each entry must be (reg, value) pair"));
        }
        
        custom_regs[i][0] = mp_obj_get_int(pair_items[0]);
        custom_regs[i][1] = mp_obj_get_int(pair_items[1]);
    }
    
    init_cam_with_registers(custom_regs, len);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(camera_init_cam_with_registers_obj, camera_init_cam_with_registers);


static mp_obj_t camera_frame(void) {
    // Returns the safe read buffer (not being written to by DMA)
    return mp_obj_new_bytearray_by_ref(CAM_FUL_SIZE * 2, cam_python_read_buf);
}
static MP_DEFINE_CONST_FUN_OBJ_0(camera_frame_obj, camera_frame);

// Double buffer control - call before reading frame to lock buffer
static mp_obj_t camera_start_read(void) {
    cam_start_read();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(camera_start_read_obj, camera_start_read);

// Double buffer control - call after reading frame to allow swap
static mp_obj_t camera_end_read(void) {
    cam_end_read();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(camera_end_read_obj, camera_end_read);


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

// Send frame via Python callback (wraps double-buffer lock/unlock)
static mp_obj_t camera_send_frame_over_eth(mp_obj_t callback) {
    if (!buffer_ready) {
        return mp_const_none;
    }
    cam_start_read();
    mp_call_function_1(callback, mp_obj_new_bytearray_by_ref(CAM_FUL_SIZE * 2, cam_python_read_buf));
    cam_end_read();
    buffer_ready = false;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(camera_send_frame_over_eth_obj, camera_send_frame_over_eth);

// Send single frame data to socket from C (with double-buffer protection)
// Returns: True = frame sent, False = error/disconnect, None = no frame ready
static mp_obj_t camera_send_frame_data_c(mp_obj_t socket_obj, mp_obj_t first_frame_obj) {
    if (!buffer_ready) {
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

    mp_uint_t ret = mp_stream_write_exactly(socket_obj, boundary, boundary_len, &errcode);
    if (ret == MP_STREAM_ERROR) {
        cam_end_read();
        buffer_ready = false;
        return mp_const_false;
    }

    const size_t chunk_size = 16384;
    const size_t frame_size = CAM_FUL_SIZE * 2;
    size_t total_sent = 0;

    while (total_sent < frame_size) {
        size_t to_send = frame_size - total_sent;
        if (to_send > chunk_size) {
            to_send = chunk_size;
        }

        ret = mp_stream_write_exactly(socket_obj, cam_python_read_buf + total_sent, to_send, &errcode);
        if (ret == MP_STREAM_ERROR || ret == 0) {
            cam_end_read();
            buffer_ready = false;
            return mp_const_false;
        }

        total_sent += ret;
    }

    cam_end_read();
    buffer_ready = false;
    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_2(camera_send_frame_data_c_obj, camera_send_frame_data_c);

// C streaming loop: eliminates Python per-frame overhead
// Args: socket object
// Returns: frame count when stream ends (disconnect, error, or KeyboardInterrupt)
static mp_obj_t camera_stream_loop_c(mp_obj_t socket_obj) {
    bool streaming = true;
    bool first_frame = true;
    uint32_t frame_count = 0;
    int errcode;

    while (streaming) {
        mp_handle_pending(true);

        if (buffer_ready) {
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

            // nlr guard to ensure cam_end_read() on exception
            nlr_buf_t nlr;
            if (nlr_push(&nlr) == 0) {
                mp_uint_t ret = mp_stream_write_exactly(socket_obj, boundary, boundary_len, &errcode);
                if (ret == MP_STREAM_ERROR) {
                    nlr_pop();
                    cam_end_read();
                    buffer_ready = false;
                    streaming = false;
                    break;
                }

                const size_t chunk_size = 16384;
                const size_t frame_size = CAM_FUL_SIZE * 2;
                size_t total_sent = 0;

                while (total_sent < frame_size) {
                    size_t to_send = frame_size - total_sent;
                    if (to_send > chunk_size) {
                        to_send = chunk_size;
                    }

                    ret = mp_stream_write_exactly(socket_obj, cam_python_read_buf + total_sent, to_send, &errcode);
                    if (ret == MP_STREAM_ERROR || ret == 0) {
                        streaming = false;
                        break;
                    }

                    total_sent += to_send;
                }

                nlr_pop();
            } else {
                // Exception raised (e.g. KeyboardInterrupt) - release lock and re-raise
                cam_end_read();
                buffer_ready = false;
                nlr_jump(nlr.ret_val);
            }

            cam_end_read();
            buffer_ready = false;

            if (!streaming) break;

            first_frame = false;
            frame_count++;
        }
    }

    return mp_obj_new_int(frame_count);
}
static MP_DEFINE_CONST_FUN_OBJ_1(camera_stream_loop_c_obj, camera_stream_loop_c);

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

// Python wrapper for ov5640_write_register - write a single register
static mp_obj_t camera_write_register(mp_obj_t reg_obj, mp_obj_t value_obj) {
    uint16_t reg = mp_obj_get_int(reg_obj);
    uint8_t value = mp_obj_get_int(value_obj);
    int result = ov5640_write_register(reg, value);
    if (result < 0) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Camera not initialized"));
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(camera_write_register_obj, camera_write_register);

// Python wrapper for ov5640_read_register - read a single register
static mp_obj_t camera_read_register(mp_obj_t reg_obj) {
    uint16_t reg = mp_obj_get_int(reg_obj);
    int value = ov5640_read_register(reg);
    if (value < 0) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Camera not initialized"));
    }
    return mp_obj_new_int(value);
}
static MP_DEFINE_CONST_FUN_OBJ_1(camera_read_register_obj, camera_read_register);

// Python wrapper for writing multiple registers - accepts list of (reg, value) tuples
static mp_obj_t camera_write_registers(mp_obj_t regs_list) {
    size_t len;
    mp_obj_t *items;
    mp_obj_get_array(regs_list, &len, &items);
    
    for (size_t i = 0; i < len; i++) {
        size_t pair_len;
        mp_obj_t *pair_items;
        mp_obj_get_array(items[i], &pair_len, &pair_items);
        
        if (pair_len != 2) {
            mp_raise_ValueError(MP_ERROR_TEXT("Each entry must be (reg, value) pair"));
        }
        
        uint16_t reg = mp_obj_get_int(pair_items[0]);
        uint8_t value = mp_obj_get_int(pair_items[1]);
        
        int result = ov5640_write_register(reg, value);
        if (result < 0) {
            mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Camera not initialized"));
        }
    }
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(camera_write_registers_obj, camera_write_registers);


// Define module globals
static const mp_rom_map_elem_t camera_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init_cam), MP_ROM_PTR(&camera_init_cam_obj) },
    { MP_ROM_QSTR(MP_QSTR_init_cam_with_registers), MP_ROM_PTR(&camera_init_cam_with_registers_obj) },
    { MP_ROM_QSTR(MP_QSTR_frame), MP_ROM_PTR(&camera_frame_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_i2c_pins), MP_ROM_PTR(&camera_set_i2c_pins_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pwm_pin), MP_ROM_PTR(&camera_set_pwm_pin_obj) },
    { MP_ROM_QSTR(MP_QSTR_start_cam), MP_ROM_PTR(&camera_start_cam_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_buffer_ready), MP_ROM_PTR(&cam_is_buffer_ready_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_data_order), MP_ROM_PTR(&camera_set_data_order_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_data_pins), MP_ROM_PTR(&camera_set_data_pins_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_control_pins), MP_ROM_PTR(&camera_set_control_pins_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_xclk_pin), MP_ROM_PTR(&camera_set_xclk_pin_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_register), MP_ROM_PTR(&camera_write_register_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_register), MP_ROM_PTR(&camera_read_register_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_registers), MP_ROM_PTR(&camera_write_registers_obj) },
    // Double buffer control
    { MP_ROM_QSTR(MP_QSTR_start_read), MP_ROM_PTR(&camera_start_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_end_read), MP_ROM_PTR(&camera_end_read_obj) },
    // C-level streaming functions
    { MP_ROM_QSTR(MP_QSTR_send_frame_over_eth), MP_ROM_PTR(&camera_send_frame_over_eth_obj) },
    { MP_ROM_QSTR(MP_QSTR_send_frame_data_c), MP_ROM_PTR(&camera_send_frame_data_c_obj) },
    { MP_ROM_QSTR(MP_QSTR_stream_loop_c), MP_ROM_PTR(&camera_stream_loop_c_obj) },
};
static MP_DEFINE_CONST_DICT(camera_module_globals, camera_module_globals_table);

// Define module
const mp_obj_module_t camera_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&camera_module_globals,
};

// Register the module to make it available in Python
MP_REGISTER_MODULE(MP_QSTR_camera, camera_module);
