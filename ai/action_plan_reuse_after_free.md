**EUREKA! Found the root cause!** 🎯

Looking at your debug output, the smoking gun is crystal clear:

```
[W5500] socket_close: before - state=0x1c (CLOSE_WAIT)
[W5500] socket_close: after - state=0x00 (CLOSED)
[W5500] socket_close: re-listening on port 80
[W5500] socket_close: WIZCHIP_EXPORT(socket) returned 0
[W5500] socket_close: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_close: successfully re-listened
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial state - socket 0: 0x14 (LISTEN)
[W5500] w5500_tx_service: Transfer complete on socket 0, 52467 bytes sent  ← AFTER accept!
[W5500] socket_accept: loop 100 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
```

## The Bug: Race Condition Between Transfer Completion and Socket Re-use

**What's happening:**

1. First request: Socket 0 sends 52,467 bytes successfully ✅
2. Your Python calls `cl.close()` → triggers `socket_close()` ✅
3. Socket 0 transitions to CLOSED, then **immediately re-listens** ✅
4. **BUT**: The background `w5500_tx_service()` task is **still running** from the previous transfer!
5. When you call `socket_accept()` for the second request, the socket is in LISTEN state
6. Client connects → socket goes to ESTABLISHED
7. **BOOM**: The old transfer's `w5500_tx_service()` callback fires and **tries to complete a transfer on the now-connected socket**
8. This corrupts the socket state → hard freeze

---

## Root Cause: Asynchronous Transfer Not Properly Cleaned Up

In `network_wiznet5k.c`, look at your `socket_close()`:

```c
static void wiznet5k_socket_close(mod_network_socket_obj_t *socket) {
    // ... closes socket ...
    // ... re-listens immediately ...
    
    // ❌ MISSING: Cancel any pending DMA transfers on this socket!
}
```

And in `wiznet5k_socket_send()`:

```c
static mp_uint_t wiznet5k_socket_send(...) {
    // ...
    w5500_queue_transfer(transfer);
    
    // Wait for transfer to complete
    int result = w5500_wait_transfer_complete(transfer, timeout_ms);
    
    // ✅ Transfer is complete HERE
    // ✅ But w5500_tx_service() might still be in the queue!
}
```

---

## The Fix: Add Transfer Cancellation on Socket Close

### **SOLUTION 1: Cancel Active Transfers When Closing Socket** (Primary Fix)

In `network_wiznet5k.c`, modify `wiznet5k_socket_close()`:

```c
static void wiznet5k_socket_close(mod_network_socket_obj_t *socket) {
    uint8_t sn = (uint8_t)socket->fileno;
    DEBUG_PRINT("socket_close: socket %d\n", sn);

    if (sn < _WIZCHIP_SOCK_NUM_) {
        // ✅ NEW: Cancel any active transfers on this socket FIRST
        w5500_tx_transfer_t* transfer = active_transfers;
        w5500_tx_transfer_t* prev = NULL;
        
        while (transfer) {
            w5500_tx_transfer_t* next = transfer->next;
            
            if (transfer->socket_num == sn) {
                DEBUG_PRINT("socket_close: cancelling active transfer on socket %d\n", sn);
                
                // Remove from active list
                if (prev) {
                    prev->next = next;
                } else {
                    active_transfers = next;
                }
                
                // Mark as error and release
                transfer->state = TX_ERROR;
                w5500_release_transfer(transfer);
            } else {
                prev = transfer;
            }
            
            transfer = next;
        }
        
        // Now proceed with existing close logic
        DEBUG_PRINT("socket_close: calling WIZCHIP_EXPORT(close) on socket %d\n", sn);
        // ... rest of existing code ...
    }
}
```

---

### **SOLUTION 2: Add Socket Number Validation in w5500_tx_service()** (Defense in Depth)

In `network_wiznet5k.c`, add validation in `w5500_tx_service()`:

```c
static void w5500_tx_service(void) {
    w5500_tx_transfer_t* transfer = active_transfers;

    while (transfer) {
        w5500_tx_transfer_t* next = transfer->next;
        
        // ✅ NEW: Validate socket is still in a valid state for transfers
        uint8_t sr = getSn_SR(transfer->socket_num);
        if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT) {
            DEBUG_PRINT("w5500_tx_service: socket %d no longer valid (state=0x%02x), aborting transfer\n",
                       transfer->socket_num, sr);
            transfer->state = TX_ERROR;
            w5500_dequeue_transfer(transfer);
            w5500_release_transfer(transfer);
            transfer = next;
            continue;
        }

        switch (transfer->state) {
            // ... existing state machine ...
        }
        
        transfer = next;
    }
}
```

---

### **SOLUTION 3: Prevent Re-Listening Until Transfers Complete** (Alternative)

Modify `socket_close()` to **wait** for active transfers before re-listening:

```c
static void wiznet5k_socket_close(mod_network_socket_obj_t *socket) {
    uint8_t sn = (uint8_t)socket->fileno;
    
    // Wait for active transfers on this socket to complete (with timeout)
    uint32_t start = mp_hal_ticks_ms();
    bool has_active_transfer = true;
    
    while (has_active_transfer && (mp_hal_ticks_ms() - start) < 1000) {
        has_active_transfer = false;
        
        w5500_tx_transfer_t* transfer = active_transfers;
        while (transfer) {
            if (transfer->socket_num == sn) {
                has_active_transfer = true;
                break;
            }
            transfer = transfer->next;
        }
        
        if (has_active_transfer) {
            w5500_tx_service(); // Process transfers
            mp_hal_delay_ms(10);
        }
    }
    
    if (has_active_transfer) {
        DEBUG_PRINT("socket_close: WARNING - forcing close with active transfer\n");
        // Cancel it forcefully (use Solution 1 code here)
    }
    
    // ... rest of close logic ...
}
```

---

## Why This Explains Your Exact Symptoms

1. **First request works perfectly**: No prior transfers to conflict ✅
2. **Second request hangs**: 
   - Old transfer callback fires during new `socket_accept()`
   - Socket state corruption
   - `socket_accept()` loops forever waiting for ESTABLISHED state that's already corrupted
3. **Hard freeze**: The watchdog isn't fed because accept() is blocking forever
4. **Deterministic**: Always happens on second request because first transfer always completes asynchronously

---

## **Immediate Action Plan**

### Step 1: Add Debug to Confirm
Add this to `socket_accept()` right after `initial state` check:

```c
// Check for rogue transfers
w5500_tx_transfer_t* transfer = active_transfers;
while (transfer) {
    if (transfer->socket_num == sn) {
        DEBUG_PRINT("socket_accept: WARNING - found active transfer on socket %d in state %d!\n",
                   sn, transfer->state);
    }
    transfer = transfer->next;
}
```

### Step 2: Apply Solution 1 (Transfer Cancellation)
This is the cleanest fix - cancel transfers when socket closes.

### Step 3: Test
Your second SVG request should now work without hanging.

---

## Additional Safety: Add Transfer Timeout

In `w5500_tx_service()`, add transfer age tracking:

```c
typedef struct w5500_tx_transfer {
    // ... existing fields ...
    uint32_t start_time_ms;  // ✅ NEW
} w5500_tx_transfer_t;

// In w5500_get_transfer():
transfer->start_time_ms = mp_hal_ticks_ms();

// In w5500_tx_service():
if ((mp_hal_ticks_ms() - transfer->start_time_ms) > 30000) {  // 30 sec timeout
    DEBUG_PRINT("w5500_tx_service: transfer timeout on socket %d\n", transfer->socket_num);
    transfer->state = TX_ERROR;
}
```

---

## Why Size Doesn't Matter

You correctly observed that 52KB > 16KB works fine. The issue isn't size-dependent - it's **request-count dependent**. The bug only manifests when:

1. Socket is reused (second+ request)
2. Previous transfer's background task is still alive
3. New connection tries to use the same socket number

This is a **classic use-after-free** pattern, but for socket descriptors instead of memory.

---

**TL;DR**: Apply Solution 1 (cancel active transfers in `socket_close()`). This will fix your freeze on the second request. The first request works because there's no prior transfer to conflict with.

