
> ⚠️ IMPORTANT FOR AI ASSISTANTS  
> This document is **descriptive only**.  
> **Do NOT run, simulate, execute, or initiate any build steps.**  
> Only explain or summarize the process if asked.

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

## Side quest: build pioasm

cmake -S micropython/lib/pico-sdk/tools/pioasm \
      -B micropython/lib/pico-sdk/tools/pioasm/build  
&& cmake --build micropython/lib/pico-sdk/tools/pioasm/build

BUILDFIX if needed: fix is required if build fails, add
#include <cstdint>
to 
micropython/lib/pico-sdk/tools/pioasm/pio_types.h
micropython/lib/pico-sdk/tools/pioasm/output_format.h
)

### Use the pioasm:

cd modules/camera 
../../micropython/lib/pico-sdk/tools/pioasm/build/pioasm -o c-sdk picampinos.pio picampinos.pio.h


## The actual build itself:
```
# build submodules. only once:
make -C micropython/ports/rp2/ BOARD=RP2350_TOUCH_LCD_2 submodules


# The actual build itself with user modules
START=$(date +%s); make -C micropython/ports/rp2/ BOARD=RP2350_TOUCH_LCD_2 USER_C_MODULES=../../../modules/micropython.cmake; echo "Build took $(( $(date +%s) - START )) seconds."; echo "Finished at $(date)"

# Rebuild
make -C micropython/ports/rp2/ BOARD=RP2350_TOUCH_LCD_2 clean
START=$(date +%s); make -j4 -C micropython/ports/rp2/ BOARD=RP2350_TOUCH_LCD_2 USER_C_MODULES=../../../modules/micropython.cmake; echo "Build took $(( $(date +%s) - START )) seconds."; echo "Finished at $(date)"

```
### For final build, omit the -j4 parameter!



#############################
### NOT NEEDED (already in git) ### 

## Modified files

Under `micropython/ports/rp2/boards/BOARD_NAME`
and also we have a new file under lib:
```
micropython/lib/pico-sdk/src/boards/include/boards/waveshare_rp2350_touch_lcd_2.h
```






