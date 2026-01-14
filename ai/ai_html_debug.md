# Analyzing HTTP 200 with HTML on W5500 server

Take the `main_w5500_eth_test.py` code, and search any incorrect behaviour.
From the user perspective everything works as expected, the webpage loads.

Important consideration:
The w5500 driver is heavily modified `micropython/extmod/network_wiznet5k.c`.
Fully study existing codebase before making any decision, do not rely of past w5500 
experiences, because it is heavily modified.

It is a server program which sole purpose to serve the hello world html.
It is a single-client implementation, it means w5500 has only one rx and one tx buffer (16kB each),
so it can only serve one client at a time.

The following logs show multiple(3) page loads, and each of it was successful. So from the user 
point of view everything is alright.

Micropython console output(please note I manually pressed Ctrl-C at the end halting the code):


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
[W5500] socket_listen: success
Listening on 172.16.1.1:80
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial state - socket 0: 0x14 (LISTEN)
[W5500] socket_accept: loop 100 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 100)
[W5500] socket_accept: loop 200 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 200)
[W5500] socket_accept: socket ESTABLISHED!
[W5500] socket_accept: client IP: 172.16.1.2, port: 47908
[W5500] socket_accept: success (single-client mode)
Client connected from ('172.16.1.2', 47908)
[W5500] socket_recv: socket=0, len=1024, timeout=-1
[W5500] socket_recv: checking, RX_RSR=330
[W5500] socket_recv: data available (330 bytes), calling recv
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 330
[W5500] socket_recv: success, received 330 bytes
[W5500] socket_send: socket=0, len=242
[W5500] socket_send: success, sent 242 bytes
[W5500] socket_close: socket 0
[W5500] socket_close: calling WIZCHIP_EXPORT(close) on socket 0
[W5500] socket_close: before - state=0x1c (CLOSE_WAIT)
[W5500] socket_close: saved state - port=80, type=0x01, was_listening=1
[W5500] socket_close: after - state=0x00 (CLOSED)
[W5500] socket_close: re-listening on port 80
[W5500] socket_close: WIZCHIP_EXPORT(socket) returned 0
[W5500] socket_close: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_close: successfully re-listened
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial state - socket 0: 0x14 (LISTEN)
[W5500] socket_accept: socket ESTABLISHED!
[W5500] socket_accept: client IP: 172.16.1.2, port: 48572
[W5500] socket_accept: success (single-client mode)
Client connected from ('172.16.1.2', 48572)
[W5500] socket_recv: socket=0, len=1024, timeout=-1
[W5500] socket_recv: checking, RX_RSR=330
[W5500] socket_recv: data available (330 bytes), calling recv
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 330
[W5500] socket_recv: success, received 330 bytes
[W5500] socket_send: socket=0, len=242
[W5500] socket_send: success, sent 242 bytes
[W5500] socket_close: socket 0
[W5500] socket_close: calling WIZCHIP_EXPORT(close) on socket 0
[W5500] socket_close: before - state=0x1c (CLOSE_WAIT)
[W5500] socket_close: saved state - port=80, type=0x01, was_listening=1
[W5500] socket_close: after - state=0x00 (CLOSED)
[W5500] socket_close: re-listening on port 80
[W5500] socket_close: WIZCHIP_EXPORT(socket) returned 0
[W5500] socket_close: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_close: successfully re-listened
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial state - socket 0: 0x14 (LISTEN)
[W5500] socket_accept: socket ESTABLISHED!
[W5500] socket_accept: client IP: 172.16.1.2, port: 48588
[W5500] socket_accept: success (single-client mode)
Client connected from ('172.16.1.2', 48588)
[W5500] socket_recv: socket=0, len=1024, timeout=-1
[W5500] socket_recv: checking, RX_RSR=330
[W5500] socket_recv: data available (330 bytes), calling recv
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 330
[W5500] socket_recv: success, received 330 bytes
[W5500] socket_send: socket=0, len=242
[W5500] socket_send: success, sent 242 bytes
[W5500] socket_close: socket 0
[W5500] socket_close: calling WIZCHIP_EXPORT(close) on socket 0
[W5500] socket_close: before - state=0x1c (CLOSE_WAIT)
[W5500] socket_close: saved state - port=80, type=0x01, was_listening=1
[W5500] socket_close: after - state=0x00 (CLOSED)
[W5500] socket_close: re-listening on port 80
[W5500] socket_close: WIZCHIP_EXPORT(socket) returned 0
[W5500] socket_close: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_close: successfully re-listened
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial state - socket 0: 0x14 (LISTEN)
[W5500] socket_accept: loop 100 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 100)
Traceback (most recent call last):
  File "main.py", line 51, in <module>
KeyboardInterrupt:
MicroPython 372064113-dirty on 2026-01-14; RP2350 with 3.0MB CAM + W5500 with RP2350



