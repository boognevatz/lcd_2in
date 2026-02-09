# W5500 Streaming Speed Analysis Report

## Current Speed: 4.8 Mbps
## Expected Speed: 30+ Mbps (W5500 realistic throughput)

---

## Executive Summary

The streaming bottleneck is **NOT** in Python vs C. Both achieve the same 4.8 Mbps because the actual limitation is in the **W5500 driver's send path**, which has multiple blocking waits and per-chunk overhead that dominate transfer time.

---

## Data Flow Analysis (Frame 2+)

```
camera.stream_loop_c() 
  -> mp_stream_write_exactly()     [micropython/py/stream.c:46-83]
       -> socket_write()           [micropython/extmod/modsocket.c:529-539]
            -> nic_protocol->send() 
                 -> wiznet5k_socket_send() [network_wiznet5k.c:1701-1778]
```

---

## Identified Bottlenecks

### 1. CRITICAL: DMA Send Path Waits for SENDOK After Each Chunk

**Location:** `network_wiznet5k.c:950-973` (`w5500_wait_transfer_complete`)

```c
while (transfer->state != TX_COMPLETE && transfer->state != TX_ERROR) {
    w5500_tx_service();
    // ...
    mp_hal_delay_ms(1);   // <-- 1ms DELAY PER ITERATION!
}
```

**Impact:** For a 153KB frame sent in 16KB chunks (~10 chunks), if each chunk waits even 1-2ms for SENDOK confirmation, that's 10-20ms overhead per frame = ~50 FPS max theoretical, but...

### 2. CRITICAL: TX State Machine Polling

**Location:** `network_wiznet5k.c:874-913` (`w5500_tx_service` TX_SENDING_PACKET state)

```c
case TX_SENDING_PACKET: {
    uint8_t ir = getSn_IR(transfer->socket_num);  // SPI read
    if (ir & Sn_IR_SENDOK) {
        // ...
    }
    // No delay here, but called from w5500_wait_transfer_complete
    // which has mp_hal_delay_ms(1)
}
```

**Impact:** The state machine is polled with 1ms intervals. With 10 chunks per frame, that's 10ms+ minimum per frame.

### 3. CRITICAL: socket.c send() Waits for TX Buffer Space

**Location:** `micropython/lib/wiznet5k/Ethernet/socket.c:388-400`

```c
while(1) {
    freesize = getSn_TX_FSR(sn);     // SPI reads (4 total per call!)
    tmp = getSn_SR(sn);               // SPI read
    // ...
    if(len <= freesize) break;
    WIZCHIP_YIELD();                  // Calls mpy_wiznet_yield()
}
wiz_send_data(sn, buf, len);
// ...
while(getSn_CR(sn));                  // Wait for command register
```

**Impact:** Each chunk triggers multiple SPI register reads before data transfer even starts.

### 4. SEVERE: getSn_TX_FSR Does 4+ SPI Reads Per Call

**Location:** `micropython/lib/wiznet5k/Ethernet/W5500/w5500.c:188-209`

```c
uint16_t getSn_TX_FSR(uint8_t sn) {
    uint16_t val=0, val1=0;
    do {
        val1 = WIZCHIP_READ(Sn_TX_FSR(sn));         // 2 SPI transactions
        val1 = (val1 << 8) + WIZCHIP_READ(...);     // (high byte, low byte)
        if (val1 != 0) {
            val = WIZCHIP_READ(Sn_TX_FSR(sn));      // 2 more SPI transactions
            val = (val << 8) + WIZCHIP_READ(...);
        }
    } while (val != val1);  // Loop until stable!
    return val;
}
```

**Impact:** 4-8 SPI transactions just to read TX free space, called before EVERY chunk!

### 5. MODERATE: SPI Burst Has Header Overhead

**Location:** `w5500.c:156-185` (`WIZCHIP_WRITE_BUF`)

```c
void WIZCHIP_WRITE_BUF(uint32_t AddrSel, uint8_t* pBuf, uint16_t len) {
    WIZCHIP_CRITICAL_ENTER();    // Disable interrupts
    WIZCHIP.CS._select();
    // ...
    WIZCHIP.IF.SPI._write_burst(spi_data, 3);  // 3-byte header
    WIZCHIP.IF.SPI._write_burst(pBuf, len);    // Data
    WIZCHIP.CS._deselect();
    WIZCHIP_CRITICAL_EXIT();     // Re-enable interrupts
}
```

**Impact:** Each 16KB chunk has CS toggle + 3-byte header overhead + critical section entry/exit.

### 6. MODERATE: recv() Function Has Debug Delays (LEFT IN!)

**Location:** `micropython/lib/wiznet5k/Ethernet/socket.c:420-605`

```c
int32_t WIZCHIP_EXPORT(recv)(uint8_t sn, uint8_t * buf, uint16_t len) {
    mp_hal_delay_ms(50);    // LINE 423!
    // ...
    mp_hal_delay_ms(50);    // LINE 426!
    mp_hal_delay_ms(50);    // LINE 438!
    // ... MANY MORE throughout the function!
}
```

**Impact:** While recv() is not in the hot send path, these delays affect any bidirectional communication.

---

## Quantified Overhead Per Frame (153,600 bytes)

| Operation | Count | Time Each | Total |
|-----------|-------|-----------|-------|
| 16KB chunk sends | ~10 | Variable | - |
| getSn_TX_FSR() calls | ~10 | 4-8 SPI ops | ~0.5ms |
| SENDOK wait polling | ~10 | 1ms min | **10ms** |
| getSn_CR() waits | ~10 | 1-2 SPI ops | ~0.1ms |
| SPI header overhead | ~10 | 3 bytes | negligible |
| **Total Overhead** | | | **~10-15ms** |

At 10-15ms per frame: **66-100 FPS theoretical max**

But actual is ~4.8 Mbps = 153,600 bytes / 4.8 Mbps = ~256ms per frame = **~4 FPS**

This means **SPI transfer time dominates**, not the polling overhead.

---

## SPI Speed Analysis

Current SPI config (from Python):
```python
spi = machine.SPI(0, baudrate=20000000, ...)  # 20 MHz
```

At 20 MHz SPI:
- 153,600 bytes = 1,228,800 bits
- Transfer time = 1,228,800 / 20,000,000 = **61.4ms per frame**
- Theoretical max = 16.3 FPS = **20 Mbps**

But we're getting 4.8 Mbps, meaning **~24% SPI efficiency**.

---

## Root Causes of Low SPI Efficiency

### 1. CRITICAL_SECTION overhead
Every SPI transaction disables/re-enables interrupts.

### 2. Per-chunk overhead
10 chunks means 10x CS toggles, 10x headers, 10x SENDOK waits.

### 3. Register polling
getSn_TX_FSR, getSn_IR, getSn_CR all require separate SPI transactions.

### 4. Non-pipelined design
The code waits for SENDOK before sending next chunk. Could pipeline: send chunk N while waiting for chunk N-1 confirmation.

---

## Recommended Fixes (Priority Order)

### 1. Increase chunk size to full TX buffer (16KB)
Current: 16KB chunks, but could use single 16KB transfer if buffer is free.
**Benefit:** Reduce per-chunk overhead by 10x.

### 2. Remove 1ms delay in transfer wait loop
```c
// network_wiznet5k.c:963
mp_hal_delay_ms(1);  // REMOVE or reduce to mp_hal_delay_us(100)
```
**Benefit:** Reduce SENDOK wait from 1ms to ~100us per chunk.

### 3. Pipeline the transfers
Start DMA for chunk N+1 while waiting for SENDOK of chunk N.
**Benefit:** Hide SENDOK latency behind next transfer.

### 4. Use non-blocking send with polling
Instead of waiting for SENDOK after each chunk, send all data first, then wait once.
**Benefit:** Eliminate per-chunk waits.

### 5. Increase SPI speed
Try 40MHz or 50MHz SPI if hardware supports it.
```python
spi = machine.SPI(0, baudrate=40000000, ...)
```
**Benefit:** Double theoretical throughput.

---

## Files to Modify

| File | Lines | Change |
|------|-------|--------|
| `network_wiznet5k.c` | 963 | Remove/reduce `mp_hal_delay_ms(1)` |
| `network_wiznet5k.c` | 1746-1748 | Increase initial chunk_size |
| `modcamera.c` | 186 | Increase chunk_size from 16384 to 65536 |
| `socket.c` | 423-467 | Remove debug `mp_hal_delay_ms(50)` calls |
| `main_*.py` | 491 | Increase SPI baudrate |

---

## Quick Test Recommendations

1. **Remove the 1ms delay** in `w5500_wait_transfer_complete()` and measure again.
2. **Remove the 50ms delays** in socket.c recv() function.
3. **Increase chunk size** to 65536 (or full frame) in modcamera.c.
4. **Try 40MHz SPI** in the Python init.

Expected improvement: 4.8 Mbps -> 15-20 Mbps with these changes.
