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

We only use (from python) without DMA sending, it is where we are focusing now.

If a request is bigger then 16kB (like /svg), it is get chopped at 16kB.
Fix it. Here is a console log of main_test04_svg_50kb.py, requesting /svg.

Please recognize, that this line:
[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00

Means the mcu is waiting for connection, and prints out only each 100th, 
Sn_SR socket state register spi query. (listen is 0x14)

# Micropython log
[MAIN] Server socket created and listening on port 80
[MAIN] New server socket ready
[MAIN] ====== WAITING FOR CONNECTION ======
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial SR=0x14 (LISTEN)
[W5500] socket_accept: success
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=338 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 48260)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=338
[W5500] socket_recv: data available (338 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=10030605, sn=0, buf=200332d0, len=8192
[W5500] before MP_THREAD_GIL_ENTER
[W5500] after MP_THREAD_GIL_ENTER
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 338
[W5500] socket_recv: success, received 338 bytes
[MAIN] recv() returned 338 bytes
[MAIN] Request decoded, length=338
[MAIN] Request split into 11 lines
[MAIN] First line: GET /svg HTTP/1.1...
[MAIN] Split into 3 parts
[MAIN] Parsed: method=GET, path=/svg
[MAIN] Request path: /svg
[MAIN] Calling generate_svg_response()
[MAIN] SVG generated, size=52314
Full response size: 52467 bytes (headers: 153, body: 52314)
Response generation time: 0ms
[MAIN] cl.send() is about to be executed
[W5500] socket_send_without_dma: socket=0, len=52467
[W5500] socket_send_without_dma: success, sent 16384 bytes
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
[W5500] socket_accept: success
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=333 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 38444)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=333
[W5500] socket_recv: data available (333 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=10030605, sn=0, buf=20033430, len=8192
[W5500] before MP_THREAD_GIL_ENTER
[W5500] after MP_THREAD_GIL_ENTER
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 333
[W5500] socket_recv: success, received 333 bytes
[MAIN] recv() returned 333 bytes
[MAIN] Request decoded, length=333
[MAIN] Request split into 11 lines
[MAIN] First line: GET /svg HTTP/1.1...
[MAIN] Split into 3 parts
[MAIN] Parsed: method=GET, path=/svg
[MAIN] Request path: /svg
[MAIN] Calling generate_svg_response()
[MAIN] SVG generated, size=52314
Full response size: 52467 bytes (headers: 153, body: 52314)
Response generation time: 0ms
[MAIN] cl.send() is about to be executed
[W5500] socket_send_without_dma: socket=0, len=52467
[W5500] socket_send_without_dma: success, sent 16384 bytes
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
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=346 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 53038)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=346
[W5500] socket_recv: data available (346 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=10030605, sn=0, buf=20040280, len=8192
[W5500] before MP_THREAD_GIL_ENTER
[W5500] after MP_THREAD_GIL_ENTER
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 346
[W5500] socket_recv: success, received 346 bytes
[MAIN] recv() returned 346 bytes
[MAIN] Request decoded, length=346
[MAIN] Request split into 11 lines
[MAIN] First line: GET /favicon.ico HTTP/1.1...
[MAIN] Split into 3 parts
[MAIN] Parsed: method=GET, path=/favicon.ico
[MAIN] Request path: /favicon.ico
[MAIN] Calling generate_large_html_response()
[MAIN] HTML generated, size=6468
Full response size: 6631 bytes (headers: 163, body: 6468)
Response generation time: 0ms
[MAIN] cl.send() is about to be executed
[W5500] socket_send_without_dma: socket=0, len=6631
[W5500] socket_send_without_dma: success, sent 6631 bytes
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

---

FILELIST:
main_test04_svg_50kb.py 
micropython/extmod/network_wiznet5k.c
micropython/extmod/machine_spi.c
micropython/lib/wiznet5k/Ethernet/wizchip_conf.h
micropython/lib/wiznet5k/Ethernet/W5500/w5500.h

