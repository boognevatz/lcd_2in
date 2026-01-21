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

There are two paths: 
- sending without DMA (working)
- sending with DMA (only first request working) 

# The job

Your job is to find why it is the case. Find anything suspicious, 
we can narrow down the probllem to be something DMA related. 
Find why the DMA path is only working for the first request.

I provide two logs, each of it contains two identical http://172.16.1.1/1 
http request. The non DMA version responds for each of it, the 
DMA one freeze at the second request.

# Important log details

This line of the console log:
[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00

This is the line which indicates when the MCU is actively waiting for 
incoming connection, and checking for the SR register value. It prints
each 100th of it, to prevent spaming.

The other important information:
When the freezing happens, th econsole do not indicate the exact freezing 
point. The console printing can not keep up with the actual code execution, 
we can only certain the program code arrived at least at that line of cod,e 
but maybe it even executed some more lines, and froze, but didn't have a 
chance to print anything to the console.

# Micropython log (non DMA, 1.working, 2. working, 3,etc working)

❯ mpremote
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
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=75 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 37520)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=75
[W5500] socket_recv: data available (75 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=10030715, sn=0, buf=20033180, len=8192
[W5500] before MP_THREAD_GIL_ENTER
[W5500] after MP_THREAD_GIL_ENTER
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 75
[W5500] socket_recv: success, received 75 bytes
[MAIN] recv() returned 75 bytes
[MAIN] Request decoded, length=75
[MAIN] Request split into 6 lines
[MAIN] First line: GET /1 HTTP/1.1...
[MAIN] Split into 3 parts
[MAIN] Parsed: method=GET, path=/1
[MAIN] Request path: /1
[MAIN] Calling generate_small_html_response()
[MAIN] HTML small generated, size=2013
Full response size: 2176 bytes (headers: 163, body: 2013)
Response generation time: 0ms
[MAIN] cl.send() is about to be executed
[W5500] socket_send_without_dma: socket=0, len=2176
[W5500] socket_send_without_dma: chunk sent 2176 bytes, total 2176/2176
[W5500] socket_send_without_dma: success, sent 2176 bytes
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
[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: success
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=75 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 51084)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=75
[W5500] socket_recv: data available (75 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=10030715, sn=0, buf=20035120, len=8192
[W5500] before MP_THREAD_GIL_ENTER
[W5500] after MP_THREAD_GIL_ENTER
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 75
[W5500] socket_recv: success, received 75 bytes
[MAIN] recv() returned 75 bytes
[MAIN] Request decoded, length=75
[MAIN] Request split into 6 lines
[MAIN] First line: GET /1 HTTP/1.1...
[MAIN] Split into 3 parts
[MAIN] Parsed: method=GET, path=/1
[MAIN] Request path: /1
[MAIN] Calling generate_small_html_response()
[MAIN] HTML small generated, size=2013
Full response size: 2176 bytes (headers: 163, body: 2013)
Response generation time: 0ms
[MAIN] cl.send() is about to be executed
[W5500] socket_send_without_dma: socket=0, len=2176
[W5500] socket_send_without_dma: chunk sent 2176 bytes, total 2176/2176
[W5500] socket_send_without_dma: success, sent 2176 bytes
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
[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00


# Micropython log (both DMA, 1. working, 2. non working)

 ❯ mpremote
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
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=75 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 44080)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=75
[W5500] socket_recv: data available (75 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=10030715, sn=0, buf=20033180, len=8192
[W5500] before MP_THREAD_GIL_ENTER
[W5500] after MP_THREAD_GIL_ENTER
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 75
[W5500] socket_recv: success, received 75 bytes
[MAIN] recv() returned 75 bytes
[MAIN] Request decoded, length=75
[MAIN] Request split into 6 lines
[MAIN] First line: GET /1 HTTP/1.1...
[MAIN] Split into 3 parts
[MAIN] Parsed: method=GET, path=/1
[MAIN] Request path: /1
[MAIN] Calling generate_small_html_response()
[MAIN] HTML small generated, size=2013
Full response size: 2176 bytes (headers: 163, body: 2013)
Response generation time: 0ms
[MAIN] cl.send() is about to be executed
[W5500] socket_send (DMA): socket=0, len=2176
[W5500] w5500_dma_init: claimed TX channel 0
[W5500] socket_send: using DMA transfer for 2176 bytes
[W5500] socket_send: queued transfer, initial chunk size=2176
[W5500] w5500_send_chunk: sn=0, len=2176, tx_wr_ptr=8300, offset=8300, space_to_end=8084
[W5500] w5500_dma_write_to_txbuf: wrote 2176 bytes to socket 0 offset 8300
[W5500] w5500_tx_service: started DMA for 2176 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 2176/2176 bytes sent on socket 0
[W5500] w5500_tx_service: transfer complete, dequeuing socket 0
[W5500] socket_send: transfer complete, sent 2176 bytes
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
[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: success
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=75 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 40614)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=75
[W5500] socket_recv: data available (75 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=10030715, sn=0, buf=20035120, len=8192


# First DMA, then non DMA (1. working, 2. freeze)


❯ mpremote
Connected to MicroPython at /dev/ttyACM0
Use Ctrl-] or Ctrl-x to exit this shell
[W5500] wiznet5k_active: n_args=2, IS_ACTIVE=0
[W5500] wiznet5k_active: activating...
[W5500] wiznet5k_active: calling wiznet5k_init()
[W5500] wiznet5k_init: initializing with provided TCP stack
[W5500] wiznet5k_init: calling ctlwizchip(CW_INIT_WIZCHIP, ...)
[W5500] wiznet5k_init: calling ctlnetwork(CN_SET_NETINFO, ...)
[W5500] wiznet5k_init: registering with network module
[W5500] w5500_tx_pool_init: initialized transfer pool
[W5500] wiznet5k_init: done, active=true
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
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=75 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 58720)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=75
[W5500] socket_recv: data available (75 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=1003071d, sn=0, buf=20033280, len=8192
[W5500] before MP_THREAD_GIL_ENTER
[W5500] after MP_THREAD_GIL_ENTER
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 75
[W5500] socket_recv: success, received 75 bytes
[MAIN] recv() returned 75 bytes
[MAIN] Request decoded, length=75
[MAIN] Request split into 6 lines
[MAIN] First line: GET /1 HTTP/1.1...
[MAIN] Split into 3 parts
[MAIN] Parsed: method=GET, path=/1
[MAIN] Request path: /1
[MAIN] Calling generate_small_html_response()
[MAIN] HTML small generated, size=2013
Full response size: 2176 bytes (headers: 163, body: 2013)
Response generation time: 0ms
[MAIN] cl.send() is about to be executed , sendwithdma:  0
[W5500] socket_send (DMA): socket=0, len=2176
[W5500] w5500_dma_init: claimed TX channel 0
[W5500] socket_send: using DMA transfer for 2176 bytes
[W5500] socket_send: queued transfer, initial chunk size=2176
[W5500] w5500_send_chunk: sn=0, len=2176, tx_wr_ptr=1061, offset=1061, space_to_end=15323
[W5500] w5500_dma_write_to_txbuf: wrote 2176 bytes to socket 0 offset 1061
[W5500] w5500_tx_service: started DMA for 2176 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 2176/2176 bytes sent on socket 0
[W5500] w5500_tx_service: transfer complete, dequeuing socket 0
[W5500] socket_send: transfer complete, sent 2176 bytes
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
[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 200 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 300 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 400 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 500 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 600 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: success
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=75 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 36834)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=75
[W5500] socket_recv: data available (75 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=1003071d, sn=0, buf=200356b0, len=8192


# First non DMA, second DMA, third DMA (1. working, 2. working, 3. freeze)

❯ mpremote
Connected to MicroPython at /dev/ttyACM0
Use Ctrl-] or Ctrl-x to exit this shell
[W5500] wiznet5k_active: calling wiznet5k_init()
[W5500] wiznet5k_init: initializing with provided TCP stack
[W5500] wiznet5k_init: calling ctlwizchip(CW_INIT_WIZCHIP, ...)
[W5500] wiznet5k_init: calling ctlnetwork(CN_SET_NETINFO, ...)
[W5500] wiznet5k_init: registering with network module
[W5500] w5500_tx_pool_init: initialized transfer pool
[W5500] wiznet5k_init: done, active=true
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
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=75 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 57608)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=75
[W5500] socket_recv: data available (75 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=1003071d, sn=0, buf=20033280, len=8192
[W5500] before MP_THREAD_GIL_ENTER
[W5500] after MP_THREAD_GIL_ENTER
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 75
[W5500] socket_recv: success, received 75 bytes
[MAIN] recv() returned 75 bytes
[MAIN] Request decoded, length=75
[MAIN] Request split into 6 lines
[MAIN] First line: GET /1 HTTP/1.1...
[MAIN] Split into 3 parts
[MAIN] Parsed: method=GET, path=/1
[MAIN] Request path: /1
[MAIN] Calling generate_small_html_response()
[MAIN] HTML small generated, size=2013
Full response size: 2176 bytes (headers: 163, body: 2013)
Response generation time: 0ms
[MAIN] cl.send() is about to be executed , sendwithdma:  0
[W5500] socket_send_without_dma: socket=0, len=2176
[W5500] socket_send_without_dma: chunk sent 2176 bytes, total 2176/2176
[W5500] socket_send_without_dma: success, sent 2176 bytes
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
[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: success
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=75 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 55934)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=75
[W5500] socket_recv: data available (75 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=1003071d, sn=0, buf=200356b0, len=8192
[W5500] before MP_THREAD_GIL_ENTER
[W5500] after MP_THREAD_GIL_ENTER
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 75
[W5500] socket_recv: success, received 75 bytes
[MAIN] recv() returned 75 bytes
[MAIN] Request decoded, length=75
[MAIN] Request split into 6 lines
[MAIN] First line: GET /1 HTTP/1.1...
[MAIN] Split into 3 parts
[MAIN] Parsed: method=GET, path=/1
[MAIN] Request path: /1
[MAIN] Calling generate_small_html_response()
[MAIN] HTML small generated, size=2013
Full response size: 2176 bytes (headers: 163, body: 2013)
Response generation time: 0ms
[MAIN] cl.send() is about to be executed , sendwithdma:  1
[W5500] socket_send (DMA): socket=0, len=2176
[W5500] w5500_dma_init: claimed TX channel 0
[W5500] socket_send: using DMA transfer for 2176 bytes
[W5500] socket_send: queued transfer, initial chunk size=2176
[W5500] w5500_send_chunk: sn=0, len=2176, tx_wr_ptr=641, offset=641, space_to_end=15743
[W5500] w5500_dma_write_to_txbuf: wrote 2176 bytes to socket 0 offset 641
[W5500] w5500_tx_service: started DMA for 2176 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 2176/2176 bytes sent on socket 0
[W5500] w5500_tx_service: transfer complete, dequeuing socket 0
[W5500] socket_send: transfer complete, sent 2176 bytes
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
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=75 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 44006)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=75
[W5500] socket_recv: data available (75 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=1003071d, sn=0, buf=20033280, len=8192


---

FILELIST:
main_test04_svg_50kb.py 
micropython/extmod/network_wiznet5k.c
micropython/extmod/machine_spi.c
micropython/lib/wiznet5k/Ethernet/wizchip_conf.h
micropython/lib/wiznet5k/Ethernet/W5500/w5500.h

