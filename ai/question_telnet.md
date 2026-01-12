
Analyze the following file, which is the main entry point:
`main_test_accept_now.py`.

It is the simplest possible application which accepts 
telnet on port 8080.


Please not I modified both network_wiznet5k.c and modsocket.c 
to use w5500 native socket capabilities and be able to dma through 
spi a whole 150kB frame without python involvement. 
This is the future development, that is why there is no lwIP usage.
That is the main goal. Get the w5500 driver in a working state without
lwIP usage.


For completeness I paste all the relevant codes here.
If there any other file you need to look, list the filenames.
---

FILELIST:
main_test_accept_now.py
micropython/lib/wiznet5k/Ethernet/wizchip_conf.c
micropython/lib/wiznet5k/Ethernet/wizchip_conf.h
micropython/lib/wiznet5k/Ethernet/W5500/w5500.c
micropython/lib/wiznet5k/Ethernet/W5500/w5500.h
micropython/extmod/network_wiznet5k.c
micropython/extmod/machine_spi.c
micropython/extmod/modnetwork.c
micropython/extmod/modnetwork.h
micropython/extmod/modsocket.c


