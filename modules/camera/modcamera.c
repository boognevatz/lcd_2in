#include "cam.h"
#include <string.h>  // for memset

extern uint8_t *cam_ptr;

// Wrapper for init_cam()
static mp_obj_t camera_init_cam() {
    init_cam();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(camera_init_cam_obj, camera_init_cam);


static mp_obj_t camera_get_buffer(void) {
    size_t size = CAM_FUL_SIZE * 2;
    cam_ptr = (uint8_t *)m_malloc(size);  //320*240*2 =153600
    memset(cam_ptr, 0, size);  // Zero out all bytes
    return mp_obj_new_bytearray_by_ref(size, cam_ptr);
}
static MP_DEFINE_CONST_FUN_OBJ_0(camera_get_buffer_obj, camera_get_buffer);


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


// Wrapper for config_cam_buffer()
static mp_obj_t camera_config_cam_buffer(mp_obj_t buf_obj) {
    config_cam_buffer(buf_obj);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(camera_config_cam_buffer_obj, camera_config_cam_buffer);

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


// Define module globals
static const mp_rom_map_elem_t camera_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init_cam), MP_ROM_PTR(&camera_init_cam_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_buffer), MP_ROM_PTR(&camera_get_buffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_i2c_pins), MP_ROM_PTR(&camera_set_i2c_pins_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pwm_pin), MP_ROM_PTR(&camera_set_pwm_pin_obj) },
    { MP_ROM_QSTR(MP_QSTR_config_cam_buffer), MP_ROM_PTR(&camera_config_cam_buffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_start_cam), MP_ROM_PTR(&camera_start_cam_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_buffer_ready), MP_ROM_PTR(&cam_is_buffer_ready_obj) },
    
};
static MP_DEFINE_CONST_DICT(camera_module_globals, camera_module_globals_table);

// Define module
const mp_obj_module_t camera_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&camera_module_globals,
};

// Register the module to make it available in Python
MP_REGISTER_MODULE(MP_QSTR_camera, camera_module);
