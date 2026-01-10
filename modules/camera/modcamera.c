#include "cam.h"
#include <string.h>  // for memset
#include "ov5640.h"
#include "py/obj.h"
#include "py/runtime.h"

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

static mp_obj_t camera_init_streaming(mp_obj_t dest_ip_obj, mp_obj_t dest_port_obj) {
    mp_buffer_info_t ip_buf;
    mp_get_buffer_raise(dest_ip_obj, &ip_buf, MP_BUFFER_READ);
    if (ip_buf.len != 4) {
        mp_raise_ValueError(MP_ERROR_TEXT("dest_ip must be 4 bytes"));
    }
    uint16_t port = mp_obj_get_int(dest_port_obj);
    init_streaming((uint8_t*)ip_buf.buf, port);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(camera_init_streaming_obj, camera_init_streaming);

static mp_obj_t camera_streaming_loop() {
    streaming_loop();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(camera_streaming_loop_obj, camera_streaming_loop);

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


// Define module globals
static const mp_rom_map_elem_t camera_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init_cam), MP_ROM_PTR(&camera_init_cam_obj) },
    { MP_ROM_QSTR(MP_QSTR_frame), MP_ROM_PTR(&camera_frame_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_i2c_pins), MP_ROM_PTR(&camera_set_i2c_pins_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pwm_pin), MP_ROM_PTR(&camera_set_pwm_pin_obj) },
    { MP_ROM_QSTR(MP_QSTR_start_cam), MP_ROM_PTR(&camera_start_cam_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_buffer_ready), MP_ROM_PTR(&cam_is_buffer_ready_obj) },
    { MP_ROM_QSTR(MP_QSTR_send_frame_over_eth), MP_ROM_PTR(&camera_send_frame_over_eth_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_data_order), MP_ROM_PTR(&camera_set_data_order_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_data_pins), MP_ROM_PTR(&camera_set_data_pins_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_control_pins), MP_ROM_PTR(&camera_set_control_pins_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_xclk_pin), MP_ROM_PTR(&camera_set_xclk_pin_obj) },
    { MP_ROM_QSTR(MP_QSTR_init_streaming), MP_ROM_PTR(&camera_init_streaming_obj) },
    { MP_ROM_QSTR(MP_QSTR_streaming_loop), MP_ROM_PTR(&camera_streaming_loop_obj) },
};
static MP_DEFINE_CONST_DICT(camera_module_globals, camera_module_globals_table);

// Define module
const mp_obj_module_t camera_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&camera_module_globals,
};

// Register the module to make it available in Python
MP_REGISTER_MODULE(MP_QSTR_camera, camera_module);
