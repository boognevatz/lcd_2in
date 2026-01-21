
Take the main_test04_svg_50kb.py, it is the micropython starting application, 
study it thoroughly.
Take the `micropython/extmod/network_wiznet5k.c`, study it from at least 3 different angles. 

Also read the application log, and crossreference how the code flows.
Make a spi_log_analyze.md file, and write there what 
is calling what based on the micropython console log and the actual code.
After each spi communication there is a detailed print.
Do not deviate, only uses this code, do not take anything from the internet, this is the 
only source of truth.

Make a calling tree like 
main_test04_svg_50kb.py:284->
  s.accept()->
    network_wiznet5k.c:1564->
      wiznet5k_socket_accept()->
        DEBUG_PRINT()

The second job is to find out why it is frigging slow, the actual [xxx ms]
printouts are accurate, it really took 321second to respond to the browser.

By design we do not print out every spi communication, the frequent ones
are printed after 100 occurences. It is working in real life.

Please be  aware, the w5500 is a heavily modified driver, it does not use
lwIP, rather hardware tcp/ip. And w5500 uses a single 16kB RX 16kB TX on
socket  0, it is a single connection client-server architecture (the mcu,
ie. the  w550 is the server, and the client is a webbrowser).

Do not take any files from the internet. Everything is here included.

# Micropython log

 ❯ mpremote
Connected to MicroPython at /dev/ttyACM0
Use Ctrl-] or Ctrl-x to exit this shell
[W5500] w5500_dma_init: claimed TX channel 0
[W5500] wiznet5k_active: calling wiznet5k_init()
[W5500] wiznet5k_init: initializing with provided TCP stack
[W5500] wiznet5k_init: calling ctlwizchip(CW_INIT_WIZCHIP, ...)
 [859 ms] [SPI TX BURST, len: 3 ] 0x 00 09 00|
 [859 ms] [SPI TX BURST, len: 3 ] 0x 00 01 00|
 [859 ms] [SPI TX BURST, len: 3 ] 0x 00 05 00 [4359 ms] [SPI TX BURST, len: 3 ] 0x 00 00 00|
 [4359 ms] [SPI [8859 ms] [SPI TX BURST, len: 3 ] 0x 00 05 04|
 [8859 ms] [SPI [13359 ms] [SPI TX BURST, len: 4 ] 0x 00 1F 2C 00|
 [13359 ms] [17859 ms] [SPI TX BURST, len: 4 ] 0x 00 1F AC 00|
 [17859 ms] [22359 ms] [SPI TX BURST, len: 4 ] 0x 00 1E 2C 00|
 [22359 ms][W5500] wiznet5k_init: calling ctlnetwork(CN_SET_NETINFO, ...)
 [32859 ms] [SPI TX BURST, len: 6 ] 0x 00 00 00 00|
 [32859 ms][W5500] wiznet5k_init: registering with network module
[W5500] w5500_tx_pool_init: initialized transfer pool
[W5500] wiznet5k_init: done, active=true
 [39359 ms] [SPI TX BURST, len: 4 ] 0x  [46859 ms] [SPI TX BURST, len: 3 ] 0x 00 05 00|
 [46859 ms] [S [52359 ms] [SPI TX BURST, len: 3 ] 0x 00 10 08|
 [52359 ms] [S [57859 ms] [SPI TX BURST, len: 3 ] 0x 00 21 08|
 [57859 ms] [S [63359 ms] [SPI TX BURST, len: 3 ] 0x 00 26 08|
 [63359 ms] [S
========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: After wiznet5k_init (Initialization Complete)
--------------------------------------------------------
 [COMMON REGISTERS]
   Mode Register (MR):           0x00
   Gateway Address:              0.0.0.0
   Subnet Mask:                  0.0.0.0
   Source IP:                    0.0.0.0
   MAC Address:                  00:00:00:00:00:00
   Interrupt Register (IR):      0x00
   Interrupt Mask (IMR):         0x00
 [68859 ms] [SPI TX BURST, len: 3   PHY Config (PHYCFGR):         0xBF
   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
   Mode (Sn_MR):                 0x00
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x00 [CLOSED]
   Interrupt (Sn_IR):            0x00
   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):            0 (0x0000)
   Dest Port (Sn_DPORT):             0 (0x0000)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x0000
   TX Read Pointer (RD):         0x0000
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):           0 bytes (0x0000)
   RX Read Pointer (RD):         0x0000
   RX Write Pointer (WR):        0x0000
   RX Buffer Available:              0 bytes
   RX Buffer Capacity:           16384 bytes
========================================================

 [75360 ms] [SPI  [81861 ms] [SPI TX BURST, len: 3 ] 0x 00 09 04|
 [81861 ms] [SPI TX BURST, len: 6 ] 0x 02 0D BF EE|
 [82111 ms] [SPI TX BURST, len: 3 ] 0x 00 09 00|
 [82111 ms] [SPI TX BURST, len: 3 ] 0x 00 01 00|
 [82111 ms] [SPI TX BURST, len: 3 ] 0x 00 05 00|

 [82111 ms] [SPI TX BURST, len: 3 ] 0x 00 0F 00|
 [82111 ms] [SPI TX BURST, len: 3 ] 0x 00 09 04|
 [82111 ms] [ [85611 ms] [SPI TX BURST, len: 3 ] 0x 00 05 04|
 [85611 ms] [SWaiting for Ethernet link...
 [91111 ms] [SPI TX BURST, len: 3 ] 0x 00 2E 00|
 [91111 ms] [SPI TX BURST, len: 3 ] 0x 00 09 00|
 [91111 ms] [SPI TX BURST, len: 3 ] 0x 00 01 00|
 [91111 ms] [SPI TX BURST, len: 3 ] 0x 00 05 00|
 [91111 ms] [SPI TX BURST, len: 3 ] 0x 00 0F 00|
1 01|
Connected. IP address: 172.16.1.1
[W5500] socket_socket: domain=2, type=1, proto=0
[W5500] socket_socket: TCP socket
[W5500] socket_socket: assigned socket number 0
[W5500] socket_socket: success, fileno=0
[W5500] socket_bind: socket=0, port=80, type=0x01
 [91112 ms] [SPI TX BURST, len: 3 ] 0x 00 0F 00|
 [91112 ms] [SPI TX BURST, len: 4 ] 0x 00 01 0C 10|
 [91112 ms] [SPI T [95612 ms] [SPI TX BURST, len: 4 ] 0x 00 00 0C 01|
 [95612 ms][W5500] socket_bind: WIZCHIP_EXPORT(socket) returned 0
[W5500] socket_bind: socket 0 state after open: 0x13 (INIT)
[W5500] socket_bind: success
[W5500] socket_listen: socket=0, backlog=5
[W5500] socket_listen: before - socket 0 state: 0x13 (INIT)
 [101112 ms] [SPI TX BURST, len: 3 ] 0x 00 01 08|
 [101112 ms] [SPI[W5500] socket_listen: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_listen: after - socket 0 state: 0x14 (LISTEN)
[W5500] socket_listen: saved state - port=80, type=0x01
 [107612 ms] [SPI TX BURST, len: 3 ] 0x 00 04 08|
 [107612 ms] [SPI TX BURST,  [112612 ms] [SPI TX BURST, len: 3 ] 0x 00 09 00|
 [112612 ms]  [117612 ms] [SPI TX BURST, len: 3 ] 0x 00 20 08|
 [117612 ms]  [122612 ms] [SPI TX BURST, len: 3 ] 0x 00 25 08|
 [122612 ms]
========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: After socket_listen (Listening Started)
--------------------------------------------------------
 [COMMON REGISTERS]
 [127612 ms] [   Mode Register (MR):           0x00
   Gateway Address:              172.16.1.1
   Subnet Mask:                  255.255.255.0
   Source IP:                    172.16.1.1
   MAC Address:                  02:0D:BF:EE:95:EA
   Interrupt Register (IR):      0x00
   Interrupt Mask (IMR):         0x00
   PHY Config (PHYCFGR):         0xBF
   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
 [133612 ms] [SPI TX BURST, len: 3 ] 0x 00 15    Mode (Sn_MR):                 0x01
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x14 [LISTEN]
   Interrupt (Sn_IR):            0x00
   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):           80 (0x0050)
   Dest Port (Sn_DPORT):             0 (0x0000)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x0000
   TX Read Pointer (RD):         0x0000
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):           0 bytes (0x0000)
   RX Read Pointer (RD):         0x0000
   RX Write Pointer (WR):        0x0000
   RX Buffer Available:              0 bytes
   RX Buffer Capacity:           16384 bytes
========================================================

[W5500] socket_listen: success
 [139613 ms] [SPI TX BURST, len: 3 ] 0x 00 01 08|
 [139613 ms] [SPI TX BURST, lListening on 172.16.1.1:80
[MAIN] ====== WAITING FOR CONNECTION ======
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial SR=0x14 (LISTEN)
 [144614 ms] [SPI TX BURST, len: 3 ] 0x 00 0F 00|
[145915 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[150915 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [153416 ms] [SPI TX BURST, len: 3 , 90 100 0x000208 0x000308 skip] 0x 00 03 08|
3 ] 0x 00 05 00|

[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 200 SR=0x14 (LISTEN) IR=0x00
[155916 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[160916 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [163326 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[165916 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 ] 0x 00 05 00|

[W5500] socket_accept: loop 300 SR=0x14 (LISTEN) IR=0x00
[170916 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [173316 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[175916 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[180916 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 ] 0x 00 05 00|

[W5500] socket_accept: loop 400 SR=0x14 (LISTEN) IR=0x00
 [183226 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[185916 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[190916 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [193216 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 500 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 600 SR=0x14 (LISTEN) IR=0x00
[195917 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[200917 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [203127 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[205917 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 700 SR=0x14 (LISTEN) IR=0x00
[210917 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [213117 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[215917 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[220917 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 800 SR=0x14 (LISTEN) IR=0x00
 [223027 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[225917 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[230917 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [233017 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 900 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 1000 SR=0x14 (LISTEN) IR=0x00
[235917 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[240917 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [242927 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[245917 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 1100 SR=0x14 (LISTEN) IR=0x00
[250917 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [252917 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[255918 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[260919 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 1200 SR=0x14 (LISTEN) IR=0x00
 [262829 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[265920 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[270921 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [272821 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 1300 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 1400 SR=0x14 (LISTEN) IR=0x00
[275922 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[280922 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [282732 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[285922 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 1500 SR=0x14 (LISTEN) IR=0x00
[290922 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [292722 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[295923 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[300924 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 1600 SR=0x14 (LISTEN) IR=0x00
 [302634 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[305925 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[310925 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [312625 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 1700 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 1800 SR=0x14 (LISTEN) IR=0x00
[315925 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[320925 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [322535 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[325925 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 1900 SR=0x14 (LISTEN) IR=0x00
[330926 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [332526 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[335926 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[340926 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 2000 SR=0x14 (LISTEN) IR=0x00
 [342436 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[345926 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[350926 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [352426 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
 [352637 ms] [SPI TX BURST, len: 4 ] 0x 00 02 0C 01|
 [352637 ms] [SPI TX BURST, len: 3 ] 0x 00 0C 08|
 [352637 ms] [SPI TX BURST, len: 3 ] 0x 00 10 08|
 [352637 ms] [SPI TX BURST, len: 3 ] 0x 00 11 08|
 BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
 [352637 ms] [SPI TX BURST, len: 3 ] 0x 00 01 00|
 [352637 ms] [SPI TX BURST, len: 3 ] 0x 00 05 00|
  [357137 ms] [SPI TX BURST, len: 3 ] 0x 00 05 08|
 [357137 ms]  [362637 ms] [SPI TX BURST, len: 3 ] 0x 00 20 08|
 [362637 ms]  [368137 ms] [SPI TX BURST, len: 3 ] 0x 00 23 08|
 [368137 ms]
========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: After socket_accept (ESTABLISHED)
--------------------------------------------------------
 [COMMON REGISTERS]
 [373637 ms] [SPI TX   Mode Register (MR):           0x00
   Gateway Address:              172.16.1.1
   Subnet Mask:                  255.255.255.0
   Source IP:                    172.16.1.1
   MAC Address:                  02:0D:BF:EE:95:EA
   Interrupt Register (IR):      0x00
   Interrupt Mask (IMR):         0x00
   PHY Config (PHYCFGR):         0xBF
   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
 [380137 ms] [SPI TX BURST, len: 3 ] 0x 00 15    Mode (Sn_MR):                 0x01
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x17 [ESTABLISHED]
   Interrupt (Sn_IR):            0x04
   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):           80 (0x0050)
   Dest Port (Sn_DPORT):         33156 (0x8184)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x1428
   TX Read Pointer (RD):         0x1428
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):         330 bytes (0x014A)
   RX Read Pointer (RD):         0x0000
   RX Write Pointer (WR):        0x014A
   RX Buffer Available:            330 bytes
   RX Buffer Capacity:           16384 bytes
========================================================

[W5500] socket_accept: success
[MAIN] accept() returned, addr=('172.16.1.2', 33156)
[W5500] socket_recv: socket=0, len=16384, timeout=-1
 [386638 ms] [SPI TX BURST, len[W5500] socket_recv: checking, RX_RSR=330
[W5500] socket_recv: data available (330 bytes), calling recv
 [393140 ms] [SPI TX BURST, len: 3 ] 0x 00 27 08|
 [393140 ms] [SPI TX BURST, len: 3  [398640 ms] [SPI TX BURST, len: 3 ] 0x 00 04 08|
 [398640 ms]  [404140 ms] [SPI TX BURST, len: 3 ] 0x 00 21 08|
 [404140 ms]  [409640 ms] [SPI TX BURST, len: 3 ] 0x 00 22 08|
 [409640 ms]  [415140 ms] [SPI TX BURST, len: 3 ] 0x 00 27 08|
 [415140 ms]
========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: Before socket_recv (Data Available)
--------------------------------------------------------
 [COMMON REGISTERS]
   Mode Register (MR):           0x00
   Gateway Address:              172.16.1.1
   Subnet Mask:                  255.255.255.0
   Source IP:                    172.16.1.1
   MAC Address:                  02:0D:BF:EE:95:EA
   Interrupt Register (IR):      0x00
   Interrupt Mask (IMR):         0x00
   PHY Config (PHYCFGR):         0xBF
 [420640 ms] [SPI TX BURST, len: 3 ] 0x 00 00 00|
 [420   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
   Mode (Sn_MR):                 0x01
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x17 [ESTABLISHED]
   Interrupt (Sn_IR):            0x04
 [426141 ms] [SPI TX BURST, le   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):           80 (0x0050)
   Dest Port (Sn_DPORT):         33156 (0x8184)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x1428
   TX Read Pointer (RD):         0x1428
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):         330 bytes (0x014A)
   RX Read Pointer (RD):         0x0000
   RX Write Pointer (WR):        0x014A
   RX Buffer Available:            330 bytes
   RX Buffer Capacity:           16384 bytes
========================================================

 [432642 ms] [SPI TX BURST, len: 3 ] 0x 00 00 08|
 [432642 ms] [SPI TX BURST, len: 3 [438142 ms] [SPI TX BURST, len: 3 ] 0x 00 27 08|
 [438142 ms] [W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 330
[RECV DATA] Length: zu bytes: 47 45 54 20
 [443642 ms] [SPI TX BURST, len: 4 ] 0x 00 29 0C 4A|
 [443642 ms] [SPI TX BURST, len: 4 ] 0 [449142 ms] [SPI TX BURST, len: 3 ] 0x 00 05 00|
 [449142 ms]  [455642 ms] [SPI TX BURST, len: 3 ] 0x 00 10 08|
 [455642 ms]  [462142 ms] [SPI TX BURST, len: 3 ] 0x 00 21 08|
 [462142 ms]  [468642 ms] [SPI TX BURST, len: 3 ] 0x 00 26 08|
 [468642 ms]
========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: After socket_recv (Data Received)
--------------------------------------------------------
 [COMMON REGISTERS]
   Mode Register (MR):           0x00
   Gateway Address:              172.16.1.1
   Subnet Mask:                  255.255.255.0
   Source IP:                    172.16.1.1
   MAC Address:                  02:0D:BF:EE:95:EA
   Interrupt Register (IR):      0x00
   Interrupt Mask (IMR):         0x00
 [475142 ms] [SPI TX BURST, len:    PHY Config (PHYCFGR):         0xBF
   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
   Mode (Sn_MR):                 0x01
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x17 [ESTABLISHED]
   Interrupt (Sn_IR):            0x04
   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):           80 (0x0050)
   Dest Port (Sn_DPORT):         33156 (0x8184)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x1428
   TX Read Pointer (RD):         0x1428
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):           0 bytes (0x0000)
   RX Read Pointer (RD):         0x014A
   RX Write Pointer (WR):        0x014A
   RX Buffer Available:              0 bytes
   RX Buffer Capacity:           16384 bytes
========================================================

[W5500] socket_recv: success, received 330 bytes
[MAIN] recv() returned 330 bytes
[MAIN] Request decoded, length=330
[MAIN] Request split into 11 lines
[MAIN] First line: GET / HTTP/1.1...
[MAIN] Split into 3 parts
[MAIN] Parsed: method=GET, path=/
[MAIN] Request path: /
[MAIN] Calling generate_large_html_response()
[MAIN] HTML generated, size=6467
Full response size: 6630 bytes (headers: 163, body: 6467)
Response generation time: 1ms
[MAIN] cl.send() is about to be executed
[W5500] socket_send: socket=0, len=6630
[SEND DATA] Length: zu bytes: 48 54 54 50
 [482643 ms] [SPI TX BURST, len: 3 ] 0x 00 39 00|
 [482643 ms] [SPI TX BU [489166 ms] [SPI TX BURST, len: 3 ] 0x 00 05 00|
 [489166 ms]  [495666 ms] [SPI TX BURST, len: 3 ] 0x 00 10 08|
 [495666 ms]  [502166 ms] [SPI TX BURST, len: 3 ] 0x 00 21 08|
 [502166 ms]  [508666 ms] [SPI TX BURST, len: 3 ] 0x 00 26 08|
 [508666 ms]
========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: Before socket_send
--------------------------------------------------------
 [COMMON REGISTERS]
   Mode Register (MR):           0x00
   Gateway Address:              172.16.1.1
   Subnet Mask:                  255.255.255.0
   Source IP:                    172.16.1.1
   MAC Address:                  02:0D:BF:EE:95:EA
   Interrupt Register (IR):      0x00
   Interrupt Mask (IMR):         0x00
 [515166 ms] [SPI TX BURST, len: 3 ] 0x 00 2B 08   PHY Config (PHYCFGR):         0xBF
   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
   Mode (Sn_MR):                 0x01
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x17 [ESTABLISHED]
   Interrupt (Sn_IR):            0x04
   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):           80 (0x0050)
   Dest Port (Sn_DPORT):         33156 (0x8184)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x1428
   TX Read Pointer (RD):         0x1428
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):           0 bytes (0x0000)
   RX Read Pointer (RD):         0x014A
   RX Write Pointer (WR):        0x014A
   RX Buffer Available:              0 bytes
   RX Buffer Capacity:           16384 bytes
========================================================

[W5500] socket_send: using DMA transfer for 6630 bytes
 [522667 ms] [SPI TX[W5500] socket_send: queued transfer, initial chunk size=6630
 [530169 ms] [SPI TX BURST, len: 3 ] 0x 00 21 08|
 [530169 ms] [ [536669 ms] [SPI TX BURST, len: 3 ] 0x 00 21 08|
 [536669 ms] [W5500] w5500_send_chunk: sn=0, len=6630, tx_wr_ptr=5160, offset=5160, space_to_end=11224
[SPI] DMA Write to TX Buffer | Addr: 0x00142810 | Len: zu
[TX] Length: zu bytes: 48 54 54 50
[W5500] w5500_dma_write_to_txbuf: wrote 6630 bytes to socket 0 offset 5160
[W5500] w5500_tx_service: started DMA for 6630 bytes on socket 0
 [543169 ms] [SPI TX BURST, len: 3 ] 0x 00 1F 08|
 [543169 ms] [SPI TX HDR, len: 3 ] 0x 14 28 14|
 [543169 ms] [SPI TX DMA DATA, len: 6630 ] 0x 48 54 54 50|
 [543176 ms] [SPI TX BURST, len: 3 ] 0x 00 24 08|
9 ms] [SPI TX BURST, len: 3 ] 0x 00 25 08|
1|
 00 03 08|
 [543176 ms] [SPI TX BURST, len: 3 ] 0x 00 25 08|
 [543176 ms] [SPI TX BURST, len: 4 ] 0x 00 24 0[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 6630/6630 bytes sent on socket 0
 [549676 ms] [SPI TX BURST, len: 3 ] 0x 00 01 08|
 [549677 ms] [SPI TX BURST, len: 4 ] 0x 00 02 0C 10|
 [549678 ms] [SPI TX BURST, len: 3 ] 0x 00 01 00|
 [549678 ms] [SPI TX BURST, len: 3 ] 0x 00 05 00|
C 20|
ms] [SPI TX BURST, len: 3 ] 0x 00 25 08|
1|
 00 03 08|
 [549678 ms] [SPI TX BURST, len: 3 ] 0x 00 0F 00|
 [549678 ms] [SPI TX BURST, len: 3 ] 0x 00 09  [556178 ms] [SPI TX BURST, len: 3 ] 0x 00 11 08|
 [556178 ms]  [562678 ms] [SPI TX BURST, len: 3 ] 0x 00 24 08|
 [562678 ms]  [569178 ms] [SPI TX BURST, len: 3 ] 0x 00 26 08|
 [569178 ms]
========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: After socket_send (Transfer Complete)
--------------------------------------------------------
 [COMMON REGISTERS]
   Mode Register (MR):           0x00
   Gateway Address:              172.16.1.1
   Subnet Mask:                  255.255.255.0
   Source IP:                    172.16.1.1
   MAC Address:                  02:0D:BF:EE:95:EA
   Interrupt Register (IR):      0x00
   Interrupt Mask (IMR):         0x00
 [576178 ms] [SPI TX BURST, len: 3 ] 0x 00 2B 08|
 [576178 ms] [SPI TX BURST, len: 3 ] 0x 00   PHY Config (PHYCFGR):         0xBF
   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
   Mode (Sn_MR):                 0x01
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x1C [CLOSE_WAIT]
   Interrupt (Sn_IR):            0x06
   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):           80 (0x0050)
   Dest Port (Sn_DPORT):         33156 (0x8184)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x2E0E
   TX Read Pointer (RD):         0x2E0E
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):           0 bytes (0x0000)
   RX Read Pointer (RD):         0x014A
   RX Write Pointer (WR):        0x014A
   RX Buffer Available:              0 bytes
   RX Buffer Capacity:           16384 bytes
========================================================

[W5500] socket_send: transfer complete, sent 6630 bytes
[MAIN] Calling cl.close()
[W5500] socket_close: socket 0
 [583179 ms] [SPI TX BURST, len: 3 ] 0x 00 39 00|
 [583179 ms] [SPI TX BURST, len: 3 ] 0 [590181 ms] [SPI TX BURST, len: 3 ] 0x 00 05 00|
 [590181 ms]  [597181 ms] [SPI TX BURST, len: 3 ] 0x 00 10 08|
 [597181 ms]  [604181 ms] [SPI TX BURST, len: 3 ] 0x 00 21 08|
 [604181 ms]  [611181 ms] [SPI TX BURST, len: 3 ] 0x 00 26 08|
 [611181 ms]
========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: Before socket_close
--------------------------------------------------------
 [COMMON REGISTERS]
   Mode Register (MR):           0x00
   Gateway Address:              172.16.1.1
   Subnet Mask:                  255.255.255.0
   Source IP:                    172.16.1.1
   MAC Address:                  02:0D:BF:EE:95:EA
   Interrupt Register (IR):      0x00
   Interrupt Mask (IMR):         0x00
 [618181 ms] [SPI TX BURST, len: 3 ] 0x 00 2B 0   PHY Config (PHYCFGR):         0xBF
   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
   Mode (Sn_MR):                 0x01
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x1C [CLOSE_WAIT]
   Interrupt (Sn_IR):            0x06
   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):           80 (0x0050)
   Dest Port (Sn_DPORT):         33156 (0x8184)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x2E0E
   TX Read Pointer (RD):         0x2E0E
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):           0 bytes (0x0000)
   RX Read Pointer (RD):         0x014A
   RX Write Pointer (WR):        0x014A
   RX Buffer Available:              0 bytes
   RX Buffer Capacity:           16384 bytes
========================================================

[W5500] socket_close: cancelling active transfer on socket 0
[W5500] socket_close: calling WIZCHIP_EXPORT(close) on socket 0
[W5500] socket_close: before - state=0x1c (CLOSE_WAIT)
[W5500] socket_close: saved state - port=80, type=0x01, was_listening=1
 [626182 ms] [SPI TX BURST, len: 3 ] 0x 00 39 00|
 [626182 ms] [SPI TX BURST, len: 3 ] 0x 00 00 08|
 [626182 ms] [SPI TX BURST, len: 3 ] 0x 00 01 08|
 [626182 ms] [SPI TX BURST, len: 3 ] 0x 00 2C 08|
 [626234 ms] [SPI TX BURST, len: 4 ] 0x 00 01 0C 10|
 00 03 08|
[W5500] socket_close: after - state=0x00 (CLOSED)
[W5500] socket_close: re-listening on port 80
 [626234 ms] [SPI TX BURST, len: 3 ] 0x 00 01 08|
 [626234 ms] [SPI TX BURST, len: 4 ] 0x 00 02 0C 1F|
 [626284 ms] [SPI TX BURST, len: 3 ] 0x 00 0F 00|
 [626284 ms] [SPI TX BURST, len: 4 ] 0x 00 01 0C 10|
34 ms] [SPI TX BURST, len: 4 ] 0x 00 01 0C 10|
 0 [627784 ms] [SPI TX BURST, len: 3 ] 0x 00 01 08|
 [627784 ms] [W5500] socket_close: WIZCHIP_EXPORT(socket) returned 0
 [63328 [640784 ms] [SPI TX BURST, len: 3 ] 0x 00 27 08|
 [640784 ms] [W5500] socket_close: after reopen - RX_RSR=0, TX_FSR=16384/16384
 [647284 ms] [SPI TX BURST, len: 3 ] 0x 00 1F 08|
 [647284 ms] [SPI TX BURST, len: 3 ] 0x 00 00 08|
 [647284 ms] [SPI TX BU[W5500] socket_close: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_close: successfully re-listened
 [652784 ms] [SPI TX BURST, len: 3 ] 0x 00 01 00|
 [652784 ms] [SPI TX BURST, len: 3 ] [659284 ms] [SPI TX BURST, len: 3 ] 0x 00 05 08|
 [659284 ms]  [665784 ms] [SPI TX BURST, len: 3 ] 0x 00 20 08|
 [665784 ms]  [672284 ms] [SPI TX BURST, len: 3 ] 0x 00 23 08|
 [672284 ms]
========================================================
           W5500 REGISTER STATE DUMP
========================================================
 Context: After socket_close
--------------------------------------------------------
 [COMMON REGISTERS]
   Mode Register (MR):           0x00
   Gateway Address:              172.16.1.1
   Subnet Mask:                  255.255.255.0
   Source IP:                    172.16.1.1
   MAC Address:                  02:0D:BF:EE:95:EA
   Interrupt Register (IR):      0x00
 [678784 ms] [SPI TX BURST, len: 3 ] 0x 00 2A 08|
 [678784 ms] [SPI TX BURST, len: 3 ]   Interrupt Mask (IMR):         0x00
   PHY Config (PHYCFGR):         0xBF
   Version (VERSIONR):           0x04
--------------------------------------------------------
 [SOCKET 0 REGISTERS]
   Mode (Sn_MR):                 0x01
   Command (Sn_CR):              0x00
   Status (Sn_SR):               0x14 [LISTEN]
   Interrupt (Sn_IR):            0x00
 [685285 ms] [SPI TX BURST, len: 3 ] 0x 00 2E 00|
 [685285 ms] [SPI TX BURST, len: 3   Interrupt Mask (Sn_IMR):      0x1F
   Source Port (Sn_PORT):           80 (0x0050)
   Dest Port (Sn_DPORT):         33156 (0x8184)
--------------------------------------------------------
 [TX BUFFER STATE]
   TX Free Size (FSR):           16384 bytes (0x4000)
   TX Write Pointer (WR):        0x0000
   TX Read Pointer (RD):         0x0000
   TX Buffer Used:                   0 bytes
   TX Buffer Capacity:           16384 bytes
--------------------------------------------------------
 [RX BUFFER STATE]
   RX Received Size (RSR):           0 bytes (0x0000)
   RX Read Pointer (RD):         0x0000
   RX Write Pointer (WR):        0x0000
   RX Buffer Available:              0 bytes
   RX Buffer Capacity:           16384 bytes
========================================================

[MAIN] close() returned successfully
[MAIN] ====== WAITING FOR CONNECTION ======
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial SR=0x14 (LISTEN)
[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00
[692036 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[697036 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [699136 ms] [SPI TX BURST, len: 3 , 84 100 0x000208 0x000308 skip] 0x 00 03 08|
[702036 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 ] 0x 00 2C 08|
0|
 00 03 08|
[W5500] socket_accept: loop 200 SR=0x14 (LISTEN) IR=0x00
[707036 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [709046 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[712036 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[717036 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 ] 0x 00 2C 08|
0|
 00 03 08|
[W5500] socket_accept: loop 300 SR=0x14 (LISTEN) IR=0x00
 [719036 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[722037 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[727037 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [728947 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 400 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 500 SR=0x14 (LISTEN) IR=0x00
[732037 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[737037 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [738938 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[742038 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 600 SR=0x14 (LISTEN) IR=0x00
[747038 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [748848 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[752039 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[757039 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 700 SR=0x14 (LISTEN) IR=0x00
 [758839 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[762040 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[767040 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [768750 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 800 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 900 SR=0x14 (LISTEN) IR=0x00
[772040 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[777040 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [778740 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[782040 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 1000 SR=0x14 (LISTEN) IR=0x00
[787040 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [788650 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[792040 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[797040 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 1100 SR=0x14 (LISTEN) IR=0x00
 [798640 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[802040 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[807041 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [808551 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 1200 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 1300 SR=0x14 (LISTEN) IR=0x00
[812041 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[817041 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [818541 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[822041 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 1400 SR=0x14 (LISTEN) IR=0x00
[827041 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [828451 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[832041 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[837041 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 1500 SR=0x14 (LISTEN) IR=0x00
 [838441 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[842042 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[847042 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [848352 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 1600 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 1700 SR=0x14 (LISTEN) IR=0x00
[852042 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[857042 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [858342 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[862042 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 1800 SR=0x14 (LISTEN) IR=0x00
[867042 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [868252 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[872042 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[877043 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 1900 SR=0x14 (LISTEN) IR=0x00
 [878243 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[882043 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[887044 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [888154 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 2000 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 2100 SR=0x14 (LISTEN) IR=0x00
[892044 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[897044 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [898145 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[902045 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 2200 SR=0x14 (LISTEN) IR=0x00
[907045 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [908055 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[912046 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[917046 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 2300 SR=0x14 (LISTEN) IR=0x00
 [918046 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[922046 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[927046 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [927956 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 2400 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 2500 SR=0x14 (LISTEN) IR=0x00
[932046 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[937046 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [937946 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[942047 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 2600 SR=0x14 (LISTEN) IR=0x00
[947047 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [947857 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[952047 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[957047 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 2700 SR=0x14 (LISTEN) IR=0x00
 [957847 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[962047 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[967047 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [967757 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 2800 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 2900 SR=0x14 (LISTEN) IR=0x00
[972047 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[977047 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [977747 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[982048 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 3000 SR=0x14 (LISTEN) IR=0x00
[987048 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [987658 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[992048 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[997048 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 3100 SR=0x14 (LISTEN) IR=0x00
 [997648 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[1002049 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[1007049 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1007559 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 3200 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 3300 SR=0x14 (LISTEN) IR=0x00
[1012050 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[1017051 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1017551 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[1022052 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 3400 SR=0x14 (LISTEN) IR=0x00
[1027052 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1027462 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[1032052 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[1037052 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 3500 SR=0x14 (LISTEN) IR=0x00
 [1037452 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[1042053 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[1047054 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1047364 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 3600 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 3700 SR=0x14 (LISTEN) IR=0x00
[1052054 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[1057054 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1057354 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[1062054 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 3800 SR=0x14 (LISTEN) IR=0x00
[1067054 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1067264 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[1072054 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[1077054 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 3900 SR=0x14 (LISTEN) IR=0x00
 [1077254 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[1082054 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[1087054 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1087164 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 4000 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 4100 SR=0x14 (LISTEN) IR=0x00
[1092055 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[1097055 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1097155 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[1102056 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 02 08|
[W5500] socket_accept: loop 4200 SR=0x14 (LISTEN) IR=0x00
[1107057 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1107067 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[1112057 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1117057 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 4300 SR=0x14 (LISTEN) IR=0x00
[1117057 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[1122057 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1126967 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[1127057 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 4400 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 4500 SR=0x14 (LISTEN) IR=0x00
[1132058 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1136958 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[1137058 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[1142058 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 4600 SR=0x14 (LISTEN) IR=0x00
 [1146868 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[1147058 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[1152058 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1156858 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 4700 SR=0x14 (LISTEN) IR=0x00
[1157058 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[1162058 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1166768 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[1167058 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 4800 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 4900 SR=0x14 (LISTEN) IR=0x00
[1172058 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1176758 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[1177058 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[1182058 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 5000 SR=0x14 (LISTEN) IR=0x00
 [1186668 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[1187058 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[1192059 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1196659 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 5100 SR=0x14 (LISTEN) IR=0x00
[1197059 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[1202060 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1206570 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[1207060 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 5200 SR=0x14 (LISTEN) IR=0x00
[W5500] socket_accept: loop 5300 SR=0x14 (LISTEN) IR=0x00
[1212060 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1216560 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[1217060 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[1222060 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
8 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 5400 SR=0x14 (LISTEN) IR=0x00
 [1226470 ms] [SPI TX BURST, len: 3 , 100 99 0x000208 0x000308 skip] 0x 00 02 08|
[1227060 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
[1232061 ms] [SPI RX, len: 1 , 100 skipped] 0x14  |
 [1236461 ms] [SPI TX BURST, len: 3 , 99 100 0x000208 0x000308 skip] 0x 00 03 08|
[W5500] socket_accept: loop 5500 SR=0x14 (LISTEN) IR=0x00

---

FILELIST:
micropython/extmod/network_wiznet5k.c
micropython/extmod/machine_spi.c
micropython/lib/wiznet5k/Ethernet/wizchip_conf.h
micropython/lib/wiznet5k/Ethernet/W5500/w5500.h
main_test04_svg_50kb.py 

