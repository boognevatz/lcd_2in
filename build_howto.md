
## Host computer sofware requirements:

```
sudo apt-get update && sudo apt-get install -y cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
```
## install micropython

```
git submodule update --init --remote --checkout
```

## install submodules under /lib directory (pico-sdk, wiznet, etc)

```
cd micropython
git submodule update --init --remote --checkout

```
## Checkout the correct branches and tags

```
cd micropython/lib/mbedtls
git checkout mbedtls-3.6.2 # tag

cd micropython/lib/wiznet5k 
git checkout micropython # branch

```


## The actual build itself:
```
# only once:
make -C micropython/ports/rp2/ BOARD=RP2350_TOUCH_LCD_2 submodules


# the actual build itself:

## without user modules:
# make -C micropython/ports/rp2/ BOARD=RP2350_TOUCH_LCD_2

# With user modules
make -C micropython/ports/rp2/ BOARD=RP2350_TOUCH_LCD_2 USER_C_MODULES=../../../modules/micropython.cmake
```



#############################
### NOT NEEDED (already in git) ### 

## Modified files

Under `micropython/ports/rp2/boards/BOARD_NAME`
and also we have a new file under lib:
```
micropython/lib/pico-sdk/src/boards/include/boards/waveshare_rp2350_touch_lcd_2.h
```






