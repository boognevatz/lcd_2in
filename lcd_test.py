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

print("Display should show text correctly now")
time.sleep(5)

# Clean up
print("Cleaning up...")
lcd.paint_clear(lcd.WHITE)
lcd.display(buf)
lcd.dev_module_exit()
print("Done")