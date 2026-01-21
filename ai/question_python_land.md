# Actual problem

Take the main_test04_svg_50kb.py, it is the micropython starting application, 
study it thoroughly.
Take the `micropython/extmod/network_wiznet5k.c`, study it from at least 3 different angles. 

The main problem is the second request makes the micropython freeze.
The w5500 chip do not freeze, as it responds to ping (icmp) requests, 
however any real request (http /get) results in NS_ERROR_CONNECTION_REFUSED.

The first request is fine it returns data, the driver closes the socket and 
reopens it. It goes to the listen state, but when the second connection happens
it can not read the actual data it freezes.

The two request is identical, also results the same socket state transitions:
(listen, established, close_wait, listen, established (freeze))
Also an important consideration:
with lwIP path, there is no freeze. Just it is slow (cpu intensive packet choping).

Please be  aware, the w5500 is a heavily modified driver, it does not use
lwIP, rather hardware tcp/ip. And w5500 uses a single 16kB RX 16kB TX on
socket  0, it is a single connection client-server architecture (the mcu,
ie. the  w550 is the server, and the client is a webbrowser).

Do not take any files from the internet. Everything is here included.

# Your job

In network_wiznet5k.c when closing a socket do not relisten it immediatly, 
instead give back the control to the python script, and python should handle 
the closing and reopening functionality. Don't be too smart in C land.

---

FILELIST:
micropython/extmod/network_wiznet5k.c
micropython/lib/wiznet5k/Ethernet/wizchip_conf.h
micropython/lib/wiznet5k/Ethernet/W5500/w5500.h
main_test04_svg_50kb.py

