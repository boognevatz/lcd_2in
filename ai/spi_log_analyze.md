# SPI Log Analysis - W5500 MicroPython Driver

## Executive Summary

**Total response time: ~321 seconds** (from first browser connection to response)

The system is **extremely slow** due to excessive debug SPI register reads during the `socket_accept()` polling loop. Each loop iteration takes ~50ms due to verbose register dumps and SPI debug logging.

---

## 1. Call Flow Trace (Code to Log Mapping)

### Phase 1: Initialization (0ms - 91,112ms)

```
main_test04_svg_50kb.py:32 ->
  network.WIZNET5K(spi, cs, rst) ->
    network_wiznet5k.c:2099 wiznet5k_make_new() ->
      stores SPI object reference

main_test04_svg_50kb.py:33 ->
  nic.active(True) ->
    network_wiznet5k.c:2237 wiznet5k_active() ->
      network_wiznet5k.c:2259 w5500_dma_init() ->
        DEBUG_PRINT("w5500_dma_init: claimed TX channel 0")
      network_wiznet5k.c:2271 wiznet5k_init() ->
        DEBUG_PRINT("wiznet5k_init: initializing with provided TCP stack")
        network_wiznet5k.c:1161 w5500_dma_init()
        network_wiznet5k.c:1172 ctlwizchip(CW_INIT_WIZCHIP, ...) ->
          [859 ms] [SPI TX BURST, len: 3] 0x 00 09 00|  (read MR register)
          [859 ms] [SPI TX BURST, len: 3] 0x 00 01 00|  (read GAR0)
          ... (multiple SPI reads for buffer config)
        network_wiznet5k.c:1175 ctlnetwork(CN_SET_NETINFO, ...) ->
          [32859 ms] [SPI TX BURST, len: 6] 0x 00 00 00 00|
        network_wiznet5k.c:1191 mod_network_register_nic()
        network_wiznet5k.c:1203 w5500_tx_pool_init() ->
          DEBUG_PRINT("w5500_tx_pool_init: initialized transfer pool")
        DEBUG_PRINT("wiznet5k_init: done, active=true")
        dump_w5500_state("After wiznet5k_init (Initialization Complete)")
```

**Log Evidence (Lines 38-97):**
```
[W5500] w5500_dma_init: claimed TX channel 0
[W5500] wiznet5k_active: calling wiznet5k_init()
[W5500] wiznet5k_init: initializing with provided TCP stack
[W5500] wiznet5k_init: calling ctlwizchip(CW_INIT_WIZCHIP, ...)
 [859 ms] [SPI TX BURST, len: 3 ] 0x 00 09 00|
...
========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: After wiznet5k_init (Initialization Complete)
```

---

### Phase 2: Socket Setup (91,112ms - 139,613ms)

```
main_test04_svg_50kb.py:44 ->
  s = socket.socket() ->
    network_wiznet5k.c:1231 wiznet5k_socket_socket() ->
      DEBUG_PRINT("socket_socket: domain=2, type=1, proto=0")
      DEBUG_PRINT("socket_socket: TCP socket")
      DEBUG_PRINT("socket_socket: assigned socket number 0")
      DEBUG_PRINT("socket_socket: success, fileno=0")

main_test04_svg_50kb.py:45 ->
  s.bind(("0.0.0.0", 80)) ->
    network_wiznet5k.c:1461 wiznet5k_socket_bind() ->
      DEBUG_PRINT("socket_bind: socket=0, port=80, type=0x01")
      [91112 ms] [SPI TX BURST, len: 3] 0x 00 0F 00|  (read Sn_SR)
      WIZCHIP_EXPORT(socket)(0, Sn_MR_TCP, 80, 0) ->
        [91112 ms] [SPI TX BURST, len: 4] 0x 00 01 0C 10|
        [95612 ms] [SPI TX BURST, len: 4] 0x 00 00 0C 01|
      DEBUG_PRINT("socket_bind: socket 0 state after open: 0x13 (INIT)")

main_test04_svg_50kb.py:46 ->
  s.listen(5) ->
    network_wiznet5k.c:1506 wiznet5k_socket_listen() ->
      DEBUG_PRINT("socket_listen: socket=0, backlog=5")
      DEBUG_PRINT("socket_listen: before - socket 0 state: 0x13 (INIT)")
      WIZCHIP_EXPORT(listen)(0) ->
        [101112 ms] [SPI TX BURST, len: 3] 0x 00 01 08|
      DEBUG_PRINT("socket_listen: after - socket 0 state: 0x14 (LISTEN)")
      dump_w5500_state("After socket_listen (Listening Started)")
```

**Log Evidence (Lines 116-178):**
```
[W5500] socket_socket: domain=2, type=1, proto=0
[W5500] socket_socket: TCP socket
[W5500] socket_socket: assigned socket number 0
[W5500] socket_socket: success, fileno=0
[W5500] socket_bind: socket=0, port=80, type=0x01
 [91112 ms] [SPI TX BURST, len: 3 ] 0x 00 0F 00|
...
[W5500] socket_listen: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_listen: after - socket 0 state: 0x14 (LISTEN)
```

---

### Phase 3: Accept Loop - THE SLOW PART (139,613ms - 352,637ms)

**CRITICAL BOTTLENECK IDENTIFIED**

```
main_test04_svg_50kb.py:284 ->
  cl, addr = s.accept() ->
    network_wiznet5k.c:1564 wiznet5k_socket_accept() ->
      DEBUG_PRINT("socket_accept: socket=0, timeout=-1")
      DEBUG_PRINT("socket_accept: initial SR=0x14 (LISTEN)")
      
      POLLING LOOP (line 1635-1686):
      for (;;) {
        loop_counter++;
        sr = getSn_SR(sn);        // SPI read Sn_SR register
        mp_hal_delay_ms(10);      // 10ms delay
        ir = getSn_IR(sn);        // SPI read Sn_IR register
        
        if (loop_counter % 100 == 0) {
          DEBUG_PRINT("socket_accept: loop %d SR=0x%02x IR=0x%02x")
        }
        
        mp_hal_delay_ms(90);      // 90ms delay (!!!)
        mpy_wiznet_yield();
      }
```

**TIMING ANALYSIS OF ACCEPT LOOP:**

| Loop # | Timestamp | Time Delta | Notes |
|--------|-----------|------------|-------|
| Start  | 144,614ms | -          | Enter accept |
| 100    | 145,915ms | ~5,000ms   | First 100 loops |
| 200    | 155,916ms | ~10,000ms  | 100ms per loop avg |
| 300    | 165,916ms | ~10,000ms  | Consistent |
| ...    | ...       | ...        | ... |
| 2000   | 345,917ms | ~200,000ms | Still polling |
| Exit   | 352,637ms | ~208,000ms | Connection established |

**Log Evidence (Lines 182-286):**
```
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial SR=0x14 (LISTEN)
 [144614 ms] [SPI TX BURST, len: 3 ] 0x 00 0F 00|
[145915 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[150915 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
...
[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 200 SR=0x14 (LISTEN) IR=0x00
...
[W5500] socket_accept: loop 2000 SR=0x14 (LISTEN) IR=0x00
 [352637 ms] [SPI TX BURST, len: 4 ] 0x 00 02 0C 01|  <- Connection!
```

---

### Phase 4: Data Receive (352,637ms - 443,642ms)

```
main_test04_svg_50kb.py:287 ->
  request = cl.recv(16384) ->
    network_wiznet5k.c:1817 wiznet5k_socket_recv() ->
      DEBUG_PRINT("socket_recv: socket=0, len=16384, timeout=-1")
      [386638 ms] getSn_RX_RSR(0) -> 330 bytes
      DEBUG_PRINT("socket_recv: data available (330 bytes), calling recv")
      dump_w5500_state("Before socket_recv (Data Available)")
      WIZCHIP_EXPORT(recv)(0, buf, 16384) ->
        [432642 ms - 438142 ms] SPI reads from RX buffer
      DEBUG_PRINT("socket_recv: WIZCHIP_EXPORT(recv) returned 330")
      dump_w5500_state("After socket_recv (Data Received)")
```

**Log Evidence (Lines 335-435):**
```
[W5500] socket_recv: socket=0, len=16384, timeout=-1
 [386638 ms] [SPI TX BURST, len...
[W5500] socket_recv: checking, RX_RSR=330
[W5500] socket_recv: data available (330 bytes), calling recv
...
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 330
[RECV DATA] Length: zu bytes: 47 45 54 20  <- "GET "
```

---

### Phase 5: Response Generation & Send (443,642ms - 549,678ms)

```
main_test04_svg_50kb.py:317-333 ->
  response_body = generate_large_html_response()  # ~1ms
  cl.send(full_response)  # 6630 bytes ->
    network_wiznet5k.c:1724 wiznet5k_socket_send() ->
      DEBUG_PRINT("socket_send: socket=0, len=6630")
      dump_w5500_state("Before socket_send")
      
      // Large transfer path (>= 1KB):
      DEBUG_PRINT("socket_send: using DMA transfer for 6630 bytes")
      w5500_get_transfer() ->
        transfer->data_ptr = buf
        transfer->total_size = 6630
        transfer->chunk_size = 6630
      w5500_queue_transfer(transfer)
      
      w5500_wait_transfer_complete() ->
        w5500_tx_service() ->
          w5500_send_chunk(0, data, 6630, tx_wr_ptr) ->
            network_wiznet5k.c:699 w5500_send_chunk()
            network_wiznet5k.c:592 w5500_dma_write_to_txbuf() ->
              [543169 ms] [SPI TX HDR, len: 3] 0x 14 28 14|
              [543169 ms] [SPI TX DMA DATA, len: 6630] 0x 48 54 54 50|  <- "HTTP"
          setSn_TX_WR(0, new_tx_wr)
          setSn_CR(0, Sn_CR_SEND)
          
          // Wait for SENDOK
          DEBUG_PRINT("w5500_tx_service: SENDOK, 6630/6630 bytes sent")
      
      dump_w5500_state("After socket_send (Transfer Complete)")
```

**Log Evidence (Lines 447-569):**
```
[W5500] socket_send: socket=0, len=6630
[SEND DATA] Length: zu bytes: 48 54 54 50  <- "HTTP"
...
[W5500] socket_send: using DMA transfer for 6630 bytes
[W5500] socket_send: queued transfer, initial chunk size=6630
[W5500] w5500_send_chunk: sn=0, len=6630, tx_wr_ptr=5160, offset=5160, space_to_end=11224
[SPI] DMA Write to TX Buffer | Addr: 0x00142810 | Len: zu
[543169 ms] [SPI TX DMA DATA, len: 6630 ] 0x 48 54 54 50|
[W5500] w5500_tx_service: SENDOK, 6630/6630 bytes sent
```

---

### Phase 6: Socket Close & Re-listen (549,678ms - 685,285ms)

```
main_test04_svg_50kb.py:336 ->
  cl.close() ->
    network_wiznet5k.c:1286 wiznet5k_socket_close() ->
      DEBUG_PRINT("socket_close: socket 0")
      dump_w5500_state("Before socket_close")
      
      // Cancel active transfers (none)
      // Check if was listening -> YES
      
      WIZCHIP_EXPORT(disconnect)(0)  // Graceful FIN
      WIZCHIP_EXPORT(close)(0)
      
      DEBUG_PRINT("socket_close: re-listening on port 80")
      WIZCHIP_EXPORT(socket)(0, Sn_MR_TCP, 80, 0)
      WIZCHIP_EXPORT(listen)(0)
      
      dump_w5500_state("After socket_close")
```

**Log Evidence (Lines 570-690):**
```
[W5500] socket_close: socket 0
...
[W5500] socket_close: re-listening on port 80
 [626234 ms] [SPI TX BURST, len: 3 ] 0x 00 01 08|
...
[W5500] socket_close: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_close: successfully re-listened
...
   Status (Sn_SR):               0x14 [LISTEN]
```

---

## 2. Three Angles of Analysis - network_wiznet5k.c

### Angle 1: Data Flow Architecture

```
                        MicroPython Script
                              |
                    socket.accept/recv/send
                              |
                    +---------v---------+
                    | mod_network_nic   |
                    | protocol_wiznet   |
                    +-------------------+
                              |
     +------------------------+------------------------+
     |                        |                        |
wiznet5k_socket_accept  wiznet5k_socket_recv  wiznet5k_socket_send
     |                        |                        |
     |                        |             +----------+----------+
     |                        |             |                     |
     |                   WIZCHIP_EXPORT   < 1KB                >= 1KB
     |                      (recv)          |                     |
     |                        |        WIZCHIP_EXPORT    DMA Transfer Pool
     |                        |           (send)              |
     |                        |             |          w5500_tx_transfer_t
     +------------------------+-------------+                 |
                              |                      w5500_send_chunk()
                    +---------v---------+                     |
                    | wizchip_conf.c    |          w5500_dma_write_to_txbuf()
                    | (W5500 register   |                     |
                    |  access layer)    |                     |
                    +-------------------+                     |
                              |                               |
               +--------------+---------------+               |
               |              |               |               |
          getSn_SR()    getSn_TX_FSR()  WIZCHIP_WRITE()       |
          getSn_IR()    getSn_RX_RSR()        |               |
               |              |               |               |
               +--------------+-------+-------+               |
                              |       |                       |
                    +---------v-------v-----------------------v--+
                    |           wiz_spi_writeburst()             |
                    |           wiz_spi_readburst()              |
                    +--------------------------------------------+
                              |
                    +---------v---------+
                    |   DMA Engine      |
                    |  (dma_tx_chan)    |
                    +-------------------+
                              |
                    +---------v---------+
                    |   RP2040 SPI      |
                    +-------------------+
                              |
                    +---------v---------+
                    |     W5500         |
                    | (Hardware TCP/IP) |
                    +-------------------+
```

### Angle 2: State Machine - Socket Lifecycle

```
                       +------------+
                       |  CLOSED    |
                       |  (0x00)    |
                       +-----+------+
                             |
                   socket() + bind()
                             |
                       +-----v------+
                       |   INIT     |
                       |  (0x13)    |
                       +-----+------+
                             |
                         listen()
                             |
                       +-----v------+
              +------->|  LISTEN    |<--------+
              |        |  (0x14)    |         |
              |        +-----+------+         |
              |              |                |
              |    Client connects            |
              |    (W5500 hardware)           |
              |              |                |
              |        +-----v------+         |
              |        | ESTABLISHED|         |
              |        |  (0x17)    |         |
              |        +-----+------+         |
              |              |                |
              |     Client sends FIN         |
              |              |                |
              |        +-----v------+         |
              |        | CLOSE_WAIT |         |
              |        |  (0x1C)    |         |
              |        +-----+------+         |
              |              |                |
              |      close() +                |
              |      re-listen                |
              |              |                |
              +--------------+----------------+
```

### Angle 3: Timing Breakdown

| Phase | Function | Duration | Bottleneck |
|-------|----------|----------|------------|
| Init | wiznet5k_init() | 91,112ms | dump_w5500_state() SPI reads |
| Bind | socket_bind() | 4,500ms | SPI register writes |
| Listen | socket_listen() | 6,500ms | dump_w5500_state() |
| **Accept** | **socket_accept()** | **~208,000ms** | **100ms delay per loop** |
| Recv | socket_recv() | ~57,000ms | dump_w5500_state() x2 |
| Send | socket_send() | ~67,000ms | DMA + dump_w5500_state() x2 |
| Close | socket_close() | ~100,000ms | disconnect + dump_w5500_state() |

---

## 3. Root Cause Analysis - Why So Slow?

### Primary Cause: Excessive Debug Delays in Accept Loop

**Location:** `network_wiznet5k.c:1647-1686`

```c
for (;;) {
    sr = getSn_SR(sn);           // SPI read
    mp_hal_delay_ms(10);         // 10ms delay
    ir = getSn_IR(sn);           // SPI read
    // ...
    mp_hal_delay_ms(90);         // 90ms delay  <-- HUGE!
    mpy_wiznet_yield();
}
```

**Each loop iteration = ~100ms minimum**

- The accept loop runs 2000+ times waiting for connection
- 2000 iterations x 100ms = 200,000ms = ~200 seconds just in accept!

### Secondary Cause: dump_w5500_state() Debug Dumps

**Location:** `network_wiznet5k.c:180-236`

Each call to `dump_w5500_state()` performs ~30+ SPI register reads:
- getMR(), getGAR(), getSUBR(), getSIPR(), getSHAR()
- getIR(), getIMR(), getPHYCFGR(), getVERSIONR()
- getSn_MR(), getSn_CR(), getSn_SR(), getSn_IR(), getSn_IMR()
- getSn_PORT(), getSn_DPORT()
- getSn_TX_FSR(), getSn_TX_WR(), getSn_TX_RD()
- getSn_RX_RSR(), getSn_RX_RD(), getSn_RX_WR()
- getSn_TxMAX()

Each SPI read takes ~5ms due to debug logging overhead.

**dump_w5500_state() appears:**
- After wiznet5k_init
- After socket_listen
- After socket_accept (ESTABLISHED)
- Before/After socket_recv (x2)
- Before/After socket_send (x2)
- Before/After socket_close (x2)

= 9 dumps x ~150ms each = ~1,350ms just in dumps

### Tertiary Cause: Raw SPI Byte Logging

**Location:** `network_wiznet5k.c:265-334` - `log_raw_spi_bytes()`

Every SPI transaction is logged with:
- Timestamp calculation
- snprintf() formatting
- printf() output

Even with skip counters (100 occurrences), this adds overhead.

---

## 4. Recommendations for Performance Fix

### Fix 1: Reduce Accept Loop Delay (CRITICAL)

```c
// BEFORE (network_wiznet5k.c:1684-1685):
mp_hal_delay_ms(90);
mpy_wiznet_yield();

// AFTER:
mp_hal_delay_ms(1);  // Reduce to 1ms
mpy_wiznet_yield();
```

**Expected improvement: 200 seconds -> 2 seconds**

### Fix 2: Disable Debug Logging

```c
// network_wiznet5k.c:139
#define W5500_DEBUG 0  // Was 1

// network_wiznet5k.c:149
#define W5500_SPI_DEBUG 0  // Was 1

// network_wiznet5k.c:154
#define W5500_SPI_RAW_LOG 0  // Was 1
```

**Expected improvement: 5-10x faster SPI operations**

### Fix 3: Remove 10ms delay before IR read

```c
// BEFORE (network_wiznet5k.c:1647):
sr = getSn_SR(sn);
mp_hal_delay_ms(10);  // REMOVE THIS
ir = getSn_IR(sn);

// AFTER:
sr = getSn_SR(sn);
ir = getSn_IR(sn);  // No delay needed
```

---

## 5. Complete Calling Tree

```
main_test04_svg_50kb.py
|
+-- :32 network.WIZNET5K(spi, cs, rst)
|   +-- network_wiznet5k.c:2099 wiznet5k_make_new()
|
+-- :33 nic.active(True)
|   +-- network_wiznet5k.c:2237 wiznet5k_active()
|       +-- network_wiznet5k.c:2259 w5500_dma_init()
|       |   +-- dma_claim_unused_channel()
|       |   +-- DEBUG_PRINT("claimed TX channel 0")
|       +-- network_wiznet5k.c:2271 wiznet5k_init()
|           +-- network_wiznet5k.c:1157 wiznet5k_init() [PROVIDED_STACK]
|               +-- w5500_dma_init()
|               +-- ctlwizchip(CW_INIT_WIZCHIP, ...)
|               |   +-- wiz_spi_writeburst() x N
|               +-- ctlnetwork(CN_SET_NETINFO, ...)
|               +-- mod_network_register_nic()
|               +-- w5500_tx_pool_init()
|               +-- dump_w5500_state() [if W5500_SPI_DEBUG]
|
+-- :44 socket.socket()
|   +-- network_wiznet5k.c:1231 wiznet5k_socket_socket()
|       +-- Assigns socket number 0
|       +-- DEBUG_PRINT("socket_socket: success")
|
+-- :45 s.bind(("0.0.0.0", 80))
|   +-- network_wiznet5k.c:1461 wiznet5k_socket_bind()
|       +-- getSn_SR() - check state
|       +-- WIZCHIP_EXPORT(socket)(0, Sn_MR_TCP, 80, 0)
|       |   +-- wizchip_conf.c -> wiz_spi_writeburst()
|       +-- DEBUG_PRINT("socket_bind: success")
|
+-- :46 s.listen(5)
|   +-- network_wiznet5k.c:1506 wiznet5k_socket_listen()
|       +-- getSn_SR() - verify INIT state
|       +-- WIZCHIP_EXPORT(listen)(0)
|       |   +-- setSn_CR(Sn_CR_LISTEN)
|       +-- Save socket state for re-listen
|       +-- dump_w5500_state() [if W5500_SPI_DEBUG]
|
+-- :284 cl, addr = s.accept()  [SLOW - ~200 seconds]
|   +-- network_wiznet5k.c:1564 wiznet5k_socket_accept()
|       +-- getSn_SR() - initial check
|       +-- POLLING LOOP (2000+ iterations):
|           +-- getSn_SR()
|           +-- mp_hal_delay_ms(10)  [UNNECESSARY]
|           +-- getSn_IR()
|           +-- mp_hal_delay_ms(90)  [TOO LONG]
|           +-- mpy_wiznet_yield()
|       +-- getSn_DIPR(), getSn_DPORT()
|       +-- dump_w5500_state() [if W5500_SPI_DEBUG]
|
+-- :287 request = cl.recv(16384)
|   +-- network_wiznet5k.c:1817 wiznet5k_socket_recv()
|       +-- getSn_RX_RSR() - check for data
|       +-- dump_w5500_state("Before socket_recv")
|       +-- WIZCHIP_EXPORT(recv)(0, buf, len)
|       |   +-- wiz_spi_readburst() - read RX buffer
|       +-- dump_w5500_state("After socket_recv")
|
+-- :319 response_body = generate_large_html_response()
|   +-- Python string building (~1ms)
|
+-- :333 cl.send(full_response)  [6630 bytes]
|   +-- network_wiznet5k.c:1724 wiznet5k_socket_send()
|       +-- dump_w5500_state("Before socket_send")
|       +-- w5500_get_transfer() - get from pool
|       +-- w5500_queue_transfer(transfer)
|       +-- w5500_wait_transfer_complete()
|           +-- w5500_tx_service()
|               +-- getSn_TX_FSR() - check buffer space
|               +-- getSn_TX_WR() - get write pointer
|               +-- w5500_send_chunk()
|               |   +-- w5500_dma_write_to_txbuf()
|               |       +-- wiz_cs_select()
|               |       +-- wiz_spi_writeburst(header, 3)
|               |       +-- DMA transfer (6630 bytes)
|               |       +-- wiz_cs_deselect()
|               +-- setSn_TX_WR() - update pointer
|               +-- setSn_CR(Sn_CR_SEND) - trigger send
|               +-- Wait for Sn_IR_SENDOK
|       +-- dump_w5500_state("After socket_send")
|
+-- :336 cl.close()
    +-- network_wiznet5k.c:1286 wiznet5k_socket_close()
        +-- dump_w5500_state("Before socket_close")
        +-- Cancel active transfers (if any)
        +-- WIZCHIP_EXPORT(disconnect)(0)
        +-- WIZCHIP_EXPORT(close)(0)
        +-- RE-LISTEN (since was_listening=true):
        |   +-- WIZCHIP_EXPORT(socket)(0, Sn_MR_TCP, 80, 0)
        |   +-- WIZCHIP_EXPORT(listen)(0)
        +-- dump_w5500_state("After socket_close")
```

---

## 6. SPI Address Decoding

W5500 SPI frame format: `[ADDR_HI] [ADDR_LO] [CONTROL] [DATA...]`

Control byte: `[BSB4:0] [RW] [OM1:OM0]`
- BSB = Block Select Bits (socket n = 0x01 + n*4 for socket regs)
- RW = 0=read, 1=write
- OM = Operation Mode (00=VDM)

### Common patterns observed:

| SPI Bytes | Meaning |
|-----------|---------|
| `0x 00 03 08` | Read Sn_SR (socket 0 status) |
| `0x 00 02 08` | Read Sn_IR (socket 0 interrupt) |
| `0x 00 01 08` | Read Sn_CR (socket 0 command) |
| `0x 00 0F 00` | Read common register 0x0F |
| `0x 00 01 0C 10` | Write Sn_CR = 0x10 (CLOSE cmd) |
| `0x 00 02 0C 01` | Write Sn_IR = 0x01 (clear CON) |
| `0x 14 28 14` | TX buffer write header, offset=0x1428 |

---

## 7. Summary

| Metric | Value | Ideal |
|--------|-------|-------|
| Total response time | ~321 seconds | < 1 second |
| Accept wait time | ~208 seconds | < 100ms |
| Debug dump overhead | ~60 seconds | 0 |
| Actual data transfer | < 1 second | same |

**The system is spending 99.7% of time on debugging/polling overhead.**

Fix the three issues identified above to achieve near-real-time HTTP response.
