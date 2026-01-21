# Action Plan: Fix W5500 Reentrant Double-SEND Bug

## STATUS: FAILED

**Implementation Date:** 2026-01-22  
**Result:** Fix implemented but problem persists - MCU still freezes on second request.  
**Conclusion:** The reentrance/double-SEND hypothesis was either **wrong** or **incomplete**. The root cause lies elsewhere.

---

## What Was Tried

All 4 changes were implemented in `micropython/extmod/network_wiznet5k.c`:

| Change | Location | Status |
|--------|----------|--------|
| Reentrance guard variable | Line 215-216 | Implemented |
| Guard check at entry | Line 620-624 | Implemented |
| Guard reset at exit | Line 739 | Implemented |
| State transition before yield | Line 677-678 | Implemented |

**Observation:** Second request still causes MCU hard lock, regardless of time between requests.

---

## Next Investigation Directions

The double-SEND hypothesis is ruled out. Need to investigate other potential causes:

### 1. Transfer Pool Exhaustion
- Check if `transfer_pool` is properly returning transfers after completion
- Verify `w5500_release_transfer()` is actually being called
- Add debug prints to track pool state

### 2. Socket State Corruption
- The socket may not be properly reset between connections
- Check `SOCK_CLOSE_WAIT` handling and socket cleanup
- Investigate if old transfer descriptors reference stale socket state

### 3. DMA Channel Issues
- DMA channel may not be properly reset between transfers
- Check if `dma_tx_chan` or temporary `dma_rx_chan` has stale state
- Verify `dma_channel_unclaim()` is working correctly

### 4. W5500 TX Buffer Pointer Corruption (Different Cause)
- TX_WR/TX_RD pointers may be corrupted via a different path
- Check if pointers are reset when socket is closed/reopened
- Verify `getSn_TX_WR()` returns expected values on second request

### 5. SPI Bus Contention
- Multiple callers may be accessing SPI without proper synchronization
- Check `wiz_cris_enter()`/`wiz_cris_exit()` usage
- Look for missing critical sections

### 6. Incomplete Transfer Cleanup
- Active transfer may not be removed when socket closes
- Check if `active_transfers` list is cleared on socket close
- Verify no dangling pointers exist

---

## Suggested Debug Steps

1. **Add extensive logging** to track:
   - Transfer pool allocations/releases
   - Socket state transitions
   - TX_WR pointer values at each step
   - DMA channel states

2. **Compare first vs second request**:
   - Log all W5500 register values at start of `/svg` handler
   - Identify which registers differ on second request

3. **Check for infinite loops**:
   - Add timeout to `while (getSn_CR(sn))` loop
   - Add timeout to `TX_SENDING_PACKET` state (waiting for SENDOK)

4. **Verify cleanup**:
   - Log when transfers are completed/released
   - Confirm pool is replenished after first request

---

## Original Hypothesis (Disproven)

~~The `w5500_tx_service()` function is called recursively during the `TX_TRANSFERRING_CHUNK` state, causing double SEND commands and W5500 internal state corruption.~~

This was addressed by:
- Adding reentrance guard to prevent recursive processing
- Moving state transition before the yield loop

**Result:** Problem persists, indicating the root cause is different.

---

## Archived Implementation Details

<details>
<summary>Original fix implementation (click to expand)</summary>

### The Bug Flow (Hypothesized)

```
w5500_tx_service() enters TX_TRANSFERRING_CHUNK
    -> setSn_CR(sn, Sn_CR_SEND)                 // First SEND command
    -> while (getSn_CR(sn)) loop
        -> mpy_wiznet_yield()
            -> w5500_tx_service()              // RECURSIVE CALL
                -> sees same transfer in TX_TRANSFERRING_CHUNK
                -> setSn_CR(sn, Sn_CR_SEND)    // SECOND SEND command!
                -> updates TX_WR again         // POINTER CORRUPTION!
```

### Code Changes Made

#### Change 1: Add reentrance guard (line 215-216)

```c
// Reentrance guard to prevent double-SEND bug
static volatile bool tx_service_active = false;
```

#### Change 2: Guard at function entry (line 620-624)

```c
static void w5500_tx_service(void) {
    // Prevent reentrant calls that cause W5500 double-SEND corruption
    if (tx_service_active) {
        return;
    }
    tx_service_active = true;
    // ...
```

#### Change 3: Guard reset at function exit (line 739)

```c
    tx_service_active = false;
}
```

#### Change 4: Move state transition earlier (line 677-678)

```c
case TX_TRANSFERRING_CHUNK: {
    uint16_t new_tx_wr = getSn_TX_WR(transfer->socket_num) + transfer->chunk_size;
    setSn_TX_WR(transfer->socket_num, new_tx_wr);
    setSn_CR(transfer->socket_num, Sn_CR_SEND);

    // CRITICAL: Change state BEFORE yield to prevent double-SEND on reentry
    transfer->state = TX_SENDING_PACKET;

    // Wait for command to register
    while (getSn_CR(transfer->socket_num)) {
        mpy_wiznet_yield();
    }
    // ...
}
```

</details>
