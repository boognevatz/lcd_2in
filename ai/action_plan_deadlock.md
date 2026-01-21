# Action Plan: W5500 Camera Streaming Deadlock Fix

## Problem Summary

Camera streaming system freezes after transmitting exactly **22.58kB** of data. The MCU hard locks with no timeout messages or error output.

| Component | Details |
|-----------|---------|
| MCU | RP2350A (Raspberry Pi Pico 2) |
| Ethernet | W5500 via SPI0 @ 20MHz |
| Camera | OV5640 via PIO + DMA |
| Frame size | 240x320 RGB565 = 153.6KB |
| TX buffer | 16KB (Socket 0) |

## Root Cause Analysis

### Identified Issue: DMA Channel Conflict

The freeze occurs inside `wiz_spi_writeburst()` at `network_wiznet5k.c:296-297`:

```c
dma_channel_wait_for_finish_blocking(dma_rx_chan);
dma_channel_wait_for_finish_blocking(dma_tx_chan);
```

These are **infinite blocking waits** with no timeout.

### Why DMA Never Completes

| Resource | Camera DMA | W5500 SPI DMA |
|----------|------------|---------------|
| IRQ | DMA_IRQ_0 | DMA_IRQ_0 |
| Channel | 1 | Dynamic |
| Trigger | Continuous (PIO) | On-demand (SPI) |

When the camera's `cam_handler()` IRQ fires during an active W5500 DMA transfer, the shared IRQ line causes corruption or starvation of the W5500 DMA completion signal.

### Evidence

- Freeze at 22.58kB = 16KB TX buffer + ~6KB retransmit overhead + HTTP headers
- Freeze happens during `WIZCHIP_WRITE_BUF()` call
- No timeout messages = code never returns from DMA wait

---

## Changes Made

### Phase 1: Timeout Protection (Partial Fix)

Added timeout helpers to `cam.c`:

```c
#define TX_WAIT_TIMEOUT_MS      5000
#define SENDOK_TIMEOUT_MS       3000
#define TX_POLL_INTERVAL_US     100

static inline uint32_t get_time_ms(void);
static inline bool timeout_elapsed(uint32_t start_ms, uint32_t timeout_ms);
```

Added timeout-protected SENDOK wait loops throughout `streaming_loop()`.

### Phase 2: DMA Isolation (Current Fix)

Added camera DMA control functions to `cam.c:332-346`:

```c
static void pause_camera_dma(void) {
    dma_channel_abort(dma_chan);
    irq_set_enabled(DMA_IRQ_0, false);
}

static void resume_camera_dma(void) {
    irq_set_enabled(DMA_IRQ_0, true);
    dma_channel_start(dma_chan);
}
```

Modified `streaming_loop()`:
- Pauses camera DMA at function start (line 369)
- Added `resume_camera_dma()` to ALL return paths (11 locations)

### Files Modified

| File | Changes |
|------|---------|
| `modules/camera/cam.c` | Timeout helpers, pause/resume DMA, fixed all return paths |

---

## Testing Protocol

### Test 1: Verify DMA Isolation Fix

1. Build and flash firmware
2. Connect to `http://172.16.1.1:8081/stream`
3. Observe streaming behavior

**Expected outcomes:**

| Result | Meaning | Next Action |
|--------|---------|-------------|
| Streaming works (static frame) | DMA conflict confirmed | Proceed to Phase 3 |
| Still freezes immediately | Issue is NOT DMA conflict | See Alternative Investigation |
| Freezes after delay | Partial fix, timing issue | Investigate interrupt latency |

### Console Output to Watch For

```
Pausing camera DMA to prevent SPI DMA conflict
Starting streaming loop on socket 0, TX buffer size: 16384
Socket 0 config: Mode=0x01, TX=16384, RX=16384, MSS=1460
Initial header sent, starting frame loop
Using static frame buffer (camera DMA paused)
Frame 1: TX_FSR=..., TX_WR=..., TX_RD=..., RX_RSR=...
```

If you see "Pausing camera DMA" but NOT "Starting streaming loop", the freeze happens during early W5500 register access (before any large DMA transfer).

---

## Phase 3: Permanent Solutions

Once DMA conflict is confirmed, choose one of these approaches:

### Option A: Keep Camera Paused During Stream (Current State)

**Status:** Already implemented

**Pros:**
- Simple, working
- No DMA conflicts possible

**Cons:**
- Streams static frame (no live video)
- Camera must be re-initialized after streaming ends

**Use case:** Acceptable for testing, not for production.

---

### Option B: Interleaved DMA Operation

Pause camera DMA around each W5500 SPI burst.

**Implementation in `streaming_loop()`:**

```c
// Before each WIZCHIP_WRITE_BUF call:
pause_camera_dma();
WIZCHIP_WRITE_BUF(addrsel, data, len);
resume_camera_dma();
```

**Pros:**
- Live video streaming
- Minimal code changes

**Cons:**
- Frame tearing possible if pause happens mid-capture
- Overhead from frequent pause/resume
- May miss camera frames

**Estimated effort:** 1 hour

---

### Option C: Separate IRQ Lines

Move camera DMA to DMA_IRQ_1, keep W5500 on DMA_IRQ_0.

**Implementation in `setup_dma_for_capture()` (cam.c:122):**

```c
// Change from:
irq_set_enabled(DMA_IRQ_0, false);
irq_set_exclusive_handler(DMA_IRQ_0, cam_handler);
dma_channel_set_irq0_enabled(dma_chan, true);
irq_set_enabled(DMA_IRQ_0, true);

// To:
irq_set_enabled(DMA_IRQ_1, false);
irq_set_exclusive_handler(DMA_IRQ_1, cam_handler);
dma_channel_set_irq1_enabled(dma_chan, true);
irq_set_enabled(DMA_IRQ_1, true);
```

**Pros:**
- Clean separation of concerns
- No runtime overhead
- Both DMAs can run simultaneously

**Cons:**
- Requires understanding of RP2350 DMA IRQ routing
- Must verify W5500 driver uses IRQ0 (not configurable)

**Estimated effort:** 30 minutes

**Recommended:** This is the cleanest solution.

---

### Option D: Disable W5500 DMA Entirely

Force all W5500 SPI transfers to use blocking non-DMA mode.

**Implementation in `network_wiznet5k.c:260`:**

```c
// Change DMA threshold from 1024 to effectively infinite:
#define WIZ_SPI_DMA_THRESHOLD 999999  // Was 1024
```

**Pros:**
- Simplest change (one line)
- Eliminates DMA conflict entirely

**Cons:**
- Slower SPI throughput
- Higher CPU usage during transfers
- May not achieve required frame rate

**Estimated effort:** 5 minutes

---

## Alternative Investigation

If the fix does NOT resolve the freeze:

### Check 1: SPI Peripheral State

Add debug output in `wiz_spi_writeburst()`:

```c
void wiz_spi_writeburst(uint8_t *buf, uint16_t len) {
    mp_printf(MP_PYTHON_PRINTER, "[SPI] writeburst len=%d\n", len);
    // ... existing code ...
    mp_printf(MP_PYTHON_PRINTER, "[SPI] DMA started, waiting...\n");
    dma_channel_wait_for_finish_blocking(dma_rx_chan);
    mp_printf(MP_PYTHON_PRINTER, "[SPI] RX DMA done\n");
    dma_channel_wait_for_finish_blocking(dma_tx_chan);
    mp_printf(MP_PYTHON_PRINTER, "[SPI] TX DMA done\n");
}
```

### Check 2: Add Watchdog

Force system reset on freeze for recovery:

```c
#include "hardware/watchdog.h"

// At start of streaming_loop():
watchdog_enable(5000, false);  // 5 second timeout

// In main loop, after each successful frame:
watchdog_update();
```

### Check 3: SPI Clock Speed

Reduce SPI clock to rule out signal integrity:

```c
// In W5500 init, change from 20MHz to 5MHz
spi_set_baudrate(spi0, 5000000);
```

---

## Code Locations Reference

| Function | File | Line |
|----------|------|------|
| `streaming_loop()` | cam.c | 352 |
| `pause_camera_dma()` | cam.c | 332 |
| `resume_camera_dma()` | cam.c | 340 |
| `setup_dma_for_capture()` | cam.c | 115 |
| `cam_handler()` | cam.c | 151 |
| `wiz_spi_writeburst()` | network_wiznet5k.c | 260 |

---

## Decision Matrix

| Priority | If Test Shows | Action |
|----------|---------------|--------|
| 1 | Streaming works | Implement Option C (separate IRQs) |
| 2 | Still freezes before first frame | Add SPI debug output, check init sequence |
| 3 | Freezes randomly after N frames | Timing issue, try Option D (disable DMA) |
| 4 | Works but slow | Tune chunk sizes, consider Option B |

---

## Changelog

| Date | Change |
|------|--------|
| 2026-01-20 | Created action plan |
| 2026-01-20 | Added `resume_camera_dma()` to all 11 return paths in `streaming_loop()` |
| 2026-01-20 | Previous session: Added timeout helpers, pause/resume functions |
| 2026-01-20 | **CRITICAL FIX:** Moved `pause_camera_dma()` to VERY FIRST line of `streaming_loop()` |

## Latest Finding (Test 1 Result)

**Symptom:** System freezes at "Pausing " - message cut off mid-output

**Analysis:**
- First `mp_printf` completed successfully ("Starting streaming loop...")
- Second `mp_printf` froze mid-output ("Pausing ")
- Between the two prints: `getSn_TxMAX()` was called, which uses SPI

**Conclusion:** 
The `getSn_TxMAX()` SPI call triggered the DMA conflict. The camera DMA IRQ fired during/after the SPI operation, corrupting state before `pause_camera_dma()` could be called.

**Fix Applied:**
Moved `pause_camera_dma()` to the absolute first line of `streaming_loop()`, before ANY other operations (including printf and SPI calls).

**Expected Result:**
With camera DMA paused first, all subsequent SPI operations should be safe from IRQ interference.
