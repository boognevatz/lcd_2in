
## Host computer sofware requirements:

```
sudo apt-get update && sudo apt-get install -y cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
```

## install pico-sdk under `micropython/lib/` dir:

```
mkdir -p pico-sdk && cd pico-sdk && git clone https://github.com/raspberrypi/pico-sdk.git . && git submodule update --init
```

## Modified files

Under `micropython/ports/rp2/boards/BOARD_NAME`
and also we have a new file under lib:
```
micropython/lib/pico-sdk/src/boards/include/boards/waveshare_rp2350_touch_lcd_2.h
```


## The actual build itself:
```
# only once?:
make -C micropython/ports/rp2/ BOARD=RP2350_TOUCH_LCD_2 submodules


# the actual build itself:

## without user modules:
# make -C micropython/ports/rp2/ BOARD=RP2350_TOUCH_LCD_2

# With user modules
make -C micropython/ports/rp2/ BOARD=RP2350_TOUCH_LCD_2 USER_C_MODULES=../../../modules/micropython.cmake
```



