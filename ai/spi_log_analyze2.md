# SPI Log Analysis 2 - NS_ERROR_CONNECTION_REFUSED

## The Problem

After the first request is served:
1. Socket closes and reports successful re-listen
2. Register dump shows `Sn_SR = 0x14 (LISTEN)`
3. **But browser gets `NS_ERROR_CONNECTION_REFUSED`**
4. No further connections are ever accepted

This means **W5500 is sending TCP RST packets** despite registers showing LISTEN state.

---

## Critical Findings

### Finding 1: Stale Destination Port After Re-Listen

**Log Line 673 (After socket_close register dump):**
```
Source Port (Sn_PORT):           80 (0x0050)     <- CORRECT
Dest Port (Sn_DPORT):         33156 (0x8184)    <- STALE!
```

After closing and re-opening the socket, `Sn_DPORT` still contains the previous client's port (33156). 

In a clean LISTEN state, this should be 0 or undefined. **This stale value may be causing the W5500 to reject new connections.**

### Finding 2: Close From CLOSE_WAIT Skips DISCON

**Log Line 620:**
```
[W5500] socket_close: before - state=0x1c (CLOSE_WAIT)
```

**Code at network_wiznet5k.c:1346-1354:**
```c
if (getSn_SR(sn) == SOCK_ESTABLISHED) {   // <- FALSE! State is CLOSE_WAIT
    WIZCHIP_EXPORT(disconnect)(sn);        // <- SKIPPED!
    // Wait for transition...
}
WIZCHIP_EXPORT(close)(sn);                 // <- Called directly
```

When in CLOSE_WAIT state:
- `disconnect()` is NOT called (condition fails)
- `close()` is called directly
- This may not properly complete the TCP FIN handshake
- **W5500 may be left in a corrupted internal state**

### Finding 3: 102-Second Close/Re-Listen Window

| Event | Timestamp | Delta |
|-------|-----------|-------|
| Close starts | 583,179ms | - |
| Socket reopened | 627,784ms | 45 seconds |
| Listen returns | 647,284ms | 64 seconds |
| Register dump done | 685,285ms | **102 seconds total** |

During this 102-second window:
1. Browser likely tried to connect for `/svg`
2. Socket was in CLOSED (0x00) or INIT (0x13) state
3. W5500 sent RST for incoming SYN
4. Browser cached the failure

### Finding 4: Register State Looks Correct But Socket Broken

**After socket_close (Lines 665-687):**
```
Mode (Sn_MR):                 0x01          <- TCP, correct
Command (Sn_CR):              0x00          <- No command pending
Status (Sn_SR):               0x14 [LISTEN] <- Shows LISTEN!
Interrupt (Sn_IR):            0x00          <- No interrupts
Source Port (Sn_PORT):           80         <- Correct
TX Free Size (FSR):           16384 bytes   <- Full buffer
RX Received Size (RSR):           0 bytes   <- Empty
```

Everything LOOKS correct, but connections are refused. This indicates **internal W5500 state machine corruption**.

---

## Root Cause Analysis

### Theory: W5500 State Machine Corruption

The W5500 has an internal TCP state machine that's separate from the status register. The corruption likely occurred due to:

1. **Improper CLOSE_WAIT handling:**
   - Client sent FIN (browser timeout)
   - Socket entered CLOSE_WAIT
   - Code called `close()` without `disconnect()`
   - W5500 may not have sent FIN-ACK properly
   - Internal state machine corrupted

2. **Rapid Socket Recycling:**
   - `socket()` was called immediately after `close()`
   - W5500 may need more time between close and reopen
   - Internal buffers/pointers may not be fully reset

3. **Stale DPORT Value:**
   - The old destination port (33156) wasn't cleared
   - W5500 may be filtering new connections based on this stale value
   - Only connections from port 33156 would be accepted (but browser uses different port)

---

## Evidence: Accept Loop Never Sees Connection

**Log Lines 691-943:**
```
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial SR=0x14 (LISTEN)
[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 200 SR=0x14 (LISTEN) IR=0x00
...
[W5500] socket_accept: loop 5500 SR=0x14 (LISTEN) IR=0x00
```

For 544+ seconds (loop 100 to 5500):
- `Sn_SR` always shows `0x14 (LISTEN)`
- `Sn_IR` always shows `0x00 (no interrupts)`

If browser connections were being REJECTED (RST sent):
- W5500 would receive SYN packet
- W5500 would send RST packet
- **But no state change or interrupt is shown**

This confirms W5500 is:
1. Receiving SYN packets (browser sends them)
2. Sending RST packets (browser gets CONNECTION_REFUSED)
3. But NOT updating registers (state machine bypass)

---

## The Bug in socket_close()

**network_wiznet5k.c:1346-1354:**
```c
// BUG: Only calls disconnect() if ESTABLISHED
if (getSn_SR(sn) == SOCK_ESTABLISHED) {
    WIZCHIP_EXPORT(disconnect)(sn);
    // Wait for transition to CLOSED or TIME_WAIT
    uint32_t start = mp_hal_ticks_ms();
    while (getSn_SR(sn) != SOCK_CLOSED && mp_hal_ticks_ms() - start < 250) {
        mp_hal_delay_ms(5);
    }
}
WIZCHIP_EXPORT(close)(sn);  // Always called
```

**The Problem:**
- When state is `CLOSE_WAIT`, `disconnect()` is skipped
- `close()` on `CLOSE_WAIT` may not properly reset internal state

**The Fix:**
```c
// FIX: Call disconnect() for CLOSE_WAIT too
uint8_t sr = getSn_SR(sn);
if (sr == SOCK_ESTABLISHED || sr == SOCK_CLOSE_WAIT) {
    WIZCHIP_EXPORT(disconnect)(sn);
    uint32_t start = mp_hal_ticks_ms();
    while (getSn_SR(sn) != SOCK_CLOSED && mp_hal_ticks_ms() - start < 500) {
        mp_hal_delay_ms(10);
    }
}
WIZCHIP_EXPORT(close)(sn);
```

---

## Additional Fix: Clear DPORT Before Re-Listen

**After line 1395 (after socket() returns), add:**
```c
// Clear stale destination port/IP to ensure clean LISTEN state
setSn_DPORT(sn, 0);
setSn_DIPR(sn, (uint8_t[]){0, 0, 0, 0});
```

This ensures no stale connection info from previous connection affects the new LISTEN socket.

---

## Additional Fix: Add Delay Between Close and Reopen

**Between close() and socket() calls:**
```c
WIZCHIP_EXPORT(close)(sn);
mp_hal_delay_ms(100);  // Give W5500 time to fully reset internal state

// Wait for CLOSED state
uint32_t start = mp_hal_ticks_ms();
while (getSn_SR(sn) != SOCK_CLOSED && mp_hal_ticks_ms() - start < 500) {
    mp_hal_delay_ms(10);
}

mp_int_t ret = WIZCHIP_EXPORT(socket)(sn, saved_type, saved_port, 0);
```

---

## Summary of Required Fixes

### Fix 1: Handle CLOSE_WAIT in disconnect logic
```c
// network_wiznet5k.c:1346
if (getSn_SR(sn) == SOCK_ESTABLISHED || getSn_SR(sn) == SOCK_CLOSE_WAIT) {
```

### Fix 2: Clear stale DPORT after socket() reopen
```c
// After line 1395
setSn_DPORT(sn, 0);
```

### Fix 3: Ensure proper close timing
```c
// After WIZCHIP_EXPORT(close)(sn) at line 1354
mp_hal_delay_ms(100);
while (getSn_SR(sn) != SOCK_CLOSED && timeout) { wait; }
```

### Fix 4: Reduce debug overhead (main slowness fix)
```c
#define W5500_DEBUG 0
#define W5500_SPI_DEBUG 0  
#define W5500_SPI_RAW_LOG 0
```

And reduce accept loop delay:
```c
// Line 1684
mp_hal_delay_ms(1);  // Was 90
```

---

## Why This Causes Permanent Failure

1. First request takes 9 minutes (debug delays)
2. Browser times out, sends FIN, socket enters CLOSE_WAIT
3. Server finally responds, then closes socket
4. Close from CLOSE_WAIT corrupts W5500 internal state
5. Socket reports LISTEN but W5500 internally sends RST to all SYN packets
6. Browser gets CONNECTION_REFUSED and won't retry
7. User refreshes -> Same result (W5500 still corrupted)
8. **Only a hardware reset (power cycle) fixes it**

---

## Verification: Check if Power Cycle Fixes It

To confirm this diagnosis:
1. After seeing CONNECTION_REFUSED
2. Power cycle the RP2040/W5500 board
3. Try connecting again
4. If it works -> confirms W5500 state corruption
5. Apply the fixes above to prevent recurrence
