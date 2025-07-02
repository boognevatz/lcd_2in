#include "py/obj.h"
#include "cam.h"


// Wrapper for init_cam()
static mp_obj_t camera_init_cam() {
    mp_printf(MP_PYTHON_PRINTER, "Hello from C module, init_cam!\n");
    init_cam();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(camera_init_cam_obj, camera_init_cam);

// Wrapper for config_cam_buffer()
static mp_obj_t camera_config_cam_buffer() {
    mp_printf(MP_PYTHON_PRINTER, "Hello from C module, config_cam_buffer!\n");
    config_cam_buffer();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(camera_config_cam_buffer_obj, camera_config_cam_buffer);


// Define module globals
static const mp_rom_map_elem_t camera_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init_cam), MP_ROM_PTR(&camera_init_cam_obj) },
    { MP_ROM_QSTR(MP_QSTR_config_cam_buffer), MP_ROM_PTR(&camera_config_cam_buffer_obj) },
    
};
static MP_DEFINE_CONST_DICT(camera_module_globals, camera_module_globals_table);

// Define module
const mp_obj_module_t camera_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&camera_module_globals,
};

// Register the module to make it available in Python
MP_REGISTER_MODULE(MP_QSTR_camera, camera_module);
