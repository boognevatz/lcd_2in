
Analyze the following file, which is the main entry point:
`main_test_accept_now2.py`.

It is the simplest possible application which accepts 
telnet on port 8080.


Please not I modified both network_wiznet5k.c and modsocket.c 
to use w5500 native socket capabilities and be able to dma through 
spi a whole 150kB frame without python involvement. 
This is the future development, that is why there is no lwIP usage.
That is the main goal. Get the w5500 driver in a working state without
lwIP usage.

The current problem, is I can connect to the w5500 using telnet, 
but i type anything in telnet, it does nothing on the micrppython 
side, even if I quit telnet, the micropython side stuck at "Waiting for data from client...".

Here is the micrpython runtime output:
Connected to MicroPython at /dev/ttyACM0
Use Ctrl-] or Ctrl-x to exit this shell
IP: 172.16.1.1
[W5500] socket_socket: domain=2, type=1, proto=0
[W5500] socket_socket: TCP socket
[W5500] socket_socket: assigned socket number 0
[W5500] socket_socket: success, fileno=0
[W5500] socket_bind: socket=0, port=8080, type=0x01
[W5500] socket_bind: WIZCHIP_EXPORT(socket) returned 0
[W5500] socket_bind: socket 0 state after open: 0x13 (INIT)
[W5500] socket_bind: success
[W5500] socket_listen: socket=0, backlog=5
[W5500] socket_listen: before - socket 0 state: 0x13 (INIT)
[W5500] socket_listen: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_listen: after - socket 0 state: 0x14 (LISTEN)
[W5500] socket_listen: success

Listening on port 8080...
1. In another terminal, run: telnet 172.16.1.1 8080
2. Type something and press Enter
3. This should now work!
[W5500] socket_settimeout: socket=0, timeout_ms=30000000
[W5500] socket_settimeout: set to 30000000 ms

Waiting for connection...
[W5500] socket_accept: socket=0, timeout=30000000
[W5500] socket_accept: initial state - socket 0: 0x14 (LISTEN)
[W5500] socket_accept: loop 100 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 100)
[W5500] socket_accept: loop 200 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 200)
[W5500] socket_accept: still waiting... (loop 400)
[W5500] socket_accept: CONNECT interrupt detected!
[W5500] socket_accept: interrupt cleared, waiting for ESTABLISHED...
[W5500] socket_accept: new state: 0x17 (ESTABLISHED)
[W5500] socket_accept: socket ESTABLISHED!
[W5500] socket_accept: client IP: 172.16.1.2, port: 8080
[W5500] socket_accept: creating new listening socket...
[W5500] socket_socket: domain=2, type=1, proto=0
[W5500] socket_socket: TCP socket
[W5500] socket_socket: assigned socket number 1
[W5500] socket_socket: success, fileno=1
[W5500] socket_bind: socket=1, port=8080, type=0x01
[W5500] socket_bind: WIZCHIP_EXPORT(socket) returned 1
[W5500] socket_bind: socket 1 state after open: 0x13 (INIT)
[W5500] socket_bind: success
[W5500] socket_listen: socket=1, backlog=0
[W5500] socket_listen: before - socket 1 state: 0x13 (INIT)
[W5500] socket_listen: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_listen: after - socket 1 state: 0x14 (LISTEN)
[W5500] socket_listen: success
[W5500] socket_accept: new listening socket created successfully
[W5500] socket_accept: success

!!! SUCCESS! Accepted connection from ('172.16.1.2', 8080)
Waiting for data from client...

For completeness I paste all the relevant codes here.
If there any other file you need to look, list the filenames.
"micropython/lib/wiznet5k/Ethernet/wizchip_conf.c"
"micropython/lib/wiznet5k/Ethernet/wizchip_conf.h"
"micropython/lib/wiznet5k/Ethernet/W5500/w5500.c"
"micropython/lib/wiznet5k/Ethernet/W5500/w5500.h"
"micropython/extmod/network_wiznet5k.c"
"micropython/extmod/machine_spi.c"
"micropython/extmod/modnetwork.h"

---

FILELIST:
main_test_accept_now2.py
micropython/lib/wiznet5k/Ethernet/wizchip_conf.c
micropython/lib/wiznet5k/Ethernet/wizchip_conf.h
micropython/lib/wiznet5k/Ethernet/W5500/w5500.c
micropython/lib/wiznet5k/Ethernet/W5500/w5500.h
micropython/extmod/network_wiznet5k.c
micropython/extmod/machine_spi.c
micropython/extmod/modnetwork.h


