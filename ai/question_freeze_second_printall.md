# Actual problem

Take the main_test04_svg_50kb.py, it is the micropython starting application, 
study it thoroughly.
Take the `micropython/extmod/network_wiznet5k.c`, study it from at least 3 different angles. 

The main problem is the second request makes the micropython freeze.
The w5500 chip do not freeze, as it responds to ping (icmp) requests, 
however any real request (http /get) results in NS_ERROR_CONNECTION_REFUSED.

The first request is fine, it returns data, the data is not corrupted.
But when the second connection happens
it can not read the actual data and it freezes.

The two request is identical, also results the same socket state transitions:
(listen, established, close_wait, listen, established (freeze))

The socket close and reopen happens in python land (before was in C land), 
but still the exact same problem.

Please be  aware, the w5500 is a heavily modified driver, it does not use
lwIP, rather hardware tcp/ip. And w5500 uses a single 16kB RX 16kB TX on
socket  0, it is a single connection client-server architecture (the mcu,
ie. the  w550 is the server, and the client is a webbrowser).

Do not take any files from the internet. Everything is here included.

# Your job

Still we need to figure out why the freezing can happen.
In the micropython log, I will indicate where the two request ends

# Micropython log


i❯ mpremote
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
# Start  of waiting for incoming connection 
[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00
# We waited around 100 loop time already, beginning of FIRST request 
[W5500] socket_accept: success
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=331 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 58520)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=331
[W5500] socket_recv: data available (331 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=10030659, sn=0, buf=20033180, len=8192
[RECV_DBG] ENTRY: sn=0, buf=20033180, len=8192
[RECV_DBG] tmp initialized to 0
[RECV_DBG] recvsize initialized to 0
[RECV_DBG] CHECK_SOCKNUM passed
[RECV_DBG] CHECK_SOCKMODE(Sn_MR_TCP) passed
[RECV_DBG] CHECK_SOCKDATA passed
[RECV_DBG] getSn_RxMAX returned recvsize=16384
[RECV_DBG] after len adjustment: len=8192
[RECV_DBG] entering while(1) loop to wait for data
[RECV_DBG] loop: getSn_RX_RSR returned recvsize=331
[RECV_DBG] loop: getSn_SR returned tmp=0x17
[RECV_DBG] loop: recvsize!=0, breaking out of loop
[RECV_DBG] exited while(1) loop
[RECV_DBG] non-W5300 path: recvsize=331, len=8192
[RECV_DBG] after final len adjustment: len=331
[RECV_DBG] calling wiz_recv_data(sn=0, buf=20033180, len=331)
[RECV_DBG] wiz_recv_data returned
[RECV_DBG] calling setSn_CR(sn=0, Sn_CR_RECV=0x40)
[RECV_DBG] setSn_CR done, waiting for CR to clear
[RECV_DBG] CR cleared (getSn_CR returned 0)
[RECV_DBG] returning len=331
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
[W5500] w5500_send_chunk: sn=0, len=2176, tx_wr_ptr=25094, offset=8710, space_to_end=7674
[W5500] w5500_dma_write_to_txbuf: wrote 2176 bytes to socket 0 offset 8710
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
# Start  of waiting for incoming connection 
[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00
# We waited around 100 loop time already, beginning of SECOND request 
[W5500] socket_accept: success
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=331 IR=0x04
[MAIN] accept() returned, addr=('172.16.1.2', 40596)
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=331
[W5500] socket_recv: data available (331 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=10030659, sn=0, buf=20035220, len=8192
[RECV_DBG] ENTRY: sn=0, buf=20035220, len=8192
[RECV_DBG] tmp initialized to 0
[RECV_DBG] recvsize initialized to 0
[RECV_DBG] CHECK_SOCKNUM passed
[RECV_DBG] CHECK_SOCKMODE(Sn_MR_TCP) passed
[RECV_DBG] CHECK_SOCKDATA passed
[RECV_DBG] getSn_RxMAX returned recvsize=16384
[RECV_DBG] after len adjustment: len=8192
[RECV_DBG] entering while(1) loop to wait for data
[RECV_DBG] loop: getSn_RX_RSR r



---

FILELIST:
micropython/extmod/network_wiznet5k.c
micropython/lib/wiznet5k/Ethernet/wizchip_conf.h
micropython/lib/wiznet5k/Ethernet/socket.c

