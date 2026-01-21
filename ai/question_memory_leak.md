# Description

Take the main_test04_svg_50kb.py, it is the micropython starting application, 
study it thoroughly.
Take the `micropython/extmod/network_wiznet5k.c`, study it from at least 3 different angles. 

Please be  aware, the w5500 is a heavily modified driver, it does not use
lwIP, rather hardware tcp/ip. And w5500 uses a single 16kB RX 16kB TX on
socket  0, it is a single connection client-server architecture (the mcu,
ie. the  w550 is the server, and the client is a webbrowser).

Do not take any files from the internet. Everything is here included.

# The problem

After requesting the /svg path 3 times, the third request result already
only 0kb data instead of 52kB and freezing the mcu.
So there is somewhere a memory leak. I suspect in C land, since 
we use extensively gc.collect() in python land.

# The job

Your job is to find why it is the case.

# Important log details

This line of the console log:
[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00

This is the line which indicates when the MCU is actively waiting for 
incoming connection, and checking for the SR register value. It prints
each 100th of it, to prevent spaming.

The other important information:
When the freezing happens, the console do not indicate the exact freezing 
point. The console printing can not keep up with the actual code execution, 
we can only certain the program code arrived at least at that line of cod,e 
but maybe it even executed some more lines, and froze, but didn't have a 
chance to print anything to the console.


# Micropython log (memory leak)


✗ mpremote
Connected to MicroPython at /dev/ttyACM0
Use Ctrl-] or Ctrl-x to exit this shell
[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: success
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=77 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 60794)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=77
[W5500] socket_recv: data available (77 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=10030a15, sn=0, buf=20033280, len=8192

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before WIZCHIP_EXPORT(recv) - Second Request
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        8
   Active Count:     0
========================================================

[W5500] before MP_THREAD_GIL_ENTER
[W5500] after MP_THREAD_GIL_ENTER
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 77
[W5500] socket_recv: success, received 77 bytes
[MAIN] recv() returned 77 bytes
[MAIN] Request decoded, length=77
[MAIN] Request split into 6 lines
[MAIN] First line: GET /svg HTTP/1.1...
[MAIN] Split into 3 parts
[MAIN] Parsed: method=GET, path=/svg
[MAIN] Request path: /svg
[MAIN] Calling generate_svg_response()
[MAIN] SVG generated, size=52314
Full response size: 52467 bytes (headers: 153, body: 52314)
Response generation time: 0ms
[MAIN] cl.send() is about to be executed , sendwithdma:  0
[W5500] socket_send_without_dma: socket=0, len=52467
[W5500] socket_send_without_dma: chunk sent 16384 bytes, total 16384/52467
[W5500] socket_send_without_dma: SENDOK detected, ready for next chunk
[W5500] socket_send_without_dma: chunk sent 16384 bytes, total 32768/52467
[W5500] socket_send_without_dma: SENDOK detected, ready for next chunk
[W5500] socket_send_without_dma: chunk sent 16384 bytes, total 49152/52467
[W5500] socket_send_without_dma: SENDOK detected, ready for next chunk
[W5500] socket_send_without_dma: chunk sent 3315 bytes, total 52467/52467
[W5500] socket_send_without_dma: success, sent 52467 bytes
[MAIN] Calling cl.close()
[W5500] socket_close: socket 0
[W5500] socket_close: calling WIZCHIP_EXPORT(close) on socket 0
[W5500] socket_close: before - state=0x17 (ESTABLISHED)
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
[W5500] socket_accept: success
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=77 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 32914)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=77
[W5500] socket_recv: data available (77 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=10030a15, sn=0, buf=200332d0, len=8192

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before WIZCHIP_EXPORT(recv) - Second Request
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        8
   Active Count:     0
========================================================

[W5500] before MP_THREAD_GIL_ENTER
[W5500] after MP_THREAD_GIL_ENTER
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 77
[W5500] socket_recv: success, received 77 bytes
[MAIN] recv() returned 77 bytes
[MAIN] Request decoded, length=77
[MAIN] Request split into 6 lines
[MAIN] First line: GET /svg HTTP/1.1...
[MAIN] Split into 3 parts
[MAIN] Parsed: method=GET, path=/svg
[MAIN] Request path: /svg
[MAIN] Calling generate_svg_response()
[MAIN] SVG generated, size=52314
Full response size: 52467 bytes (headers: 153, body: 52314)
Response generation time: 0ms
[MAIN] cl.send() is about to be executed , sendwithdma:  1
[W5500] socket_send (DMA): socket=0, len=52467

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        8
   Active Count:     0
========================================================

[W5500] w5500_dma_init: claimed TX channel 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 0
   READ_ADDR:        0x2003331D
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00300011
   AL1_CTRL:         0x00300011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         0
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        8
   Active Count:     0
========================================================

[W5500] socket_send: using DMA transfer for 52467 bytes
[W5500] socket_send: queued transfer, initial chunk size=16384
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=29637, offset=13253, space_to_end=3131

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: 0
   READ_ADDR:        0x2003331D
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00300011
   AL1_CTRL:         0x00300011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         0
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=0/52467
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 3131 bytes to socket 0 offset 13253

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=0/52467
   Active Count:     1
========================================================


========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=0/52467
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 0
   READ_ADDR:        0x2006702B
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00300011
   AL1_CTRL:         0x00300011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         0
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=0/52467
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 13253 bytes to socket 0 offset 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=0/52467
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 16384/52467 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=46021, offset=13253, space_to_end=3131

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=16384/52467
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 0
   READ_ADDR:        0x2006A3F0
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00300011
   AL1_CTRL:         0x00300011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         0
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=16384/52467
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 3131 bytes to socket 0 offset 13253

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=16384/52467
   Active Count:     1
========================================================


========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=16384/52467
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 0
   READ_ADDR:        0x2006B02B
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00300011
   AL1_CTRL:         0x00300011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         0
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=16384/52467
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 13253 bytes to socket 0 offset 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=16384/52467
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 32768/52467 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=62405, offset=13253, space_to_end=3131

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=32768/52467
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 0
   READ_ADDR:        0x2006E3F0
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00300011
   AL1_CTRL:         0x00300011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         0
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=32768/52467
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 3131 bytes to socket 0 offset 13253

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=32768/52467
   Active Count:     1
========================================================


========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=32768/52467
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 0
   READ_ADDR:        0x2006F02B
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00300011
   AL1_CTRL:         0x00300011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         0
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=32768/52467
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 13253 bytes to socket 0 offset 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=32768/52467
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 49152/52467 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=3315, tx_wr_ptr=13253, offset=13253, space_to_end=3131

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=49152/52467
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 0
   READ_ADDR:        0x200723F0
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00300011
   AL1_CTRL:         0x00300011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         0
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=49152/52467
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 3131 bytes to socket 0 offset 13253

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=49152/52467
   Active Count:     1
========================================================


========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=49152/52467
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 0
   READ_ADDR:        0x2007302B
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00300011
   AL1_CTRL:         0x00300011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         0
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=49152/52467
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 184 bytes to socket 0 offset 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=49152/52467
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 3315 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 52467/52467 bytes sent on socket 0
[W5500] w5500_tx_service: transfer complete, dequeuing socket 0
[W5500] socket_send: transfer complete, sent 52467 bytes
[MAIN] Calling cl.close()
[W5500] socket_close: socket 0
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
[W5500] socket_accept: success
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=77 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 33198)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=77
[W5500] socket_recv: data available (77 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=10030a15, sn=0, buf=2003ff80, len=8192

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before WIZCHIP_EXPORT(recv) - Second Request
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        8
   Active Count:     0
========================================================

[W5500] before MP_THREAD_GIL_ENTER
[W5500] after MP_THREAD_GIL_ENTER
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 77
[W5500] socket_recv: success, received 77 bytes
[MAIN] recv() returned 77 bytes
[MAIN] Request decoded, length=77
[MAIN] Request split into 6 lines
[MAIN] First line: GET /svg HTTP/1.1...
[MAIN] Split into 3 parts
[MAIN] Parsed: method=GET, path=/svg
[MAIN] Request path: /svg
[MAIN] Calling generate_svg_response()
[MAIN] SVG generated, size=52314
Error: memory allocation failed, allocating 52468 bytes
[W5500] socket_close: socket 0
[W5500] socket_close: calling WIZCHIP_EXPORT(close) on socket 0
[W5500] socket_close: before - state=0x17 (ESTABLISHED)
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
[MAIN] ====== WAITING FOR CONNECTION ======
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial SR=0x14 (LISTEN)
[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00

---

FILELIST:
main_test04_svg_50kb.py 
micropython/extmod/network_wiznet5k.c
micropython/extmod/machine_spi.c
micropython/lib/wiznet5k/Ethernet/wizchip_conf.h
micropython/lib/wiznet5k/Ethernet/W5500/w5500.h

