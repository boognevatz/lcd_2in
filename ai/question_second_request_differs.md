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
Also an important consideration:
with lwIP path, there is no freeze. Just it is slow (cpu intensive packet choping).

The socket close and reopen happens in python land (before was in C land), 
but still the exact same problem.

Please be  aware, the w5500 is a heavily modified driver, it does not use
lwIP, rather hardware tcp/ip. And w5500 uses a single 16kB RX 16kB TX on
socket  0, it is a single connection client-server architecture (the mcu,
ie. the  w550 is the server, and the client is a webbrowser).

Do not take any files from the internet. Everything is here included.

# Your job

Still we need to figure out why the freezing can happen.
My hunch is some C variable points to old memory location, and simply freezes
the micropython.

Compare the two request logs, especially these two line:
1. request: 
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=10030659, sn=0, buf=20033180, len=8192

2. request:
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=10030659, sn=0, buf=20035220, len=8192

This is the programcode:

      DEBUG_PRINT("after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=%p, sn=%u, buf=%p, len=%u\n", 
                  WIZCHIP_EXPORT(recv), sn, buf, (unsigned)len);

      mp_hal_delay_ms(50);
      mp_int_t ret = WIZCHIP_EXPORT(recv)(sn, buf, len);


Is it alright? Why the recv is the same? And why the buf is different?

---

FILELIST:
main_test04_svg_50kb.py
micropython/extmod/network_wiznet5k.c
micropython/lib/wiznet5k/Ethernet/wizchip_conf.h
micropython/lib/wiznet5k/Ethernet/wizchip_conf.c
micropython/lib/wiznet5k/Ethernet/socket.c

