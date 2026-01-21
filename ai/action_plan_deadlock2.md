# Action Plan: W5500 Second Request Deadlock Fix

## Problem Summary

The MCU hard-locks on the second `/svg` request. The deadlock occurs immediately after `[W5500] socket_accept: socket ESTABLISHED!` is printed, before `getSn_DIPR()` completes. Root cause analysis identified the SPI DMA mode register (`dmacr`) not being cleared as the most likely culprit.

---

## Fix 1: Defensive SPI DMA Mode Clearing

### Problem
The SPI peripheral's `dmacr` register may remain in DMA mode after a transfer, causing subsequent regular SPI operations to hang waiting for DMA signals.

### Location
`micropython/extmod/network_wiznet5k.c`

### Action Plan

#### Step 1.1: Add helper function to ensure SPI is not in DMA mode

**CRITICAL: This function must ONLY be called at high-level entry points where CS is NOT asserted.
Do NOT call it from low-level functions like `wiz_spi_read()` or `wiz_spi_writeburst()` as they
are called mid-transaction with CS already low and header already sent.**

Insert after line 241 (after `wiz_cs_deselect`):

```c
// === DEFENSIVE SPI DMA MODE CLEARING ===
// Call this ONLY at high-level entry points BEFORE CS is asserted
// Do NOT call this mid-transaction (i.e., not in wiz_spi_read/wiz_spi_writeburst)
static inline void wiz_ensure_spi_ready(void) {
    machine_spi_obj_t *spi_obj = (machine_spi_obj_t *)wiznet5k_obj.spi;
    if (spi_obj && spi_obj->spi_inst) {
        // Clear DMA control register - ensures SPI is in normal mode
        // This is the critical fix: if dmacr is left set from a previous DMA transfer,
        // subsequent regular SPI transfers will hang waiting for DMA signals
        spi_get_hw(spi_obj->spi_inst)->dmacr = 0;
        
        // Memory barrier to ensure the write completes before we proceed
        __compiler_memory_barrier();
    }
}
```

#### Step 1.2: Call the helper at CORRECT entry points

**WARNING: The original plan incorrectly suggested adding the call to `wiz_spi_read()` and 
`wiz_spi_writeburst()`. This is WRONG because these functions are called mid-SPI-transaction
(CS is already asserted, header already sent). Calling the helper there corrupts transactions.**

The CORRECT locations for `wiz_ensure_spi_ready()` are:

| Function | Location | Reason |
|----------|----------|--------|
| `w5500_dma_write_to_txbuf()` | At function start (before CS assert) | Before DMA transfer starts |
| `wiznet5k_socket_accept()` | Before `getSn_DIPR()` calls | Clean state before register reads |

**DO NOT** add to:
- `wiz_spi_read()` - called mid-transaction
- `wiz_spi_writeburst()` - called mid-transaction
- `wiz_spi_readbyte()` - called mid-transaction
- `wiz_spi_writebyte()` - called mid-transaction

#### Step 1.3: Add dmacr clearing in `w5500_dma_write_to_txbuf()`

```c
static void w5500_dma_write_to_txbuf(uint8_t sn, uint16_t offset, const uint8_t* data, uint16_t len) {
    wiz_ensure_spi_ready();  // CORRECT: Called before CS is asserted
    if (len == 0 || dma_tx_chan < 0) return;
    // ... rest of function (this function manages its own CS)
}
```

### Verification
- First request should complete successfully
- Second request should NOT hang at `getSn_DIPR()`
- Debug print should appear: `socket_accept: client IP: x.x.x.x`

---

## Fix 2: Use-After-Free in Transfer Completion Wait

### Problem
`w5500_wait_transfer_complete()` accesses `transfer->state` after `w5500_tx_service()` may have released the transfer back to the pool.

### Location
`micropython/extmod/network_wiznet5k.c`, lines 618-633

### Action Plan

#### Step 2.1: Copy state before potential release

Replace the entire `w5500_wait_transfer_complete()` function:

```c
// FIXED: Helper function to wait for a specific transfer to complete
// Now properly handles potential use-after-free by copying state before release
static int w5500_wait_transfer_complete(w5500_tx_transfer_t* transfer, uint32_t timeout_ms) {
    uint32_t start = mp_hal_ticks_ms();
    tx_state_t last_known_state = transfer->state;
    
    while (last_known_state != TX_COMPLETE && last_known_state != TX_ERROR) {
        // Service transfers - this may release our transfer!
        w5500_tx_service();
        
        // Check if transfer is still in the active list before accessing
        bool still_active = false;
        w5500_tx_transfer_t* check = active_transfers;
        while (check) {
            if (check == transfer) {
                still_active = true;
                last_known_state = transfer->state;
                break;
            }
            check = check->next;
        }
        
        // If transfer was removed from active list, it completed or errored
        if (!still_active) {
            // Transfer was dequeued - check the copied state
            break;
        }
        
        if (timeout_ms > 0 && (mp_hal_ticks_ms() - start) >= timeout_ms) {
            DEBUG_PRINT("w5500_wait_transfer_complete: timeout after %d ms\n", timeout_ms);
            // Mark as error and remove from queue
            transfer->state = TX_ERROR;
            w5500_dequeue_transfer(transfer);
            w5500_release_transfer(transfer);
            return -1;  // Timeout
        }
        
        mp_hal_delay_ms(1);
    }
    
    return (last_known_state == TX_COMPLETE) ? 0 : -1;
}
```

#### Step 2.2: Update caller to not access transfer after wait

In `wiznet5k_socket_send()` around line 1462-1475, modify:

```c
    // CLAUDE FIX: Wait for transfer to complete (synchronous!)
    uint32_t timeout_ms = (socket->timeout > 0) ? socket->timeout : 30000;
    
    // Save bytes_sent before wait (in case transfer is released)
    uint32_t expected_bytes = transfer->total_size;
    
    int result = w5500_wait_transfer_complete(transfer, timeout_ms);
    
    // After this point, 'transfer' pointer may be invalid!
    // Do NOT access transfer->bytes_sent or any other field
    
    if (result < 0) {
        DEBUG_PRINT("socket_send: transfer failed or timed out\n");
        *_errno = MP_EIO;
        return -1;
    }
    
    DEBUG_PRINT("socket_send: transfer complete, sent %d bytes\n", expected_bytes);
    return expected_bytes;
```

### Verification
- No crashes or memory corruption during large transfers
- Multiple sequential requests should work correctly

---

## Fix 3: DMA Channel Claim Consistency

### Problem
`wiz_spi_writeburst()` uses `dma_claim_unused_channel(true)` which panics if no channel is available, while `w5500_dma_write_to_txbuf()` uses `false` and falls back gracefully.

### Location
`micropython/extmod/network_wiznet5k.c`, line 267

### Action Plan

#### Step 3.1: Change panic behavior to graceful fallback

Replace line 267 in `wiz_spi_writeburst()`:

```c
static void wiz_spi_writeburst(const uint8_t* pBuf, uint16_t len) {
    if (len >= 1024 && dma_tx_chan >= 0) {
        machine_spi_obj_t *spi_obj = (machine_spi_obj_t *)wiznet5k_obj.spi;
        spi_inst_t *spi_inst = spi_obj->spi_inst;

        // FIXED: Use false to avoid panic, fall back to regular SPI if unavailable
        int dma_rx_chan = dma_claim_unused_channel(false);
        if (dma_rx_chan < 0) {
            // No RX channel available - fall back to regular SPI
            DEBUG_PRINT("wiz_spi_writeburst: no DMA RX channel, using regular SPI\n");
            wiznet5k_obj.spi_transfer(wiznet5k_obj.spi, len, pBuf, NULL);
            return;
        }
        // ... rest of DMA code
```

### Verification
- System should not panic when DMA channels are exhausted
- Large transfers should fall back to regular SPI gracefully

---

## Fix 4: Add Timeout to DMA Blocking Waits

### Problem
`dma_channel_wait_for_finish_blocking()` has no timeout - if DMA never completes, the system hangs forever.

### Location
`micropython/extmod/network_wiznet5k.c`, lines 296-297, 452-453

### Action Plan

#### Step 4.1: Create timeout-enabled DMA wait helper

Insert after line 340 (after `w5500_dma_init`):

```c
// DMA wait with timeout - returns true if completed, false if timed out
#define DMA_WAIT_TIMEOUT_MS 1000

static bool dma_channel_wait_with_timeout(int channel, uint32_t timeout_ms) {
    uint32_t start = mp_hal_ticks_ms();
    
    while (dma_channel_is_busy(channel)) {
        if ((mp_hal_ticks_ms() - start) >= timeout_ms) {
            DEBUG_PRINT("DMA channel %d timeout after %d ms - aborting\n", channel, timeout_ms);
            dma_channel_abort(channel);
            return false;
        }
        tight_loop_contents();
    }
    return true;
}

// Wait for both TX and RX DMA channels with cleanup on timeout
static bool dma_wait_both_with_timeout(int rx_chan, int tx_chan, uint32_t timeout_ms) {
    uint32_t start = mp_hal_ticks_ms();
    
    // Wait for RX first (it completes after TX for SPI)
    while (dma_channel_is_busy(rx_chan)) {
        if ((mp_hal_ticks_ms() - start) >= timeout_ms) {
            DEBUG_PRINT("DMA RX timeout - aborting both channels\n");
            dma_channel_abort(rx_chan);
            dma_channel_abort(tx_chan);
            return false;
        }
        tight_loop_contents();
    }
    
    // TX should already be done, but check anyway
    while (dma_channel_is_busy(tx_chan)) {
        if ((mp_hal_ticks_ms() - start) >= timeout_ms) {
            DEBUG_PRINT("DMA TX timeout - aborting\n");
            dma_channel_abort(tx_chan);
            return false;
        }
        tight_loop_contents();
    }
    
    return true;
}
```

#### Step 4.2: Replace blocking waits in wiz_spi_writeburst()

Replace lines 296-297:

```c
        // OLD:
        // dma_channel_wait_for_finish_blocking(dma_rx_chan);
        // dma_channel_wait_for_finish_blocking(dma_tx_chan);
        
        // NEW: Wait with timeout
        bool dma_ok = dma_wait_both_with_timeout(dma_rx_chan, dma_tx_chan, DMA_WAIT_TIMEOUT_MS);
        
        __compiler_memory_barrier();
        // Disable SPI DMA - do this ALWAYS, even on timeout
        spi_get_hw(spi_obj->spi_inst)->dmacr = 0;
        
        dma_channel_unclaim(dma_rx_chan);
        
        if (!dma_ok) {
            DEBUG_PRINT("wiz_spi_writeburst: DMA failed, SPI may be in bad state\n");
            wiz_ensure_spi_ready();  // Try to recover
        }
```

#### Step 4.3: Replace blocking waits in w5500_dma_write_to_txbuf()

Replace lines 451-458:

```c
        // OLD:
        // dma_channel_wait_for_finish_blocking(dma_rx_chan);
        // dma_channel_wait_for_finish_blocking(dma_tx_chan);
        
        // NEW: Wait with timeout
        bool dma_ok = dma_wait_both_with_timeout(dma_rx_chan, dma_tx_chan, DMA_WAIT_TIMEOUT_MS);

        __compiler_memory_barrier();

        // === DISABLE SPI DMA MODE (CRITICAL - do this ALWAYS!) ===
        spi_get_hw(spi_inst)->dmacr = 0;

        // Release RX channel
        dma_channel_unclaim(dma_rx_chan);
        
        if (!dma_ok) {
            DEBUG_PRINT("w5500_dma_write_to_txbuf: DMA timeout, attempting recovery\n");
            wiz_ensure_spi_ready();
        }
```

### Verification
- System should recover from DMA hangs instead of locking
- Debug messages should indicate timeout if it occurs
- SPI should be usable after timeout recovery

---

## Fix 5: Add Critical Section Protection for DMA Operations

### Problem
DMA and SPI operations are not protected from interrupts, allowing concurrent access that can corrupt state.

### Location
`micropython/extmod/network_wiznet5k.c`

### Action Plan

#### Step 5.1: Wrap DMA operations in critical sections

In `w5500_dma_write_to_txbuf()`, add critical section around the entire DMA operation:

```c
static void w5500_dma_write_to_txbuf(uint8_t sn, uint16_t offset, const uint8_t* data, uint16_t len) {
    if (len == 0 || dma_tx_chan < 0) {
        wiz_ensure_spi_ready();
        return;
    }

    // Enter critical section for entire DMA operation
    wiz_cris_enter();
    
    // ... existing code for DMA setup and transfer ...
    
    // Get SPI object
    machine_spi_obj_t *spi_obj = (machine_spi_obj_t *)wiznet5k_obj.spi;
    spi_inst_t *spi_inst = spi_obj->spi_inst;
    
    // ... rest of function ...
    
    // At the very end, after CS deassert:
    wiz_cs_deselect();
    
    // Exit critical section
    wiz_cris_exit();

    DEBUG_PRINT("w5500_dma_write_to_txbuf: wrote %d bytes to socket %d offset %d\n", len, sn, offset);
}
```

#### Step 5.2: Add critical section to wiz_spi_writeburst()

```c
static void wiz_spi_writeburst(const uint8_t* pBuf, uint16_t len) {
    if (len >= 1024 && dma_tx_chan >= 0) {
        wiz_cris_enter();  // ADD: Protect DMA operation
        
        // ... existing DMA code ...
        
        dma_channel_unclaim(dma_rx_chan);
        
        wiz_cris_exit();  // ADD: End protection
    } else {
        wiznet5k_obj.spi_transfer(wiznet5k_obj.spi, len, pBuf, NULL);
    }
}
```

### Verification
- No corruption when interrupts fire during DMA transfers
- System remains stable under high interrupt load

---

## Fix 6: Prevent Recursive Yield Reentrancy

### Problem
`mpy_wiznet_yield()` -> `w5500_tx_service()` -> `mpy_wiznet_yield()` creates unbounded recursion.

### Location
`micropython/extmod/network_wiznet5k.c`, lines 244-254 and 556-558

### Action Plan

#### Step 6.1: Add reentrancy guard to mpy_wiznet_yield()

```c
static volatile bool in_wiznet_yield = false;

void mpy_wiznet_yield(void) {
    // Prevent reentrancy
    if (in_wiznet_yield) {
        return;
    }
    in_wiznet_yield = true;
    
    #if MICROPY_PY_THREAD
    MICROPY_THREAD_YIELD();
    #else
    mp_handle_pending(true);
    #endif
    
    // Service zero-copy transfers
    w5500_tx_service();
    
    in_wiznet_yield = false;
}
```

#### Step 6.2: Alternative - Remove yield from tx_service inner loop

In `w5500_tx_service()`, replace line 556-558:

```c
            // Wait for command to register
            // OLD: while (getSn_CR(transfer->socket_num)) { mpy_wiznet_yield(); }
            
            // NEW: Simple busy wait without yield (command register clears quickly)
            uint32_t cmd_start = mp_hal_ticks_ms();
            while (getSn_CR(transfer->socket_num)) {
                if (mp_hal_ticks_ms() - cmd_start > 100) {
                    DEBUG_PRINT("w5500_tx_service: CR wait timeout\n");
                    break;
                }
                tight_loop_contents();
            }
```

### Verification
- No stack overflow from recursive calls
- Transfer state machine progresses correctly

---

## Fix 7: Improve Socket Re-Listen Timing

### Problem
The 250ms timeout for socket disconnect may not be sufficient, causing re-listen to fail.

### Location
`micropython/extmod/network_wiznet5k.c`, lines 1040-1051

### Action Plan

#### Step 7.1: Increase timeout and add state verification

Replace lines 1040-1051:

```c
        // IMPROVED: Graceful disconnect with proper state verification
        if (getSn_SR(sn) == SOCK_ESTABLISHED) {
            WIZCHIP_EXPORT(disconnect)(sn);
            
            // Wait for transition to CLOSED with longer timeout
            uint32_t start = mp_hal_ticks_ms();
            uint8_t sr;
            while ((sr = getSn_SR(sn)) != SOCK_CLOSED && 
                   sr != SOCK_TIME_WAIT &&
                   mp_hal_ticks_ms() - start < 500) {  // Increased to 500ms
                mp_hal_delay_ms(10);
            }
            
            DEBUG_PRINT("socket_close: disconnect complete, state=0x%02x after %d ms\n",
                        sr, mp_hal_ticks_ms() - start);
        }
        
        // Force close if not already closed
        if (getSn_SR(sn) != SOCK_CLOSED) {
            WIZCHIP_EXPORT(close)(sn);
        }
        
        // Wait for CLOSED state before re-listening
        uint32_t close_start = mp_hal_ticks_ms();
        while (getSn_SR(sn) != SOCK_CLOSED && mp_hal_ticks_ms() - close_start < 200) {
            mp_hal_delay_ms(5);
        }
        
        if (getSn_SR(sn) != SOCK_CLOSED) {
            DEBUG_PRINT("socket_close: WARNING - socket not in CLOSED state before re-listen\n");
        }
```

### Verification
- Socket consistently reaches CLOSED state before re-listen
- Re-listen succeeds on first attempt

---

## Fix 8: Make dev_null Static in All Functions

### Problem
`wiz_spi_writeburst()` uses stack variable for DMA RX destination, which could cause stack corruption if interrupted.

### Location
`micropython/extmod/network_wiznet5k.c`, line 274

### Action Plan

#### Step 8.1: Make dev_null static

Replace line 274:

```c
        // OLD: uint8_t dev_null;
        // NEW: Static to ensure it's always valid during DMA
        static uint8_t dev_null;  // Dummy variable for RX data
```

### Verification
- No stack corruption during DMA transfers

---

## Fix 9: Clear SPI State Before Socket Accept

### Problem
Belt-and-suspenders approach to ensure SPI is clean before critical operations.

### Location
`micropython/extmod/network_wiznet5k.c`, `wiznet5k_socket_accept()` function

### Action Plan

#### Step 9.1: Add SPI cleanup before reading client info

In `wiznet5k_socket_accept()`, after line 1323 (`DEBUG_PRINT("socket_accept: socket ESTABLISHED!\n")`):

```c
            DEBUG_PRINT("socket_accept: socket ESTABLISHED!\n");
            
            // DEFENSIVE: Ensure SPI is in clean state before reading client info
            wiz_ensure_spi_ready();

            // Get client info
            getSn_DIPR(sn, ip);
            *port = getSn_DPORT(sn);
```

Apply same fix at line 1259 for the immediate-accept path:

```c
        // DEFENSIVE: Ensure SPI is ready
        wiz_ensure_spi_ready();
        
        // Get client info
        getSn_DIPR(sn, ip);
        *port = getSn_DPORT(sn);
```

### Verification
- Second request no longer hangs at `getSn_DIPR()`
- Client IP is successfully read and printed

---

## Implementation Priority

| Priority | Fix | Effort | Impact |
|----------|-----|--------|--------|
| 1 | Fix 1: Defensive SPI DMA clearing | Medium | **Critical** - Most likely root cause |
| 2 | Fix 9: Clear SPI before accept | Low | **High** - Direct fix for symptom |
| 3 | Fix 4: DMA timeout | Medium | **High** - Prevents future hangs |
| 4 | Fix 2: Use-after-free | Medium | **High** - Prevents memory corruption |
| 5 | Fix 6: Recursive yield | Low | **Medium** - Prevents stack overflow |
| 6 | Fix 3: DMA claim consistency | Low | **Medium** - Prevents panics |
| 7 | Fix 5: Critical sections | Medium | **Medium** - Race condition prevention |
| 8 | Fix 7: Re-listen timing | Low | **Low** - Edge case improvement |
| 9 | Fix 8: Static dev_null | Low | **Low** - Belt-and-suspenders |

---

## Testing Checklist

After implementing fixes:

1. [ ] First `/svg` request returns complete 52KB file
2. [ ] Second `/svg` request succeeds without hang
3. [ ] Third, fourth, fifth requests all succeed
4. [ ] Rapid repeated requests (browser refresh spam) work
5. [ ] No debug messages about DMA timeout
6. [ ] No debug messages about transfer errors
7. [ ] Memory usage stays stable across requests
8. [ ] System can run for extended period serving requests

---

## Rollback Plan

If fixes cause new issues:

1. Each fix is independent - can be reverted individually
2. Add `#define W5500_LEGACY_DMA 1` to disable all DMA improvements
3. Fall back to pure blocking SPI (remove all DMA code paths)
