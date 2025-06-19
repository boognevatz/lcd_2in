CEXAMPLE_MOD_DIR := $(USERMOD_DIR)

# Add all C files to SRC_USERMOD.
SRC_USERMOD += $(CEXAMPLE_MOD_DIR)/DEV_Config.c
SRC_USERMOD += $(CEXAMPLE_MOD_DIR)/LCD_2in.c

# We can add our module folder to include paths if needed
# This is not actually needed in this example.
CFLAGS_USERMOD += -I$(CEXAMPLE_MOD_DIR)
