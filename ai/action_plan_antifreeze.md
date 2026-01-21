# Action Plan: Fixing W5500 Camera Streaming Freeze

## Problem Summary

The camera streaming freezes after sending approximately **16.44kB or 22.58kB** of data. This correlates directly with the W5500's 16KB TX buffer size, indicating a **TCP flow control deadlock**.

## Root Cause Analysis

| Factor | Description |
|--------|-------------|
| TX Buffer Size | 16,384 bytes (16KB) |
| Freeze Point | ~16.44KB (16KB + HTTP headers) |
| Primary Cause | TCP window exhaustion - W5500 waiting for ACKs |
| Trigger | Infinite spin in `tight_loop_contents()` without timeout or RX polling |

---

## Phase 1: Add Timeout Protection (Critical)

### Step 1.1: Define Timeout Constants

**File:** `modules/camera/cam.c`

**Location:** After line 47 (after includes), add:

```c
// Timeout constants for flow control
#define TX_WAIT_TIMEOUT_MS      5000    // Max time to wait for TX buffer space
#define SENDOK_TIMEOUT_MS       3000    // Max time to wait for SENDOK
#define TX_POLL_INTERVAL_US     100     // Microseconds between TX_FSR checks
```

### Step 1.2: Add Timeout Helper Function

**File:** `modules/camera/cam.c`

**Location:** Before `streaming_loop()` function (around line 315), add:

```c
// Helper to get current time in milliseconds
static inline uint32_t get_time_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

// Check if timeout has elapsed
static inline bool timeout_elapsed(uint32_t start_ms, uint32_t timeout_ms) {
    return (get_time_ms() - start_ms) >= timeout_ms;
}
```

**Required include:** Add at top of file:
```c
#include "pico/time.h"
```

### Step 1.3: Replace TX_FSR Polling Loop with Timeout

**File:** `modules/camera/cam.c`

**Location:** Lines 410-419 (inside the frame sending while loop)

**Replace:**
```c
// Wait for enough TX buffer space
uint16_t free_space;
while ((free_space = getSn_TX_FSR(stream_socket_num)) < send_len) {
    // Check if socket is still connected while waiting
    sr = getSn_SR(stream_socket_num);
    if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT) {
        mp_printf(MP_PYTHON_PRINTER, "Socket disconnected while waiting for TX space\n");
        return;
    }
    tight_loop_contents();
}
```

**With:**
```c
// Wait for enough TX buffer space WITH TIMEOUT
uint16_t free_space;
uint32_t tx_wait_start = get_time_ms();
while ((free_space = getSn_TX_FSR(stream_socket_num)) < send_len) {
    // Check if socket is still connected
    sr = getSn_SR(stream_socket_num);
    if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT) {
        mp_printf(MP_PYTHON_PRINTER, "Socket disconnected while waiting for TX space\n");
        return;
    }
    
    // Check for timeout
    if (timeout_elapsed(tx_wait_start, TX_WAIT_TIMEOUT_MS)) {
        mp_printf(MP_PYTHON_PRINTER, "TIMEOUT waiting for TX buffer space! FSR=%d, need=%d\n", 
                  free_space, send_len);
        mp_printf(MP_PYTHON_PRINTER, "Socket state: 0x%02x, TX_WR=%d, TX_RD=%d\n",
                  sr, getSn_TX_WR(stream_socket_num), getSn_TX_RD(stream_socket_num));
        return;
    }
    
    // Small delay to reduce CPU spinning
    sleep_us(TX_POLL_INTERVAL_US);
}
```

### Step 1.4: Replace SENDOK Polling Loop with Timeout

**File:** `modules/camera/cam.c`

**Location:** Lines 455-470 (SENDOK wait loop after chunk send)

**Replace:**
```c
// Wait for SENDOK interrupt
while (!(getSn_IR(stream_socket_num) & Sn_IR_SENDOK)) {
    // Check for timeout or disconnect
    if (getSn_IR(stream_socket_num) & Sn_IR_TIMEOUT) {
        mp_printf(MP_PYTHON_PRINTER, "Send timeout!\n");
        setSn_IR(stream_socket_num, Sn_IR_TIMEOUT);
        return;
    }
    
    sr = getSn_SR(stream_socket_num);
    if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT) {
        mp_printf(MP_PYTHON_PRINTER, "Socket disconnected during frame send\n");
        return;
    }
    tight_loop_contents();
}
```

**With:**
```c
// Wait for SENDOK interrupt WITH TIMEOUT
uint32_t sendok_start = get_time_ms();
while (!(getSn_IR(stream_socket_num) & Sn_IR_SENDOK)) {
    // Check for W5500 timeout flag
    if (getSn_IR(stream_socket_num) & Sn_IR_TIMEOUT) {
        mp_printf(MP_PYTHON_PRINTER, "W5500 TIMEOUT flag set!\n");
        setSn_IR(stream_socket_num, Sn_IR_TIMEOUT);
        return;
    }
    
    // Check socket state
    sr = getSn_SR(stream_socket_num);
    if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT) {
        mp_printf(MP_PYTHON_PRINTER, "Socket disconnected during frame send\n");
        return;
    }
    
    // Check for our timeout
    if (timeout_elapsed(sendok_start, SENDOK_TIMEOUT_MS)) {
        mp_printf(MP_PYTHON_PRINTER, "TIMEOUT waiting for SENDOK! IR=0x%02x\n",
                  getSn_IR(stream_socket_num));
        return;
    }
    
    sleep_us(TX_POLL_INTERVAL_US);
}
```

### Step 1.5: Apply Same Pattern to Boundary Send

**File:** `modules/camera/cam.c`

**Location:** Lines 514-528 (SENDOK wait after boundary send)

Apply the same timeout pattern as Step 1.4.

---

## Phase 2: Reduce Chunk Size for Better Flow Control

### Step 2.1: Reduce Chunk Size

**File:** `modules/camera/cam.c`

**Location:** Line 402

**Replace:**
```c
uint16_t chunk_size = 8192;  // 8KB chunks for safety
```

**With:**
```c
// Use smaller chunks to allow TCP ACKs to arrive between sends
// 2KB allows ~8 chunks per TX buffer, giving more opportunities for ACK processing
uint16_t chunk_size = 2048;  // 2KB chunks for better flow control
```

**Rationale:** Smaller chunks mean:
- More frequent SENDOK waits
- More opportunities for the W5500 to process incoming ACKs
- Less data in-flight at any time
- Reduced risk of TCP window exhaustion

---

## Phase 3: Add RX Polling During TX Waits (Advanced)

### Step 3.1: Check for Pending RX Data

**File:** `modules/camera/cam.c`

**Location:** Inside the TX_FSR wait loop (from Step 1.3), add after the socket state check:

```c
// Check if there's pending RX data that needs processing
// This can help if the W5500 needs to handle incoming ACKs
uint16_t rx_pending = getSn_RX_RSR(stream_socket_num);
if (rx_pending > 0) {
    // Log for debugging - in a production system you might want to 
    // actually read and discard this data
    mp_printf(MP_PYTHON_PRINTER, "DEBUG: %d bytes pending in RX buffer\n", rx_pending);
}
```

### Step 3.2: Add Diagnostic Logging

**File:** `modules/camera/cam.c`

**Location:** At the start of the frame sending loop (after line 404), add:

```c
// Diagnostic: Log buffer state at start of each frame
if (frame_count % 10 == 1) {  // Every 10 frames
    mp_printf(MP_PYTHON_PRINTER, "Frame %d: TX_FSR=%d, TX_WR=%d, TX_RD=%d, RX_RSR=%d\n",
              frame_count,
              getSn_TX_FSR(stream_socket_num),
              getSn_TX_WR(stream_socket_num),
              getSn_TX_RD(stream_socket_num),
              getSn_RX_RSR(stream_socket_num));
}
```

---

## Phase 4: Add Keep-Alive and Recovery Mechanism

### Step 4.1: Add Frame Skip on TX Stall

**File:** `modules/camera/cam.c`

**Location:** Replace the TX_FSR wait loop with a skip mechanism

**Concept:** If TX buffer isn't ready within a short time, skip this frame and try the next one.

```c
// Quick check for TX buffer space - if not available, skip this frame
uint16_t free_space = getSn_TX_FSR(stream_socket_num);
if (free_space < send_len) {
    uint32_t quick_wait_start = get_time_ms();
    
    // Wait up to 100ms for buffer space
    while (free_space < send_len && !timeout_elapsed(quick_wait_start, 100)) {
        sr = getSn_SR(stream_socket_num);
        if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT) {
            mp_printf(MP_PYTHON_PRINTER, "Socket disconnected\n");
            return;
        }
        sleep_us(500);
        free_space = getSn_TX_FSR(stream_socket_num);
    }
    
    // If still no space, skip remaining frame data and try next frame
    if (free_space < send_len) {
        mp_printf(MP_PYTHON_PRINTER, "Skipping frame %d - TX buffer busy\n", frame_count);
        break;  // Break inner while loop, continue to next frame
    }
}
```

---

## Phase 5: Verify W5500 Register Configuration

### Step 5.1: Add Initialization Verification

**File:** `modules/camera/cam.c`

**Location:** At the start of `streaming_loop()`, after getting TX buffer size (line 332), add:

```c
// Verify W5500 socket configuration
uint8_t socket_mode = getSn_MR(stream_socket_num);
uint16_t rx_buffer_size = getSn_RxMAX(stream_socket_num);
uint16_t mss = getSn_MSSR(stream_socket_num);

mp_printf(MP_PYTHON_PRINTER, "Socket %d config: Mode=0x%02x, TX=%d, RX=%d, MSS=%d\n",
          stream_socket_num, socket_mode, tx_buffer_size, rx_buffer_size, mss);

// Verify we're in TCP mode
if ((socket_mode & 0x0F) != Sn_MR_TCP) {
    mp_printf(MP_PYTHON_PRINTER, "ERROR: Socket not in TCP mode!\n");
    return;
}
```

### Step 5.2: Check Keep-Alive Settings

**File:** `modules/camera/cam.c` or in the Python initialization

Consider setting TCP keep-alive to prevent connection timeouts:

```c
// Enable TCP keep-alive (optional - may help with long streams)
setSn_KPALVTR(stream_socket_num, 5);  // 5 * 5 seconds = 25 second keep-alive
```

---

## Phase 6: Testing Checklist

### Test 1: Basic Timeout Protection
- [ ] Apply Phase 1 changes only
- [ ] Run streaming test
- [ ] Verify timeout messages appear instead of freeze
- [ ] Check if streaming resumes after timeout

### Test 2: Reduced Chunk Size
- [ ] Apply Phase 2 changes
- [ ] Run streaming test
- [ ] Measure if freeze point moves (should be further into stream)
- [ ] Check FPS impact

### Test 3: Full Implementation
- [ ] Apply all phases
- [ ] Run extended streaming test (10+ minutes)
- [ ] Monitor for memory leaks (gc.collect() and mem_free())
- [ ] Test with different browsers (Chrome, Firefox, Edge)

### Test 4: Network Conditions
- [ ] Test with direct Ethernet connection
- [ ] Test through a switch
- [ ] Test with network stress (other traffic)

---

## Debugging Commands

Add these to your Python test script to monitor W5500 state:

```python
# Add to main_webcamera_colorfix_dma_fps_fix.py for debugging

def debug_w5500_state():
    """Print W5500 socket 0 state for debugging"""
    import nic  # Your wiznet interface
    
    # These would need to be exposed via the MicroPython bindings
    # This is pseudocode - actual implementation depends on your bindings
    print(f"Socket 0 State:")
    print(f"  SR (Status): {nic.get_socket_status(0):#04x}")
    print(f"  TX_FSR (Free): {nic.get_tx_free(0)}")
    print(f"  TX_WR (Write Ptr): {nic.get_tx_wr(0)}")
    print(f"  TX_RD (Read Ptr): {nic.get_tx_rd(0)}")
    print(f"  RX_RSR (Received): {nic.get_rx_rsr(0)}")
```

---

## Expected Outcomes

| Change | Expected Result |
|--------|-----------------|
| Timeout protection | System no longer freezes; returns error instead |
| Smaller chunks | Freeze point moves later or disappears |
| RX polling | Better TCP flow control, fewer stalls |
| Frame skipping | Continuous streaming with occasional dropped frames |

---

## Files Modified

1. `modules/camera/cam.c` - Main streaming implementation
2. (Optional) `micropython/extmod/network_wiznet5k.c` - If DMA improvements needed

---

## Rollback Plan

If changes cause issues:
1. Revert `cam.c` to original state
2. Keep diagnostic logging for future debugging
3. Consider using Python-level streaming with `send_frame_over_eth()` callback instead of C-level `streaming_loop()`

---

## Additional Notes

### Why 16.44kB and 22.58kB?

- **16.44kB** = 16,384 (TX buffer) + ~60 bytes HTTP headers + TCP overhead
- **22.58kB** = 16,384 (TX buffer) + ~6KB (TCP retransmit window) + headers

These numbers confirm the diagnosis: the freeze occurs when the TX buffer fills and there's no room for new data.

### Alternative Approach: Use UDP Instead of TCP

If TCP flow control continues to cause issues, consider UDP streaming:
- No ACK waiting
- No flow control delays
- Accepts packet loss
- Better for real-time video

This would require significant changes to both the C code and JavaScript client.
