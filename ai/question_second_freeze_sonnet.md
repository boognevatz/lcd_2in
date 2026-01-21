# W5500 Second Request Freeze - Investigation Summary

## Problem Statement

A MicroPython application using a W5500 Ethernet controller freezes on the **second HTTP request**. The first request always works perfectly, but any subsequent request causes the system to freeze.

### System Architecture
- **Hardware**: W5500 Ethernet controller
- **Software**: MicroPython with custom W5500 driver
- **Network Stack**: W5500 hardware TCP/IP stack (NOT lwIP)
- **Configuration**: Single socket (socket 0) with 16kB RX + 16kB TX buffers
- **Architecture**: Client-server model (MCU is server, browser is client)
- **DMA**: Used for TX operations with circular buffer

### Symptoms

1. **First request**: Works perfectly
   - Socket transitions: `LISTEN → ESTABLISHED → CLOSE_WAIT → CLOSED → LISTEN`
   - Data sent/received successfully
   - Socket closes and reopens cleanly

2. **Second request**: Always freezes
   - Socket transitions: `LISTEN → ESTABLISHED → FREEZE`
   - Accept succeeds, returns valid connection
   - Freeze occurs when calling `recv()`

3. **W5500 chip remains responsive**
   - Responds to ICMP ping requests
   - Hardware is NOT frozen
   - HTTP requests from browser get `NS_ERROR_CONNECTION_REFUSED`

4. **State is identical for both requests**
   - After accept: `SR=0x17 (ESTABLISHED), RX_RSR=331, IR=0x04`
   - Socket number: 0
   - Data is available in RX buffer

### Key Observations

- **Timing independent**: Waiting 60+ seconds between requests doesn't help
- **lwIP path doesn't freeze**: Using software TCP stack (lwIP) works, just slower
- **First request always works**: No matter the payload size
- **Freeze location**: After `accept()` returns to Python, when `recv()` is called
- **Console buffering**: Print statements may lag behind actual execution
- **Smartness in python**: opening/closing in python land, to prevent any timing/thread issues.

## Investigation Timeline

### Theory 1: DMA/SPI Not Cleaned Up (DISPROVED)
**Hypothesis**: DMA transfer from first request still active when second request starts, leaving SPI in DMA mode.

**Evidence Against**:
- Waiting 60+ seconds between requests still causes freeze
- DMA transfers complete in milliseconds

### Theory 2: Socket State Not Properly Cleaned Up (DISPROVED)
**Hypothesis**: W5500 hardware state machine stuck or has residual data.

**Evidence Against**:
- Extensive state verification already in code
- Multiple delays and polling loops wait for state transitions
- State after accept is identical for both requests
- Hardware responds to ping (not stuck)

### Theory 3: GIL (Global Interpreter Lock) Deadlock (PARTIAL)
**Hypothesis**: GIL becomes deadlocked preventing recv() from being called.

**Evidence For**:
- Freeze occurs at Python→C transition
- First request: GIL exit/enter works normally
- Second request: Never reaches GIL exit/enter

**Evidence Against**:
- `accept()` successfully acquires/releases GIL on second request
- Python can print after accept (GIL working)
- Only recv() fails, not other operations

**Status**: Unclear why GIL would work for accept but not recv.


## Detailed Logs


lcd_2in main  ? ❯ mpremote
Connected to MicroPython at /dev/ttyACM0
Use Ctrl-] or Ctrl-x to exit this shell
Waiting for Ethernet link...
Connected. IP address: 172.16.1.1
Server will listen on 172.16.1.1:80
[W5500] socket_socket: domain=2, type=1, proto=0
[W5500] socket_socket: TCP socket
[W5500] socket_socket: assigned socket number 0
[W5500] socket_socket: success, fileno=0
[W5500] socket_bind: socket=0, port=80, type=0x01
[W5500] socket_bind: WIZCHIP_EXPORT(socket) returned 0
[W5500] socket_bind: socket 0 state after open: 0x13 (INIT)
[W5500] socket_bind: success
[W5500] socket_listen: socket=0, backlog=5
[W5500] socket_listen: before - socket 0 state: 0x13 (INIT)
[W5500] socket_listen: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_listen: after - socket 0 state: 0x14 (LISTEN)
[W5500] socket_listen: saved state - port=80, type=0x01
[W5500] socket_listen: success
[MAIN] Server socket created and listening on port 80
[MAIN] ====== WAITING FOR CONNECTION ======
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial SR=0x14 (LISTEN)
[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: success
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=331 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 55182)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=331
[W5500] socket_recv: data available (331 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT
[W5500] before MP_THREAD_GIL_ENTER
[W5500] after MP_THREAD_GIL_ENTER
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 331
[W5500] socket_recv: success, received 331 bytes
[MAIN] recv() returned 331 bytes
[MAIN] Request decoded, length=331
[MAIN] Request split into 11 lines
[MAIN] First line: GET /1 HTTP/1.1...
[MAIN] Split into 3 parts
[MAIN] Parsed: method=GET, path=/1
[MAIN] Request path: /1
[MAIN] Calling generate_small_html_response()
[MAIN] HTML small generated, size=2013
Full response size: 2176 bytes (headers: 163, body: 2013)
Response generation time: 0ms
[MAIN] cl.send() is about to be executed
[W5500] socket_send: socket=0, len=2176
[W5500] socket_send: using DMA transfer for 2176 bytes
[W5500] socket_send: queued transfer, initial chunk size=2176
[W5500] w5500_send_chunk: sn=0, len=2176, tx_wr_ptr=8154, offset=8154, space_to_end=8230
[W5500] w5500_dma_write_to_txbuf: wrote 2176 bytes to socket 0 offset 8154
[W5500] w5500_tx_service: started DMA for 2176 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 2176/2176 bytes sent on socket 0
[W5500] socket_send: transfer complete, sent 2176 bytes
[MAIN] Calling cl.close()
[W5500] socket_close: socket 0
[W5500] socket_close: cancelling active transfer on socket 0
[W5500] socket_close: calling WIZCHIP_EXPORT(close) on socket 0
[W5500] socket_close: before - state=0x1c (CLOSE_WAIT)
[W5500] socket_close: after - state=0x00 (CLOSED)
[W5500] socket_close: done, Python should handle re-listen
[MAIN] close() returned successfully
[MAIN] Closing server socket and creating new one. ..
[W5500] socket_close: socket 0
[W5500] socket_close: calling WIZCHIP_EXPORT(close) on socket 0
[W5500] socket_close: before - state=0x00 (CLOSED)
[W5500] socket_close: after - state=0x00 (CLOSED)
[W5500] socket_close: done, Python should handle re-listen
[W5500] socket_socket: domain=2, type=1, proto=0
[W5500] socket_socket: TCP socket
[W5500] socket_socket: assigned socket number 0
[W5500] socket_socket: success, fileno=0
[W5500] socket_bind: socket=0, port=80, type=0x01
[W5500] socket_bind: WIZCHIP_EXPORT(socket) returned 0
[W5500] socket_bind: socket 0 state after open: 0x13 (INIT)
[W5500] socket_bind: success
[W5500] socket_listen: socket=0, backlog=5
[W5500] socket_listen: before - socket 0 state: 0x13 (INIT)
[W5500] socket_listen: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_listen: after - socket 0 state: 0x14 (LISTEN)
[W5500] socket_listen: saved state - port=80, type=0x01
[W5500] socket_listen: success
[MAIN] Server socket created and listening on port 80
[MAIN] New server socket ready
[MAIN] ====== WAITING FOR CONNECTION ======
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial SR=0x14 (LISTEN)
[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 200 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 300 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 400 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 500 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 600 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 700 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 800 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 900 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 1000 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 1100 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 1200 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 1300 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 1400 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 1500 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 1600 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 1700 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 1800 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 1900 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 2000 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 2100 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 2200 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 2300 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 2400 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 2500 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 2600 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 2700 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 2800 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 2900 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: success
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=331 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 44796)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=331
[W5500] socket_recv: data available (331 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT


## Comparison: lwIP vs Hardware Stack

### lwIP (Working)
- Software TCP/IP stack
- No DMA usage
- Slower but more CPU intensive
- **No freeze issues**

### W5500 Hardware Stack (Freezing)
- Hardware TCP/IP offload
- Uses DMA for efficiency
- Single socket, circular buffer
- **Freezes on second request**


## Environment

- **MicroPython Version**: Unknown (RP2040 port)
- **Hardware**: W5500 Ethernet controller on RP2040
- **Connection**: USB serial console `/dev/ttyACM0`
- **Network**: 172.16.1.1 (MCU) ↔ 172.16.1.2 (client)

## Notes

- Console output may lag behind execution due to buffering
- No memory allocation failures reported

## Conclusion

The root cause remains unidentified. The freeze is **100% reproducible** on the second request, regardless of timing. 

The fact that lwIP works suggests this is a W5500-specific driver issue, not a fundamental MicroPython or network problem.


---

FILELIST:
micropython/extmod/network_wiznet5k.c
micropython/lib/wiznet5k/Ethernet/wizchip_conf.h
micropython/lib/wiznet5k/Ethernet/W5500/w5500.h
main_test04_svg_50kb.py

