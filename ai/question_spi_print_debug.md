Take the `micropython/extmod/network_wiznet5k.c`, study it from at least 3 different angles. And
create an action plan named `action_plan_spi_sniff.md`, and put a step
by step instruction, what everyone can follow to implement the change.

After each spi communication there is a detailed print of the registers.
However I suspect some of the register printout are not working right.
Your job is to conecntrate on the tx buffer size and usage, during a 
330byte request (get /), which the mcu (tx buffer) response with a 6629byte length.

Please be  aware, the w5500 is a heavily modified driver, it does not use
lwIP, rather hardware tcp/ip. And w5500 uses a single 16kB RX 16kB TX on
socket  0, it is a single connection client-server architecture (the mcu,
ie. the  w550 is the server, and the client is a webbrowser).

# Micropython log

 ❯ mpremote
Connected to MicroPython at /dev/ttyACM0
Use Ctrl-] or Ctrl-x to exit this shell
Waiting for Ethernet link...
Connected. IP address: 172.16.1.1
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

========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: After socket_listen (Listening Started)
--------------------------------------------------------
 [COMMON REGISTERS]
   Mode Register (MR):           0x00
   Gateway Address:              172.16.1.1
   Subnet Mask:                  255.255.255.0
   Source IP:                    172.16.1.1
   MAC Address:                  02:0D:BF:EE:95:EA
   Interrupt Register (IR):      0x00
   Interrupt Mask (IMR):         0x00
   PHY Config (PHYCFGR):         0xBF
   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
   Mode (Sn_MR):                 0x01
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x14 [LISTEN]
   Interrupt (Sn_IR):            0x00
   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):           80 (0x0050)
   Dest Port (Sn_DPORT):             0 (0x0000)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x0000
   TX Read Pointer (RD):         0x0000
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):           0 bytes (0x0000)
   RX Read Pointer (RD):         0x0000
   RX Write Pointer (WR):        0x0000
   RX Buffer Available:              0 bytes
   RX Buffer Capacity:           16384 bytes
========================================================

[W5500] socket_listen: success
Listening on 172.16.1.1:80
[MAIN] ====== WAITING FOR CONNECTION ======
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial state - socket 0: 0x14 (LISTEN)
[W5500] socket_accept: loop 100 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 100)
[W5500] socket_accept: loop 200 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 200)
[W5500] socket_accept: loop 300 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 300)
[W5500] socket_accept: loop 400 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 400)
[W5500] socket_accept: loop 500 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 500)
[W5500] socket_accept: loop 600 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 600)
[W5500] socket_accept: loop 700 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 700)
[W5500] socket_accept: loop 800 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 800)
[W5500] socket_accept: loop 900 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 900)
[W5500] socket_accept: loop 1000 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 1000)
[W5500] socket_accept: loop 1100 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 1100)
[W5500] socket_accept: loop 1200 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 1200)
[W5500] socket_accept: socket ESTABLISHED!
[W5500] socket_accept: RX_RSR=330, TX_FSR=16384 (expected: RX=0, TX=16384)
[W5500] socket_accept: client IP: 172.16.1.2, port: 51998

========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: After socket_accept (Connection Established)
--------------------------------------------------------
 [COMMON REGISTERS]
   Mode Register (MR):           0x00
   Gateway Address:              172.16.1.1
   Subnet Mask:                  255.255.255.0
   Source IP:                    172.16.1.1
   MAC Address:                  02:0D:BF:EE:95:EA
   Interrupt Register (IR):      0x00
   Interrupt Mask (IMR):         0x00
   PHY Config (PHYCFGR):         0xBF
   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
   Mode (Sn_MR):                 0x01
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x17 [ESTABLISHED]
   Interrupt (Sn_IR):            0x04
   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):           80 (0x0050)
   Dest Port (Sn_DPORT):         51998 (0xCB1E)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x7303
   TX Read Pointer (RD):         0x7303
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):         330 bytes (0x014A)
   RX Read Pointer (RD):         0x0000
   RX Write Pointer (WR):        0x014A
   RX Buffer Available:            330 bytes
   RX Buffer Capacity:           16384 bytes
========================================================

[W5500] socket_accept: success (single-client mode)
[MAIN] accept() returned, addr=('172.16.1.2', 51998)
[W5500] socket_recv: socket=0, len=16384, timeout=-1
[W5500] socket_recv: checking, RX_RSR=330
[W5500] socket_recv: data available (330 bytes), calling recv

========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: Before socket_recv (Data Available)
--------------------------------------------------------
 [COMMON REGISTERS]
   Mode Register (MR):           0x00
   Gateway Address:              172.16.1.1
   Subnet Mask:                  255.255.255.0
   Source IP:                    172.16.1.1
   MAC Address:                  02:0D:BF:EE:95:EA
   Interrupt Register (IR):      0x00
   Interrupt Mask (IMR):         0x00
   PHY Config (PHYCFGR):         0xBF
   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
   Mode (Sn_MR):                 0x01
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x17 [ESTABLISHED]
   Interrupt (Sn_IR):            0x04
   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):           80 (0x0050)
   Dest Port (Sn_DPORT):         51998 (0xCB1E)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x7303
   TX Read Pointer (RD):         0x7303
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):         330 bytes (0x014A)
   RX Read Pointer (RD):         0x0000
   RX Write Pointer (WR):        0x014A
   RX Buffer Available:            330 bytes
   RX Buffer Capacity:           16384 bytes
========================================================

[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 330
[RECV DATA] Length: zu bytes | Data: 47 45 54 20 ... [+zu bytes]

========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: After socket_recv (Data Received)
--------------------------------------------------------
 [COMMON REGISTERS]
   Mode Register (MR):           0x00
   Gateway Address:              172.16.1.1
   Subnet Mask:                  255.255.255.0
   Source IP:                    172.16.1.1
   MAC Address:                  02:0D:BF:EE:95:EA
   Interrupt Register (IR):      0x00
   Interrupt Mask (IMR):         0x00
   PHY Config (PHYCFGR):         0xBF
   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
   Mode (Sn_MR):                 0x01
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x17 [ESTABLISHED]
   Interrupt (Sn_IR):            0x04
   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):           80 (0x0050)
   Dest Port (Sn_DPORT):         51998 (0xCB1E)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x7303
   TX Read Pointer (RD):         0x7303
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):           0 bytes (0x0000)
   RX Read Pointer (RD):         0x014A
   RX Write Pointer (WR):        0x014A
   RX Buffer Available:              0 bytes
   RX Buffer Capacity:           16384 bytes
========================================================

[W5500] socket_recv: success, received 330 bytes
[MAIN] recv() returned 330 bytes
[MAIN] Request decoded, length=330
[MAIN] Request split into 11 lines
[MAIN] First line: GET / HTTP/1.1...
[MAIN] Split into 3 parts
[MAIN] Parsed: method=GET, path=/
[MAIN] Request path: /
[MAIN] Calling generate_large_html_response()
[MAIN] HTML generated, size=6466
Full response size: 6629 bytes (headers: 163, body: 6466)
Response generation time: 0ms
[MAIN] cl.send() is about to be executed
[W5500] socket_send: socket=0, len=6629
[SEND DATA] Length: zu bytes | Data: 48 54 54 50 ... [+zu bytes]

========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: Before socket_send
--------------------------------------------------------
 [COMMON REGISTERS]
   Mode Register (MR):           0x00
   Gateway Address:              172.16.1.1
   Subnet Mask:                  255.255.255.0
   Source IP:                    172.16.1.1
   MAC Address:                  02:0D:BF:EE:95:EA
   Interrupt Register (IR):      0x00
   Interrupt Mask (IMR):         0x00
   PHY Config (PHYCFGR):         0xBF
   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
   Mode (Sn_MR):                 0x01
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x17 [ESTABLISHED]
   Interrupt (Sn_IR):            0x04
   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):           80 (0x0050)
   Dest Port (Sn_DPORT):         51998 (0xCB1E)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x7303
   TX Read Pointer (RD):         0x7303
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):           0 bytes (0x0000)
   RX Read Pointer (RD):         0x014A
   RX Write Pointer (WR):        0x014A
   RX Buffer Available:              0 bytes
   RX Buffer Capacity:           16384 bytes
========================================================

[W5500] socket_send: using DMA transfer for 6629 bytes
[W5500] socket_send: queued transfer, initial chunk size=6629
[W5500] w5500_send_chunk: sn=0, len=6629, tx_wr_ptr=29443, offset=13059, space_to_end=3325
[SPI] DMA Write to TX Buffer | Addr: 0x00330310 | Len: zu
[TX] Length: zu bytes | Data: 48 54 54 50 ... [+zu bytes]
[W5500] w5500_dma_write_to_txbuf: wrote 3325 bytes to socket 0 offset 13059
[SPI] DMA Write to TX Buffer | Addr: 0x00000010 | Len: zu
[TX] Length: zu bytes | Data: 69 70 3C 2F ... [+zu bytes]
[W5500] w5500_dma_write_to_txbuf: wrote 3304 bytes to socket 0 offset 0
[W5500] w5500_tx_service: started DMA for 6629 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 6629/6629 bytes sent on socket 0

========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: After socket_send (Transfer Complete)
--------------------------------------------------------
 [COMMON REGISTERS]
   Mode Register (MR):           0x00
   Gateway Address:              172.16.1.1
   Subnet Mask:                  255.255.255.0
   Source IP:                    172.16.1.1
   MAC Address:                  02:0D:BF:EE:95:EA
   Interrupt Register (IR):      0x00
   Interrupt Mask (IMR):         0x00
   PHY Config (PHYCFGR):         0xBF
   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
   Mode (Sn_MR):                 0x01
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x17 [ESTABLISHED]
   Interrupt (Sn_IR):            0x04
   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):           80 (0x0050)
   Dest Port (Sn_DPORT):         51998 (0xCB1E)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x8CE8
   TX Read Pointer (RD):         0x8CE8
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):           0 bytes (0x0000)
   RX Read Pointer (RD):         0x014A
   RX Write Pointer (WR):        0x014A
   RX Buffer Available:              0 bytes
   RX Buffer Capacity:           16384 bytes
========================================================

[W5500] socket_send: transfer complete, sent 6629 bytes
[MAIN] Calling cl.close()
[W5500] socket_close: socket 0

========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: Before socket_close
--------------------------------------------------------
 [COMMON REGISTERS]
   Mode Register (MR):           0x00
   Gateway Address:              172.16.1.1
   Subnet Mask:                  255.255.255.0
   Source IP:                    172.16.1.1
   MAC Address:                  02:0D:BF:EE:95:EA
   Interrupt Register (IR):      0x00
   Interrupt Mask (IMR):         0x00
   PHY Config (PHYCFGR):         0xBF
   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
   Mode (Sn_MR):                 0x01
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x1C [CLOSE_WAIT]
   Interrupt (Sn_IR):            0x06
   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):           80 (0x0050)
   Dest Port (Sn_DPORT):         51998 (0xCB1E)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x8CE8
   TX Read Pointer (RD):         0x8CE8
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):           0 bytes (0x0000)
   RX Read Pointer (RD):         0x014A
   RX Write Pointer (WR):        0x014A
   RX Buffer Available:              0 bytes
   RX Buffer Capacity:           16384 bytes
========================================================

[W5500] socket_close: cancelling active transfer on socket 0
[W5500] socket_close: calling WIZCHIP_EXPORT(close) on socket 0
[W5500] socket_close: before - state=0x1c (CLOSE_WAIT)
[W5500] socket_close: saved state - port=80, type=0x01, was_listening=1
[W5500] socket_close: after - state=0x00 (CLOSED)
[W5500] socket_close: re-listening on port 80
[W5500] socket_close: WIZCHIP_EXPORT(socket) returned 0
[W5500] socket_close: after reopen - RX_RSR=0, TX_FSR=16384/16384
[W5500] socket_close: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_close: successfully re-listened

========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: After socket_close
--------------------------------------------------------
 [COMMON REGISTERS]
   Mode Register (MR):           0x00
   Gateway Address:              172.16.1.1
   Subnet Mask:                  255.255.255.0
   Source IP:                    172.16.1.1
   MAC Address:                  02:0D:BF:EE:95:EA
   Interrupt Register (IR):      0x00
   Interrupt Mask (IMR):         0x00
   PHY Config (PHYCFGR):         0xBF
   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
   Mode (Sn_MR):                 0x01
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x14 [LISTEN]
   Interrupt (Sn_IR):            0x00
   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):           80 (0x0050)
   Dest Port (Sn_DPORT):         51998 (0xCB1E)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x0000
   TX Read Pointer (RD):         0x0000
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):           0 bytes (0x0000)
   RX Read Pointer (RD):         0x0000
   RX Write Pointer (WR):        0x0000
   RX Buffer Available:              0 bytes
   RX Buffer Capacity:           16384 bytes
========================================================

[MAIN] close() returned successfully
[MAIN] ====== WAITING FOR CONNECTION ======
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial state - socket 0: 0x14 (LISTEN)
[W5500] socket_accept: socket ESTABLISHED!
[W5500] socket_accept: RX_RSR=338, TX_FSR=16384 (expected: RX=0, TX=16384)
[W5500] socket_accept: client IP: 172.16.1.2, port: 52012

========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: After socket_accept (Connection Established)
--------------------------------------------------------
 [COMMON REGISTERS]
   Mode Register (MR):           0x00
   Gateway Address:              172.16.1.1
   Subnet Mask:                  255.255.255.0
   Source IP:                    172.16.1.1
   MAC Address:                  02:0D:BF:EE:95:EA
   Interrupt Register (IR):      0x00
   Interrupt Mask (IMR):         0x00
   PHY Config (PHYCFGR):         0xBF
   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
   Mode (Sn_MR):                 0x01
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x17 [ESTABLISHED]
   Interrupt (Sn_IR):            0x04
   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):           80 (0x0050)
   Dest Port (Sn_DPORT):         52012 (0xCB2C)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x5B50
   TX Read Pointer (RD):         0x5B50
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):         338 bytes (0x0152)
   RX Read Pointer (RD):         0x0000
   RX Write Pointer (WR):        0x0152
   RX Buffer Available:            338 bytes
   RX Buffer Capacity:           16384 bytes
========================================================

[W5500] socket_accept: success (single-client mode)
[MAIN] accept() returned, addr=('172.16.1.2', 52012)
[W5500] socket_recv: socket=0, len=16384, timeout=-1
[W5500] socket_recv: checking, RX_RSR=338
[W5500] socket_recv: data available (338 bytes), calling recv

========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: Before socket_recv (Data Available)
--------------------------------------------------------
 [COMMON REGISTERS]
   Mode Register (MR):           0x00
   Gateway Address:              172.16.1.1
   Subnet Mask:                  255.255.255.0
   Source IP:                    172.16.1.1
   MAC Address:                  02:0D:BF:EE:95:EA
   Interrupt Register (IR):      0x00
   Interrupt Mask (IMR):         0x00
   PHY Config (PHYCFGR):         0xBF
   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
   Mode (Sn_MR):                 0x01
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x17 [ESTABLISHED]
   Interrupt (Sn_IR):            0x04
   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):           80 (0x0050)
   Dest Port (Sn_DPORT):         52012 (0xCB2C)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x5B50
   TX Read Pointer (RD):         0x5B50
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):         338 bytes (0x0152)
   RX Read Pointer (RD):         0x

---

FILELIST:
micropython/extmod/network_wiznet5k.c
micropython/extmod/machine_spi.c

