### INITIALIZING LCD
import lcd
import time

# Mirroring constants (since they're not defined in module)
MIRROR_NONE = 0x00
MIRROR_HORIZONTAL = 0x01
MIRROR_VERTICAL = 0x02
MIRROR_ORIGIN = 0x03

print("Initializing hardware...")
lcd.dev_module_init()
lcd.init(lcd.VERTICAL)  # Use VERTICAL orientation

# Display dimensions
width = lcd.LCD_2IN_WIDTH  # Should be 240
height = lcd.LCD_2IN_HEIGHT  # Should be 320
print(f"Display size: {width}x{height}")

# Create image buffer
buf_size = height * width * 2  # 320*240*2 = 153,600 bytes
print(f"Creating buffer size: {buf_size} bytes")
buf = bytearray(buf_size)

# Initialize paint structure
print("Initializing paint...")
lcd.paint_new_image(buf, width, height, 0, lcd.WHITE)
lcd.paint_set_scale(65)  # Set to 16bpp mode

# Apply mirroring fix - try different options
#mirror_mode = MIRROR_NONE  # Start with no mirroring
mirror_mode = MIRROR_HORIZONTAL  # If text is backwards
# mirror_mode = MIRROR_VERTICAL    # If text is upside down
# mirror_mode = MIRROR_ORIGIN      # If both issues
lcd.paint_set_mirroring(mirror_mode)

# Draw "Hello World" text
text = "Hello World"
x = 50
y = 150

# Clear to white background
lcd.paint_clear(lcd.WHITE)

# Draw the text
lcd.paint_draw_string_en(x, y, text, lcd.Font16, lcd.BLACK, lcd.WHITE)

# Display the image buffer
lcd.display(buf)

#### INITIALIZING CAMERA


import camera
# Set new hardware pinout for camera
# camera.set_data_pins([19, 18, 17, 16, 15, 14, 13, 12])  # D0-D7
# camera.set_control_pins(9, 8, 6)  # VSYNC, HREF, PCLK
# camera.set_xclk_pin(7)            # XCLK
# camera.set_i2c_pins(24, 25)       # SDA, SCL (new hardware)
# camera.init_cam()
# buf = camera.get_buffer()
# camera.config_cam_buffer(buf)
# camera.start_cam()

# Set old hardware pinout for camera
camera.set_data_pins([0, 1, 2, 3, 4, 5, 6, 7])  # D0-D7
camera.set_control_pins(8, 9, 10)  # VSYNC, HREF, PCLK
camera.set_xclk_pin(11)            # XCLK
camera.set_i2c_pins(22, 23)       # SDA, SCL (new hardware)
camera.init_cam()
buf = camera.get_buffer()
camera.config_cam_buffer(buf)
camera.start_cam()

lcd.paint_draw_image(buf,0,0,320,240)
lcd.display(buf)

