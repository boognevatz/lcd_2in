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

The goal is to log some critical variable around the freeze point, 
maybe inside WIZCHIP_EXPORT. The freezing happens in
wiznet5k_socket_recv, most possibly around this line:
      DEBUG_PRINT("before MP_THREAD_GIL_EXIT\n");
      mp_hal_delay_ms(50);
      MP_THREAD_GIL_EXIT();
      mp_hal_delay_ms(50);
      DEBUG_PRINT("after MP_THREAD_GIL_EXIT\n");
      mp_hal_delay_ms(50);
here->      mp_int_t ret = WIZCHIP_EXPORT(recv)(sn, buf, len);
      mp_hal_delay_ms(50);
      DEBUG_PRINT("before MP_THREAD_GIL_ENTER\n");
      mp_hal_delay_ms(50);
      MP_THREAD_GIL_ENTER();
      mp_hal_delay_ms(50);
      DEBUG_PRINT("after MP_THREAD_GIL_ENTER\n");


So sprinkle some printing inside wizchip_export.

---

FILELIST:
main_test04_svg_50kb.py
micropython/extmod/network_wiznet5k.c
micropython/lib/wiznet5k/Ethernet/wizchip_conf.h
micropython/lib/wiznet5k/Ethernet/wizchip_conf.c
micropython/lib/wiznet5k/Ethernet/socket.c

