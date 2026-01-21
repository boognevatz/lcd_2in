# Action Plan: Fix RX Buffer State Not Cleared (Issue #3)

## Problem Summary

When the first HTTP connection closes and the socket re-listens, the W5500's RX buffer retains **333 bytes** of stale data from the previous connection. This is proven by Wireshark packet 80 showing `Win=16051` instead of `Win=16384` (16384 - 333 = 16051).

The stale RX buffer causes the second connection to either:
- Read garbage data from the previous request
- Deadlock waiting for "new" data that's already in the buffer
- Corrupt the TCP state machine

---

## Implementation Steps

### Step 1: Create RX Buffer Drain Helper Function

**File:** `micropython/extmod/network_wiznet5k.c`

**Location:** Add after line 500 (after `w5500_send_chunk` function)

**Add this new function:**

```c
/*
 * Drain any remaining data from socket RX buffer.
 * This must be called before re-listening to ensure clean state.
 * 
 * Parameters:
 *   sn - socket number (0-7)
 * 
 * Returns:
 *   Total bytes drained from buffer
 */
static uint32_t w5500_drain_rx_buffer(uint8_t sn) {
    uint32_t total_drained = 0;
    uint16_t rx_size;
    
    // Keep reading until RX buffer is empty
    while ((rx_size = getSn_RX_RSR(sn)) > 0) {
        // Calculate how much to read this iteration (max 256 bytes at a time)
        uint16_t to_read = (rx_size > 256) ? 256 : rx_size;
        
        // We don't need the data, just need to advance the read pointer
        // Read into a small static buffer and discard
        static uint8_t drain_buf[256];
        
        // Use WIZCHIP_EXPORT(recv) to properly update RX pointers
        mp_int_t ret = WIZCHIP_EXPORT(recv)(sn, drain_buf, to_read);
        
        if (ret <= 0) {
            // Error or no more data - stop draining
            DEBUG_PRINT("w5500_drain_rx_buffer: recv returned %d, stopping\\n", ret);
            break;
        }
        
        total_drained += ret;
        
        // Safety limit - don't drain more than 64KB (prevents infinite loop)
        if (total_drained > 65536) {
            DEBUG_PRINT("w5500_drain_rx_buffer: safety limit reached\\n");
            break;
        }
    }
    
    if (total_drained > 0) {
        DEBUG_PRINT("w5500_drain_rx_buffer: drained %d bytes from socket %d\\n", 
                    total_drained, sn);
    }
    
    return total_drained;
}
```

---

### Step 2: Add Forward Declaration

**File:** `micropython/extmod/network_wiznet5k.c`

**Location:** Add at line 217 (in the forward declarations section)

**Add this line:**

```c
static uint32_t w5500_drain_rx_buffer(uint8_t sn);
```

---

### Step 3: Modify socket_close() to Drain RX Buffer Before Re-Listen

**File:** `micropython/extmod/network_wiznet5k.c`

**Location:** Lines 1056-1060 (inside `wiznet5k_socket_close` function)

**Find this code block:**

```c
        // Re-listen if this was a listening socket that accepted a connection
        if (can_relisten && saved_port != 0) {
            DEBUG_PRINT("socket_close: re-listening on port %d\n", saved_port);

            mp_int_t ret = WIZCHIP_EXPORT(socket)(sn, saved_type, saved_port, 0);
```

**Replace with:**

```c
        // Re-listen if this was a listening socket that accepted a connection
        if (can_relisten && saved_port != 0) {
            DEBUG_PRINT("socket_close: re-listening on port %d\n", saved_port);

            // CRITICAL FIX: Drain any remaining RX data before re-listening
            // This prevents stale data from the previous connection polluting
            // the new connection's RX buffer
            uint8_t sr_before_drain = getSn_SR(sn);
            if (sr_before_drain != SOCK_CLOSED) {
                // Socket not fully closed yet - drain the buffer first
                uint32_t drained = w5500_drain_rx_buffer(sn);
                if (drained > 0) {
                    DEBUG_PRINT("socket_close: drained %d stale bytes before re-listen\n", drained);
                }
            }

            mp_int_t ret = WIZCHIP_EXPORT(socket)(sn, saved_type, saved_port, 0);
```

---

### Step 4: Add RX Buffer Drain After Socket Close Command

**File:** `micropython/extmod/network_wiznet5k.c`

**Location:** Lines 1048-1051 (inside `wiznet5k_socket_close` function)

**Find this code block:**

```c
        WIZCHIP_EXPORT(close)(sn);
        // Give it a moment to transition to ESTABLISHED state
        mp_hal_delay_ms(50);
```

**Replace with:**

```c
        WIZCHIP_EXPORT(close)(sn);
        // Give it a moment to transition to CLOSED state
        mp_hal_delay_ms(50);

        // Verify socket reached CLOSED state
        uint8_t sr_after_close = getSn_SR(sn);
        int close_wait_count = 0;
        while (sr_after_close != SOCK_CLOSED && close_wait_count < 20) {
            mp_hal_delay_ms(25);
            sr_after_close = getSn_SR(sn);
            close_wait_count++;
        }
        
        if (sr_after_close != SOCK_CLOSED) {
            DEBUG_PRINT("socket_close: WARNING - socket not fully closed, state=0x%02x\n", 
                        sr_after_close);
        }
```

---

### Step 5: Add RX Buffer Validation in socket_accept()

**File:** `micropython/extmod/network_wiznet5k.c`

**Location:** Lines 1320-1340 (inside `wiznet5k_socket_accept` function, after connection is established)

**Find this code block:**

```c
            // Verify we're now in ESTABLISHED state
            sr = getSn_SR(sn);
            if (sr == SOCK_ESTABLISHED) {
                DEBUG_PRINT("socket_accept: socket ESTABLISHED!\n");

                // Get client info
                getSn_DIPR(sn, ip);
                *port = getSn_DPORT(sn);
```

**Replace with:**

```c
            // Verify we're now in ESTABLISHED state
            sr = getSn_SR(sn);
            if (sr == SOCK_ESTABLISHED) {
                DEBUG_PRINT("socket_accept: socket ESTABLISHED!\n");

                // CRITICAL FIX: Check RX buffer state for debugging
                uint16_t rx_rsr = getSn_RX_RSR(sn);
                uint16_t tx_fsr = getSn_TX_FSR(sn);
                DEBUG_PRINT("socket_accept: RX_RSR=%d, TX_FSR=%d (expected: RX=0, TX=16384)\n",
                            rx_rsr, tx_fsr);
                
                // If RX buffer already has data, this is suspicious
                // (data arrived before we officially accepted - this is normal)
                // But if TX_FSR is not at max, we have stale TX state
                if (tx_fsr < getSn_TxMAX(sn)) {
                    DEBUG_PRINT("socket_accept: WARNING - TX buffer not fully free!\n");
                }

                // Get client info
                getSn_DIPR(sn, ip);
                *port = getSn_DPORT(sn);
```

---

### Step 6: Add TX Buffer Reset on Re-Listen

**File:** `micropython/extmod/network_wiznet5k.c`

**Location:** After the `WIZCHIP_EXPORT(socket)` call in re-listen block (around line 1062)

**Find this code block:**

```c
            mp_int_t ret = WIZCHIP_EXPORT(socket)(sn, saved_type, saved_port, 0);
            DEBUG_PRINT("socket_close: WIZCHIP_EXPORT(socket) returned %d\n", ret);

            if (ret >= 0) {
```

**Replace with:**

```c
            mp_int_t ret = WIZCHIP_EXPORT(socket)(sn, saved_type, saved_port, 0);
            DEBUG_PRINT("socket_close: WIZCHIP_EXPORT(socket) returned %d\n", ret);

            if (ret >= 0) {
                // Verify buffer state is clean after socket reopen
                uint16_t rx_rsr = getSn_RX_RSR(sn);
                uint16_t tx_fsr = getSn_TX_FSR(sn);
                uint16_t tx_max = getSn_TxMAX(sn);
                
                DEBUG_PRINT("socket_close: after reopen - RX_RSR=%d, TX_FSR=%d/%d\n",
                            rx_rsr, tx_fsr, tx_max);
                
                // RX should be 0, TX should be at maximum
                if (rx_rsr != 0 || tx_fsr != tx_max) {
                    DEBUG_PRINT("socket_close: WARNING - buffers not clean after reopen!\n");
                    
                    // Force a small delay to let W5500 settle
                    mp_hal_delay_ms(100);
                    
                    // Re-check
                    rx_rsr = getSn_RX_RSR(sn);
                    tx_fsr = getSn_TX_FSR(sn);
                    DEBUG_PRINT("socket_close: after settle - RX_RSR=%d, TX_FSR=%d/%d\n",
                                rx_rsr, tx_fsr, tx_max);
                }
```

---

### Step 7: Alternative - Full Socket Reset Function (Optional)

If the above changes don't fully resolve the issue, create a comprehensive reset function.

**File:** `micropython/extmod/network_wiznet5k.c`

**Location:** Add after `w5500_drain_rx_buffer` function

**Add this function:**

```c
/*
 * Perform a complete socket reset to ensure clean state.
 * This closes the socket, waits for full closure, and clears all buffers.
 * 
 * Parameters:
 *   sn - socket number (0-7)
 * 
 * Returns:
 *   0 on success, -1 on failure
 */
static int w5500_full_socket_reset(uint8_t sn) {
    DEBUG_PRINT("w5500_full_socket_reset: socket %d\n", sn);
    
    // Step 1: If socket is connected, try graceful disconnect first
    uint8_t sr = getSn_SR(sn);
    if (sr == SOCK_ESTABLISHED || sr == SOCK_CLOSE_WAIT) {
        DEBUG_PRINT("w5500_full_socket_reset: disconnecting...\n");
        WIZCHIP_EXPORT(disconnect)(sn);
        
        // Wait up to 500ms for disconnect
        uint32_t start = mp_hal_ticks_ms();
        while (getSn_SR(sn) != SOCK_CLOSED && (mp_hal_ticks_ms() - start) < 500) {
            mp_hal_delay_ms(10);
        }
    }
    
    // Step 2: Force close the socket
    sr = getSn_SR(sn);
    if (sr != SOCK_CLOSED) {
        DEBUG_PRINT("w5500_full_socket_reset: force closing (state=0x%02x)\n", sr);
        WIZCHIP_EXPORT(close)(sn);
        
        // Wait up to 200ms for close
        uint32_t start = mp_hal_ticks_ms();
        while (getSn_SR(sn) != SOCK_CLOSED && (mp_hal_ticks_ms() - start) < 200) {
            mp_hal_delay_ms(10);
        }
    }
    
    // Step 3: Verify socket is closed
    sr = getSn_SR(sn);
    if (sr != SOCK_CLOSED) {
        DEBUG_PRINT("w5500_full_socket_reset: FAILED - socket still in state 0x%02x\n", sr);
        return -1;
    }
    
    // Step 4: Clear any interrupt flags
    setSn_IR(sn, 0xFF);  // Clear all interrupt flags
    
    // Step 5: Verify buffers are empty (they should be after close)
    uint16_t rx_rsr = getSn_RX_RSR(sn);
    uint16_t tx_fsr = getSn_TX_FSR(sn);
    
    DEBUG_PRINT("w5500_full_socket_reset: final state - RX_RSR=%d, TX_FSR=%d\n", 
                rx_rsr, tx_fsr);
    
    // Small delay to ensure W5500 internal state is settled
    mp_hal_delay_ms(10);
    
    return 0;
}
```


