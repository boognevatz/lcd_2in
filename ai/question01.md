-[ 40%] Building C object CMakeFiles/firmware.dir/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c.o
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c: In function 'wiz_spi_writeburst':
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:170:9: error: unknown type name 'machine_spi_obj_t'; did you mean 'machine_mem_obj_t'?
  170 |         machine_spi_obj_t *spi_obj = (machine_spi_obj_t *)wiznet5k_obj.spi;
      |         ^~~~~~~~~~~~~~~~~
      |         machine_mem_obj_t
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:170:39: error: 'machine_spi_obj_t' undeclared (first use in this function); did you mean 'machine_mem_obj_t'?
  170 |         machine_spi_obj_t *spi_obj = (machine_spi_obj_t *)wiznet5k_obj.spi;
      |                                       ^~~~~~~~~~~~~~~~~
      |                                       machine_mem_obj_t
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:170:39: note: each undeclared identifier is reported only once for each function it appears in
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:170:58: error: expected expression before ')' token
  170 |         machine_spi_obj_t *spi_obj = (machine_spi_obj_t *)wiznet5k_obj.spi;
      |                                                          ^
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:171:39: error: request for member 'spi_inst' in something not a structure or union
  171 |         spi_inst_t *spi_inst = spi_obj->spi_inst;
      |                                       ^~
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:174:27: error: implicit declaration of function 'dma_claim_unused_channel' [-Wimplicit-function-declaration]
  174 |         int dma_rx_chan = dma_claim_unused_channel(true);
      |                           ^~~~~~~~~~~~~~~~~~~~~~~~
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:184:9: error: unknown type name 'dma_channel_config'
  184 |         dma_channel_config c = dma_channel_get_default_config(dma_tx_chan);
      |         ^~~~~~~~~~~~~~~~~~
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:184:32: error: implicit declaration of function 'dma_channel_get_default_config' [-Wimplicit-function-declaration]
  184 |         dma_channel_config c = dma_channel_get_default_config(dma_tx_chan);
      |                                ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:185:9: error: implicit declaration of function 'channel_config_set_transfer_data_size' [-Wimplicit-function-declaration]
  185 |         channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
      |         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:185:51: error: 'DMA_SIZE_8' undeclared (first use in this function)
  185 |         channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
      |                                                   ^~~~~~~~~~
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:186:9: error: implicit declaration of function 'channel_config_set_read_increment' [-Wimplicit-function-declaration]
  186 |         channel_config_set_read_increment(&c, true);
      |         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:187:9: error: implicit declaration of function 'channel_config_set_write_increment' [-Wimplicit-function-declaration]
  187 |         channel_config_set_write_increment(&c, false);
      |         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:188:9: error: implicit declaration of function 'channel_config_set_dreq' [-Wimplicit-function-declaration]
  188 |         channel_config_set_dreq(&c, spi_get_dreq(spi_inst, true));
      |         ^~~~~~~~~~~~~~~~~~~~~~~
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:189:9: error: implicit declaration of function 'dma_channel_configure' [-Wimplicit-function-declaration]
  189 |         dma_channel_configure(dma_tx_chan, &c, &spi_get_hw(spi_inst)->dr, pBuf, len, false);
      |         ^~~~~~~~~~~~~~~~~~~~~
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:200:9: error: implicit declaration of function 'dma_start_channel_mask' [-Wimplicit-function-declaration]
  200 |         dma_start_channel_mask((1u << dma_rx_chan) | (1u << dma_tx_chan));
      |         ^~~~~~~~~~~~~~~~~~~~~~
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:201:9: error: implicit declaration of function 'dma_channel_wait_for_finish_blocking' [-Wimplicit-function-declaration]
  201 |         dma_channel_wait_for_finish_blocking(dma_rx_chan);
      |         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:205:9: error: implicit declaration of function 'dma_channel_unclaim' [-Wimplicit-function-declaration]
  205 |         dma_channel_unclaim(dma_rx_chan);
      |         ^~~~~~~~~~~~~~~~~~~
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c: In function 'wiznet5k_active':
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:914:17: error: unknown type name 'machine_spi_obj_t'; did you mean 'machine_mem_obj_t'?
  914 |                 machine_spi_obj_t *spi_obj = (machine_spi_obj_t *)wiznet5k_obj.spi;
      |                 ^~~~~~~~~~~~~~~~~
      |                 machine_mem_obj_t
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:914:47: error: 'machine_spi_obj_t' undeclared (first use in this function); did you mean 'machine_mem_obj_t'?
  914 |                 machine_spi_obj_t *spi_obj = (machine_spi_obj_t *)wiznet5k_obj.spi;
      |                                               ^~~~~~~~~~~~~~~~~
      |                                               machine_mem_obj_t
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:914:66: error: expected expression before ')' token
  914 |                 machine_spi_obj_t *spi_obj = (machine_spi_obj_t *)wiznet5k_obj.spi;
      |                                                                  ^
/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c:915:35: error: request for member 'spi_inst' in something not a structure or union
  915 |                 spi_get_hw(spi_obj->spi_inst)->dmacr = (1 << 1) | (1 << 0);
      |                                   ^~
make[3]: *** [CMakeFiles/firmware.dir/build.make:2859: CMakeFiles/firmware.dir/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/extmod/network_wiznet5k.c.o] Error 1
make[2]: *** [CMakeFiles/Makefile2:2359: CMakeFiles/firmware.dir/all] Error 2
make[1]: *** [Makefile:91: all] Error 2
make[1]: Leaving directory '/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/ports/rp2/build-RP2350_TOUCH_LCD_2'
See https://github.com/micropython/micropython/wiki/Build-Troubleshooting
make: *** [Makefile:65: all] Error 1
make: Leaving directory '/home/lama/Asztal/projects2/aron_polinvent/lcd_2in/micropython/ports/rp2'
Build took 359 seconds.
Finished at Sun Jan 11 11:59:12 AM CET 202



6--

Good news first: **this error confirms you are now in the correct build path** (non-lwIP, WIZnet-native stack).
Bad news: you’ve hit a **classic MicroPython layering boundary**.

Let’s go through this cleanly.

---

## What actually broke (root cause)

### 1. `network_wiznet5k.c` is *platform-agnostic*

It lives in:

```
micropython/extmod/network_wiznet5k.c
```

This file is intentionally written to:

* work on **many ports**
* not depend on **machine SPI internals**
* not depend on **RP2 SDK headers**
* not know what `machine_spi_obj_t` is

So when you wrote:

```c
machine_spi_obj_t *spi_obj = (machine_spi_obj_t *)wiznet5k_obj.spi;
```

you crossed a **forbidden boundary**:

> ❌ `extmod/` code must NOT depend on `ports/rp2/` internals

That’s why the compiler says:

```text
unknown type name 'machine_spi_obj_t'
```

Because:

* `machine_spi_obj_t` is defined in
  `ports/rp2/machine_spi.c`
* It is **not visible** to `extmod/`

---

### 2. Same problem with DMA

All these symbols:

```c
dma_claim_unused_channel
dma_channel_config
DMA_SIZE_8
spi_get_hw
```

come from:

```
pico-sdk (hardware_dma.h, hardware_spi.h)
```

Those headers are:

* only included by **rp2 port code**
* NOT available in `extmod/`

So the compiler errors are 100% correct.

---

## Very important rule (MicroPython architecture)

> **`extmod/` must be portable**
> **`ports/rp2/` is where hardware-specific code goes**

You violated this rule by putting RP2350-specific SPI + DMA code into:

```
extmod/network_wiznet5k.c
```

---

## Why this appeared *only now*

Earlier:

* lwIP was enabled
* `network_wiznet5k.c` was mostly idle
* your new code wasn’t compiled in the active path

Now:

* lwIP is disabled ✅
* WIZnet native stack is active ✅
* your DMA/SPI hack is finally being compiled ❌

So this is expected.

---

## The correct fix (the only clean solution)

### ✅ Move ALL RP2350-specific code into the rp2 port

You have **two valid patterns**. I’ll show the correct one.

---

## ✅ Correct architecture for your use case

### Step 1: Keep `network_wiznet5k.c` generic

In `extmod/network_wiznet5k.c`:

* ❌ No `machine_spi_obj_t`
* ❌ No DMA
* ❌ No pico-sdk headers

Instead, define **hooks**:

```c
// extmod/network_wiznet5k.c
void wiznet5k_spi_writeburst(const uint8_t *buf, size_t len) {
    mp_hal_wiznet_spi_write(buf, len);
}
```

No hardware details here.

---

### Step 2: Implement the hook in the RP2350 port

Create or modify:

```
ports/rp2/mp_hal_wiznet_spi.c   (or similar)
```

There, you may safely do:

```c
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "machine_spi.h"

void mp_hal_wiznet_spi_write(const uint8_t *buf, size_t len) {
    machine_spi_obj_t *spi_obj = MP_OBJ_TO_PTR(wiznet5k_obj.spi);
    spi_inst_t *spi = spi_obj->spi_inst;

    // DMA code here (fully legal)
}
```

This is the **only place** where:

* `machine_spi_obj_t`
* `spi_inst_t`
* DMA
  are allowed.

---

### Step 3: Wire it together

`extmod/network_wiznet5k.c`
⬇
`mp_hal_wiznet_spi_write()`
⬇
`ports/rp2/… DMA SPI code`

This preserves:

* portability
* build correctness
* upstream compatibility

---

## Why this matters for your camera streaming

You want:

* Zero-copy DMA
* Deterministic SPI
* Direct TX buffer fill

That is **absolutely valid**, but it must be:

* **below** the network module
* **inside** the rp2 HAL layer

You were conceptually right, just **one layer too high**.

---

## Final mental model (important)

| Layer                       | What belongs here      |
| --------------------------- | ---------------------- |
| `extmod/network_wiznet5k.c` | Generic W5500 logic    |
| `modsocket.c`               | Python socket API      |
| `ports/rp2/`                | SPI, DMA, GPIO, clocks |
| `machine_spi.c`             | SPI object internals   |
| Your camera DMA             | **rp2 only**           |

---

## Summary

* Your build error is **expected and correct**
* `machine_spi_obj_t` must NOT be used in `extmod/`
* DMA must NOT be used in `extmod/`
* Move SPI+DMA code into `ports/rp2/`
* Keep `network_wiznet5k.c` generic
* You are now on the *right* architectural path

