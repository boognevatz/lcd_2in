#include <stdint.h>
#include "py/obj.h"
#include "py/objfun.h"  // <-- ADD THIS LINE
#include "py/builtin.h"
#include "py/objarray.h"
#include "py/runtime.h"
#include "LCD_2in.h"
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "fonts.h"

// --- LCD_2in wrappers ---
static mp_obj_t lcd_init(mp_obj_t scan_dir_obj) {
    UBYTE scan_dir = mp_obj_get_int(scan_dir_obj);
    LCD_2IN_Init(scan_dir);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(lcd_init_obj, lcd_init);

static mp_obj_t lcd_clear(mp_obj_t color_obj) {
    UWORD color = mp_obj_get_int(color_obj);
    LCD_2IN_Clear(color);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(lcd_clear_obj, lcd_clear);

static mp_obj_t lcd_set_windows(size_t n_args, const mp_obj_t *args) {
    UWORD xstart = mp_obj_get_int(args[0]);
    UWORD ystart = mp_obj_get_int(args[1]);
    UWORD xend = mp_obj_get_int(args[2]);
    UWORD yend = mp_obj_get_int(args[3]);
    LCD_2IN_SetWindows(xstart, ystart, xend, yend);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(lcd_set_windows_obj, 4, 4, lcd_set_windows);

static mp_obj_t lcd_display_point(mp_obj_t x_obj, mp_obj_t y_obj, mp_obj_t color_obj) {
    UWORD x = mp_obj_get_int(x_obj);
    UWORD y = mp_obj_get_int(y_obj);
    UWORD color = mp_obj_get_int(color_obj);
    LCD_2IN_DisplayPoint(x, y, color);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(lcd_display_point_obj, lcd_display_point);

static mp_obj_t lcd_display(mp_obj_t image_obj) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(image_obj, &bufinfo, MP_BUFFER_READ);
    LCD_2IN_Display((UBYTE *)bufinfo.buf);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(lcd_display_obj, lcd_display);

// --- DEV_Config wrappers ---
static mp_obj_t dev_module_init(void) {
    return mp_obj_new_int(DEV_Module_Init());
}
static MP_DEFINE_CONST_FUN_OBJ_0(dev_module_init_obj, dev_module_init);

static mp_obj_t dev_set_pwm(mp_obj_t value_obj) {
    uint8_t value = mp_obj_get_int(value_obj);
    DEV_SET_PWM(value);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(dev_set_pwm_obj, dev_set_pwm);

static mp_obj_t dev_digital_write(mp_obj_t pin_obj, mp_obj_t value_obj) {
    UWORD pin = mp_obj_get_int(pin_obj);
    UBYTE value = mp_obj_get_int(value_obj);
    DEV_Digital_Write(pin, value);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(dev_digital_write_obj, dev_digital_write);

static mp_obj_t dev_digital_read(mp_obj_t pin_obj) {
    UWORD pin = mp_obj_get_int(pin_obj);
    return mp_obj_new_int(DEV_Digital_Read(pin));
}
static MP_DEFINE_CONST_FUN_OBJ_1(dev_digital_read_obj, dev_digital_read);

static mp_obj_t dev_gpio_mode(mp_obj_t pin_obj, mp_obj_t mode_obj) {
    UWORD pin = mp_obj_get_int(pin_obj);
    UWORD mode = mp_obj_get_int(mode_obj);
    DEV_GPIO_Mode(pin, mode);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(dev_gpio_mode_obj, dev_gpio_mode);

static mp_obj_t dev_key_config(mp_obj_t pin_obj) {
    UWORD pin = mp_obj_get_int(pin_obj);
    DEV_KEY_Config(pin);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(dev_key_config_obj, dev_key_config);

static mp_obj_t dev_spi_write_byte(mp_obj_t value_obj) {
    UBYTE value = mp_obj_get_int(value_obj);
    DEV_SPI_WriteByte(value);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(dev_spi_write_byte_obj, dev_spi_write_byte);

static mp_obj_t dev_spi_write_nbyte(mp_obj_t data_obj) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(data_obj, &bufinfo, MP_BUFFER_READ);
    DEV_SPI_Write_nByte((uint8_t *)bufinfo.buf, bufinfo.len);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(dev_spi_write_nbyte_obj, dev_spi_write_nbyte);

static mp_obj_t dev_delay_ms(mp_obj_t ms_obj) {
    UDOUBLE ms = mp_obj_get_int(ms_obj);
    DEV_Delay_ms(ms);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(dev_delay_ms_obj, dev_delay_ms);

static mp_obj_t dev_delay_us(mp_obj_t us_obj) {
    UDOUBLE us = mp_obj_get_int(us_obj);
    DEV_Delay_us(us);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(dev_delay_us_obj, dev_delay_us);

static mp_obj_t dev_gpio_init(void) {
    DEV_GPIO_Init();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(dev_gpio_init_obj, dev_gpio_init);

static mp_obj_t dev_module_exit(void) {
    DEV_Module_Exit();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(dev_module_exit_obj, dev_module_exit);

// --- GUI_Paint wrappers ---
  static mp_obj_t paint_new_image(size_t n_args, const mp_obj_t *args) {
      mp_buffer_info_t bufinfo;
      mp_get_buffer_raise(args[0], &bufinfo, MP_BUFFER_RW);
      UWORD width = mp_obj_get_int(args[1]);
      UWORD height = mp_obj_get_int(args[2]);
      UWORD rotate = mp_obj_get_int(args[3]);
      UWORD color = mp_obj_get_int(args[4]);
      Paint_NewImage((UBYTE *)bufinfo.buf, width, height, rotate, color);
      return mp_const_none;
  }
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(paint_new_image_obj, 5, 5, paint_new_image);

static mp_obj_t paint_select_image(mp_obj_t image_obj) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(image_obj, &bufinfo, MP_BUFFER_RW);
    Paint_SelectImage((UBYTE *)bufinfo.buf);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(paint_select_image_obj, paint_select_image);

static mp_obj_t paint_set_rotate(mp_obj_t rotate_obj) {
    UWORD rotate = mp_obj_get_int(rotate_obj);
    Paint_SetRotate(rotate);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(paint_set_rotate_obj, paint_set_rotate);

static mp_obj_t paint_set_mirroring(mp_obj_t mirror_obj) {
    UBYTE mirror = mp_obj_get_int(mirror_obj);
    Paint_SetMirroring(mirror);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(paint_set_mirroring_obj, paint_set_mirroring);

static mp_obj_t paint_set_pixel(mp_obj_t x_obj, mp_obj_t y_obj, mp_obj_t color_obj) {
    UWORD x = mp_obj_get_int(x_obj);
    UWORD y = mp_obj_get_int(y_obj);
    UWORD color = mp_obj_get_int(color_obj);
    Paint_SetPixel(x, y, color);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(paint_set_pixel_obj, paint_set_pixel);

static mp_obj_t paint_set_scale(mp_obj_t scale_obj) {
    UBYTE scale = mp_obj_get_int(scale_obj);
    Paint_SetScale(scale);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(paint_set_scale_obj, paint_set_scale);

static mp_obj_t paint_clear(mp_obj_t color_obj) {
    UWORD color = mp_obj_get_int(color_obj);
    Paint_Clear(color);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(paint_clear_obj, paint_clear);

static mp_obj_t paint_clear_windows(size_t n_args, const mp_obj_t *args) {
    UWORD xstart = mp_obj_get_int(args[0]);
    UWORD ystart = mp_obj_get_int(args[1]);
    UWORD xend = mp_obj_get_int(args[2]);
    UWORD yend = mp_obj_get_int(args[3]);
    UWORD color = mp_obj_get_int(args[4]);
    Paint_ClearWindows(xstart, ystart, xend, yend, color);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(paint_clear_windows_obj, 5, 5, paint_clear_windows);

static mp_obj_t paint_draw_string_en(size_t n_args, const mp_obj_t *args) {
    UWORD x = mp_obj_get_int(args[0]);
    UWORD y = mp_obj_get_int(args[1]);
    const char *str = mp_obj_str_get_str(args[2]);
    sFONT *font = (sFONT *)MP_OBJ_TO_PTR(args[3]);
    UWORD color_fg = mp_obj_get_int(args[4]);
    UWORD color_bg = mp_obj_get_int(args[5]);
    Paint_DrawString_EN(x, y, str, font, color_fg, color_bg);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(paint_draw_string_en_obj, 6, 6, paint_draw_string_en);

// --- Font pointers as opaque ints (addresses) ---
#define FONT_PTR_OBJ(font) (mp_obj_new_int((uintptr_t)&font))

// --- Module globals ---
static const mp_rom_map_elem_t lcd_module_globals_table[] = {
    // LCD_2in
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&lcd_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&lcd_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_windows), MP_ROM_PTR(&lcd_set_windows_obj) },
    { MP_ROM_QSTR(MP_QSTR_display_point), MP_ROM_PTR(&lcd_display_point_obj) },
    { MP_ROM_QSTR(MP_QSTR_display), MP_ROM_PTR(&lcd_display_obj) },
    // DEV_Config
    { MP_ROM_QSTR(MP_QSTR_dev_module_init), MP_ROM_PTR(&dev_module_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_dev_set_pwm), MP_ROM_PTR(&dev_set_pwm_obj) },
    { MP_ROM_QSTR(MP_QSTR_dev_digital_write), MP_ROM_PTR(&dev_digital_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_dev_digital_read), MP_ROM_PTR(&dev_digital_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_dev_gpio_mode), MP_ROM_PTR(&dev_gpio_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_dev_key_config), MP_ROM_PTR(&dev_key_config_obj) },
    { MP_ROM_QSTR(MP_QSTR_dev_spi_write_byte), MP_ROM_PTR(&dev_spi_write_byte_obj) },
    { MP_ROM_QSTR(MP_QSTR_dev_spi_write_nbyte), MP_ROM_PTR(&dev_spi_write_nbyte_obj) },
    { MP_ROM_QSTR(MP_QSTR_dev_delay_ms), MP_ROM_PTR(&dev_delay_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_dev_delay_us), MP_ROM_PTR(&dev_delay_us_obj) },
    { MP_ROM_QSTR(MP_QSTR_dev_gpio_init), MP_ROM_PTR(&dev_gpio_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_dev_module_exit), MP_ROM_PTR(&dev_module_exit_obj) },
    // GUI_Paint
    { MP_ROM_QSTR(MP_QSTR_paint_new_image), MP_ROM_PTR(&paint_new_image_obj) },
    { MP_ROM_QSTR(MP_QSTR_paint_select_image), MP_ROM_PTR(&paint_select_image_obj) },
    { MP_ROM_QSTR(MP_QSTR_paint_set_rotate), MP_ROM_PTR(&paint_set_rotate_obj) },
    { MP_ROM_QSTR(MP_QSTR_paint_set_mirroring), MP_ROM_PTR(&paint_set_mirroring_obj) },
    { MP_ROM_QSTR(MP_QSTR_paint_set_pixel), MP_ROM_PTR(&paint_set_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_paint_set_scale), MP_ROM_PTR(&paint_set_scale_obj) },
    { MP_ROM_QSTR(MP_QSTR_paint_clear), MP_ROM_PTR(&paint_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_paint_clear_windows), MP_ROM_PTR(&paint_clear_windows_obj) },
    { MP_ROM_QSTR(MP_QSTR_paint_draw_string_en), MP_ROM_PTR(&paint_draw_string_en_obj) },
    // --- Constants ---
    { MP_ROM_QSTR(MP_QSTR_LCD_2IN_WIDTH), MP_ROM_INT(LCD_2IN_WIDTH) },
    { MP_ROM_QSTR(MP_QSTR_LCD_2IN_HEIGHT), MP_ROM_INT(LCD_2IN_HEIGHT) },
    { MP_ROM_QSTR(MP_QSTR_HORIZONTAL), MP_ROM_INT(HORIZONTAL) },
    { MP_ROM_QSTR(MP_QSTR_VERTICAL), MP_ROM_INT(VERTICAL) },
    { MP_ROM_QSTR(MP_QSTR_WHITE), MP_ROM_INT(WHITE) },
    { MP_ROM_QSTR(MP_QSTR_BLACK), MP_ROM_INT(BLACK) },
    { MP_ROM_QSTR(MP_QSTR_BLUE), MP_ROM_INT(BLUE) },
    { MP_ROM_QSTR(MP_QSTR_BRED), MP_ROM_INT(BRED) },
    { MP_ROM_QSTR(MP_QSTR_GRED), MP_ROM_INT(GRED) },
    { MP_ROM_QSTR(MP_QSTR_GBLUE), MP_ROM_INT(GBLUE) },
    { MP_ROM_QSTR(MP_QSTR_RED), MP_ROM_INT(RED) },
    { MP_ROM_QSTR(MP_QSTR_MAGENTA), MP_ROM_INT(MAGENTA) },
    { MP_ROM_QSTR(MP_QSTR_GREEN), MP_ROM_INT(GREEN) },
    { MP_ROM_QSTR(MP_QSTR_CYAN), MP_ROM_INT(CYAN) },
    { MP_ROM_QSTR(MP_QSTR_YELLOW), MP_ROM_INT(YELLOW) },
    { MP_ROM_QSTR(MP_QSTR_BROWN), MP_ROM_INT(BROWN) },
    { MP_ROM_QSTR(MP_QSTR_BRRED), MP_ROM_INT(BRRED) },
    { MP_ROM_QSTR(MP_QSTR_GRAY), MP_ROM_INT(GRAY) },
    // --- Font pointers ---
    { MP_ROM_QSTR(MP_QSTR_Font24), MP_ROM_PTR(&Font24) },
    { MP_ROM_QSTR(MP_QSTR_Font20), MP_ROM_PTR(&Font20) },
    { MP_ROM_QSTR(MP_QSTR_Font16), MP_ROM_PTR(&Font16) },
    { MP_ROM_QSTR(MP_QSTR_Font12), MP_ROM_PTR(&Font12) },
    { MP_ROM_QSTR(MP_QSTR_Font8), MP_ROM_PTR(&Font8) },
    { MP_ROM_QSTR(MP_QSTR_Font12CN), MP_ROM_PTR(&Font12CN) },
    { MP_ROM_QSTR(MP_QSTR_Font24CN), MP_ROM_PTR(&Font24CN) },
};
static MP_DEFINE_CONST_DICT(lcd_module_globals, lcd_module_globals_table);

const mp_obj_module_t lcd_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&lcd_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_lcd, lcd_user_cmodule);

