# Very important considerations

The w5500 driver is a heavily modified one, with non lwIP path, 
single socket (16kB RX and 16kB TX, 32kB in total), single client-server
architecture for maximum throughput.
Please note the `network_wiznet5k.c` is not using lwIP! Ituses hardware based tcp/ip of w5500.

# Description

I'm running on actual device the `main_test02b_html_hello_world_nodelay.py` program, which is a hello world in html.
So the whole job is to send a .html file to the client to see if it works.
The actual driver which is a heavily modified w5500 driver without lwIP, it is in `micropython/extmod/network_wiznet5k.c`. 
I tried to implement a circular buffer and filling the 16kB TX buffer of the w5500 chip.

Please note I tried to implement circular buffer, but 
even the simple html file fails to load now: it results on the browser 
NS_ERROR_CONNECTION_REFUSED error (in the developer console).
Please study the file at least 5 different angles, 
what would be the correct fix, find the bug.

The html file was served before, it was working (the `main_test02b_html_hello_world_nodelay`),
I have not changed spi configuration or speed, just tried to implement circular buffer with 
DMA in the `network_wiznet5k.c` driver.

Here is the micropython console output:
 mpremote
Connected to MicroPython at /dev/ttyACM0
Use Ctrl-] or Ctrl-x to exit this shell
[W5500] wiznet5k_active: calling wiznet5k_init()
[W5500] wiznet5k_init: initializing with provided TCP stack
[W5500] wiznet5k_init: calling ctlwizchip(CW_INIT_WIZCHIP, ...)
[W5500] wiznet5k_init: calling ctlnetwork(CN_SET_NETINFO, ...)
[W5500] wiznet5k_init: registering with network module
[W5500] w5500_tx_init: claimed TX channel 0
[W5500] wiznet5k_init: done, active=true
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
[W5500] socket_accept: loop 1300 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 1300)
[W5500] socket_accept: loop 1400 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 1400)
[W5500] socket_accept: loop 1500 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 1500)
[W5500] socket_accept: loop 1600 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 1600)
[W5500] socket_accept: loop 1700 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 1700)
[W5500] socket_accept: loop 1800 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 1800)
[W5500] socket_accept: loop 1900 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 1900)
[W5500] socket_accept: loop 2000 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 2000)
[W5500] socket_accept: loop 2100 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 2100)
[W5500] socket_accept: loop 2200 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 2200)
[W5500] socket_accept: loop 2300 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 2300)
[W5500] socket_accept: loop 2400 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 2400)
[W5500] socket_accept: loop 2500 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 2500)
[W5500] socket_accept: loop 2600 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 2600)
[W5500] socket_accept: loop 2700 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 2700)
[W5500] socket_accept: loop 2800 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 2800)
[W5500] socket_accept: loop 2900 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 2900)



---

FILELIST:
main_test02b_html_hello_world_nodelay.py
micropython/extmod/network_wiznet5k.c
micropython/extmod/machine_spi.c
micropython/extmod/modnetwork.c
micropython/extmod/modnetwork.h
micropython/extmod/modsocket.c

