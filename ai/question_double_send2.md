#Goal

Make a detailed action plan, and save it `to action_plan_double_send2.md`,
study in details the "Problem found?" chapter, and base your action plan on it.

## Application description


It is a webpage serving application, where it serves a very big (~53kB) .svg file 
at the /svg endpoint. It is clearly bigger then the TX buffer (16kB).

Give extra attention to DMA handling. The w5500 should use DMA. 


## Problem Summary

First 172.16.1.1/svg is successful, I receive the whole .svg file nothing truncated.
The second call (loading again) fails.

The MCU hard locks with no timeout messages or error output.

The below logs reflects the following:
http://172.16.1.1 loading, which also loads the 172.16.1.1/svg url, which freezes completely.

# Architecture

| Component | Details |
|-----------|---------|
| MCU | RP2350A (Raspberry Pi Pico 2) |
| Ethernet | W5500 via SPI0 @ 20MHz |
| Camera | OV5640 via PIO + DMA |
| Frame size | 240x320 RGB565 = 153.6KB |
| TX buffer | 16KB (Socket 0) |
| RX buffer | 16KB (Socket 0) |


# Goal

Cross-reference everything, with at least 9 angles.
Here are the files of interests:
main_test04_svg_50kb.py
micropython/extmod/network_wiznet.c

Also list if any other files worth mentioning and looked into.
Come up with an action plan for potential fixes of the identified 
bug or bugs.

Also note, that although the logs ends abrubtly ([W5500] socket_accept: client IP: 172.16.1.2, port),
it does not indicate the concrete freezing point in program code, since 
there are multiple hardware incolved: dma, spi, spi with w5500, 
which are somewhat parallel from the main program, so the actual 
freeze may happened earlier in the code, just needed some minuscule time to freezethe 
main program completely.

# Micropython logs

✗ mpremote
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
[MAIN] ====== WAITING FOR CONNECTION ======
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
[W5500] socket_accept: socket ESTABLISHED!
[W5500] socket_accept: RX_RSR=330, TX_FSR=16384 (expected: RX=0, TX=16384)
[W5500] socket_accept: client IP: 172.16.1.2, port: 32790
[W5500] socket_accept: success (single-client mode)
[MAIN] accept() returned, addr=('172.16.1.2', 32790)
[W5500] socket_recv: socket=0, len=16384, timeout=-1
[W5500] socket_recv: checking, RX_RSR=330
[W5500] socket_recv: data available (330 bytes), calling recv
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 330
[W5500] socket_recv: success, received 330 bytes
[MAIN] recv() returned 330 bytes
[MAIN] Request decoded, length=330
[MAIN] Request split into 11 lines
[MAIN] First line: GET / HTTP/1.1...
[MAIN] Split into 3 parts
[MAIN] Parsed: method=GET, path=/
[MAIN] Request path: /
[MAIN] Calling generate_large_html_response()
[MAIN] HTML generated, size=6466
Full response size: 6629 bytes (headers: 163, body: 6466)
Response generation time: 1ms
[MAIN] cl.send() is about to be executed
[W5500] socket_send: socket=0, len=6629
[W5500] socket_send: using DMA transfer for 6629 bytes
[W5500] socket_send: queued transfer, initial chunk size=6629
[W5500] w5500_send_chunk: sn=0, len=6629, tx_wr_ptr=133, offset=133, space_to_end=16251
[W5500] w5500_dma_write_to_txbuf: wrote 6629 bytes to socket 0 offset 133
[W5500] w5500_tx_service: started DMA for 6629 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 6629/6629 bytes sent on socket 0
[W5500] socket_send: transfer complete, sent 6629 bytes
[MAIN] Calling cl.close()
[W5500] socket_close: socket 0
[W5500] socket_close: cancelling active transfer on socket 0
[W5500] socket_close: calling WIZCHIP_EXPORT(close) on socket 0
[W5500] socket_close: before - state=0x17 (ESTABLISHED)
[W5500] socket_close: saved state - port=80, type=0x01, was_listening=1
[W5500] socket_close: after - state=0x00 (CLOSED)
[W5500] socket_close: re-listening on port 80
[W5500] socket_close: WIZCHIP_EXPORT(socket) returned 0
[W5500] socket_close: after reopen - RX_RSR=0, TX_FSR=16384/16384
[W5500] socket_close: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_close: successfully re-listened
[MAIN] close() returned successfully
[MAIN] ====== WAITING FOR CONNECTION ======
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial state - socket 0: 0x14 (LISTEN)
[W5500] socket_accept: socket ESTABLISHED!
[W5500] socket_accept: RX_RSR=338, TX_FSR=16384 (expected: RX=0, TX=16384)
[W5500] socket_accept: client IP: 172.16.1.2, port

# Wireshark log

First request packets 5-23, second request (/svg) packet 24-31. 
I believe unrelated packets: 1,2,3,4,26,27


No.     Time           Source                Destination           Protocol Length Info
      1 0.000000000    fe80::56ee:75ff:fead:cdcc ff02::2               ICMPv6   70     Router Solicitation from 54:ee:75:ad:cd:cc

Frame 1: Packet, 70 bytes on wire (560 bits), 70 bytes captured (560 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: IPv6mcast_02 (33:33:00:00:00:02)
Internet Protocol Version 6, Src: fe80::56ee:75ff:fead:cdcc, Dst: ff02::2
Internet Control Message Protocol v6

No.     Time           Source                Destination           Protocol Length Info
      2 0.194004004    0.0.0.0               255.255.255.255       DHCP     345    DHCP Discover - Transaction ID 0x4816901b

Frame 2: Packet, 345 bytes on wire (2760 bits), 345 bytes captured (2760 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: Broadcast (ff:ff:ff:ff:ff:ff)
Internet Protocol Version 4, Src: 0.0.0.0, Dst: 255.255.255.255
User Datagram Protocol, Src Port: 68, Dst Port: 67
Dynamic Host Configuration Protocol (Discover)

No.     Time           Source                Destination           Protocol Length Info
      3 25.470258443   WistronInfoC_ad:cd:cc Broadcast             ARP      42     Who has 172.16.1.1? Tell 172.16.1.2

Frame 3: Packet, 42 bytes on wire (336 bits), 42 bytes captured (336 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: Broadcast (ff:ff:ff:ff:ff:ff)
Address Resolution Protocol (request)

No.     Time           Source                Destination           Protocol Length Info
      4 25.470564245   MS-NLB-PhysServer-13_bf:ee:95:ea WistronInfoC_ad:cd:cc ARP      60     172.16.1.1 is at 02:0d:bf:ee:95:ea

Frame 4: Packet, 60 bytes on wire (480 bits), 60 bytes captured (480 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Address Resolution Protocol (reply)

No.     Time           Source                Destination           Protocol Length Info
      5 25.470584585   172.16.1.2            172.16.1.1            TCP      74     32790 → 80 [SYN] Seq=0 Win=64240 Len=0 MSS=1460 SACK_PERM TSval=2611325003 TSecr=0 WS=1024

Frame 5: Packet, 74 bytes on wire (592 bits), 74 bytes captured (592 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 32790, Dst Port: 80, Seq: 0, Len: 0

No.     Time           Source                Destination           Protocol Length Info
      6 25.470821293   172.16.1.1            172.16.1.2            TCP      60     80 → 32790 [SYN, ACK] Seq=0 Ack=1 Win=16384 Len=0 MSS=1460

Frame 6: Packet, 60 bytes on wire (480 bits), 60 bytes captured (480 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 32790, Seq: 0, Ack: 1, Len: 0

No.     Time           Source                Destination           Protocol Length Info
      7 25.470901323   172.16.1.2            172.16.1.1            TCP      54     32790 → 80 [ACK] Seq=1 Ack=1 Win=64240 Len=0

Frame 7: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 32790, Dst Port: 80, Seq: 1, Ack: 1, Len: 0

No.     Time           Source                Destination           Protocol Length Info
      8 25.471274059   172.16.1.2            172.16.1.1            HTTP     384    GET / HTTP/1.1 

Frame 8: Packet, 384 bytes on wire (3072 bits), 384 bytes captured (3072 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 32790, Dst Port: 80, Seq: 1, Ack: 1, Len: 330
Hypertext Transfer Protocol

No.     Time           Source                Destination           Protocol Length Info
      9 25.557617536   172.16.1.1            172.16.1.2            TCP      1514   80 → 32790 [PSH, ACK] Seq=1 Ack=331 Win=16384 Len=1460 [TCP PDU reassembled in 17]

Frame 9: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 32790, Seq: 1, Ack: 331, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     10 25.557689888   172.16.1.2            172.16.1.1            TCP      54     32790 → 80 [ACK] Seq=331 Ack=1461 Win=65535 Len=0

Frame 10: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 32790, Dst Port: 80, Seq: 331, Ack: 1461, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     11 25.557878137   172.16.1.1            172.16.1.2            TCP      1514   80 → 32790 [PSH, ACK] Seq=1461 Ack=331 Win=16384 Len=1460 [TCP PDU reassembled in 17]

Frame 11: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 32790, Seq: 1461, Ack: 331, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     12 25.557903867   172.16.1.2            172.16.1.1            TCP      54     32790 → 80 [ACK] Seq=331 Ack=2921 Win=65535 Len=0

Frame 12: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 32790, Dst Port: 80, Seq: 331, Ack: 2921, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     13 25.557878459   172.16.1.1            172.16.1.2            TCP      1514   80 → 32790 [PSH, ACK] Seq=2921 Ack=331 Win=16384 Len=1460 [TCP PDU reassembled in 17]

Frame 13: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 32790, Seq: 2921, Ack: 331, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     14 25.557924017   172.16.1.2            172.16.1.1            TCP      54     32790 → 80 [ACK] Seq=331 Ack=4381 Win=65535 Len=0

Frame 14: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 32790, Dst Port: 80, Seq: 331, Ack: 4381, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     15 25.558138668   172.16.1.1            172.16.1.2            TCP      1514   80 → 32790 [PSH, ACK] Seq=4381 Ack=331 Win=16384 Len=1460 [TCP PDU reassembled in 17]

Frame 15: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 32790, Seq: 4381, Ack: 331, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     16 25.558161713   172.16.1.2            172.16.1.1            TCP      54     32790 → 80 [ACK] Seq=331 Ack=5841 Win=65535 Len=0

Frame 16: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 32790, Dst Port: 80, Seq: 331, Ack: 5841, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     17 25.558138957   172.16.1.1            172.16.1.2            HTTP     843    HTTP/1.1 200 OK  (text/html)

Frame 17: Packet, 843 bytes on wire (6744 bits), 843 bytes captured (6744 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 32790, Seq: 5841, Ack: 331, Len: 789
[5 Reassembled TCP Segments (6629 bytes): #9(1460), #11(1460), #13(1460), #15(1460), #17(789)]
Hypertext Transfer Protocol
Line-based text data: text/html (164 lines)

No.     Time           Source                Destination           Protocol Length Info
     18 25.558181585   172.16.1.2            172.16.1.1            TCP      54     32790 → 80 [ACK] Seq=331 Ack=6630 Win=65535 Len=0

Frame 18: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 32790, Dst Port: 80, Seq: 331, Ack: 6630, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     19 25.561239600   172.16.1.2            172.16.1.1            TCP      54     32790 → 80 [FIN, ACK] Seq=331 Ack=6630 Win=65535 Len=0

Frame 19: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 32790, Dst Port: 80, Seq: 331, Ack: 6630, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     20 25.561552534   172.16.1.1            172.16.1.2            TCP      60     80 → 32790 [ACK] Seq=6630 Ack=332 Win=16384 Len=0

Frame 20: Packet, 60 bytes on wire (480 bits), 60 bytes captured (480 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 32790, Seq: 6630, Ack: 332, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     21 27.705887366   172.16.1.2            172.16.1.1            TCP      74     32798 → 80 [SYN] Seq=0 Win=64240 Len=0 MSS=1460 SACK_PERM TSval=2611327239 TSecr=0 WS=1024

Frame 21: Packet, 74 bytes on wire (592 bits), 74 bytes captured (592 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 32798, Dst Port: 80, Seq: 0, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     22 27.706203312   172.16.1.1            172.16.1.2            TCP      60     80 → 32798 [SYN, ACK] Seq=0 Ack=1 Win=16384 Len=0 MSS=1460

Frame 22: Packet, 60 bytes on wire (480 bits), 60 bytes captured (480 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 32798, Seq: 0, Ack: 1, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     23 27.706278714   172.16.1.2            172.16.1.1            TCP      54     32798 → 80 [ACK] Seq=1 Ack=1 Win=64240 Len=0

Frame 23: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 32798, Dst Port: 80, Seq: 1, Ack: 1, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     24 27.708096245   172.16.1.2            172.16.1.1            HTTP     392    GET /svg HTTP/1.1 

Frame 24: Packet, 392 bytes on wire (3136 bits), 392 bytes captured (3136 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 32798, Dst Port: 80, Seq: 1, Ack: 1, Len: 338
Hypertext Transfer Protocol

No.     Time           Source                Destination           Protocol Length Info
     25 27.910523251   172.16.1.1            172.16.1.2            TCP      60     80 → 32798 [PSH, ACK] Seq=1 Ack=339 Win=16046 Len=0

Frame 25: Packet, 60 bytes on wire (480 bits), 60 bytes captured (480 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 32798, Seq: 1, Ack: 339, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     26 31.496740147   0.0.0.0               255.255.255.255       DHCP     345    DHCP Discover - Transaction ID 0x4816901b

Frame 26: Packet, 345 bytes on wire (2760 bits), 345 bytes captured (2760 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: Broadcast (ff:ff:ff:ff:ff:ff)
Internet Protocol Version 4, Src: 0.0.0.0, Dst: 255.255.255.255
User Datagram Protocol, Src Port: 68, Dst Port: 67
Dynamic Host Configuration Protocol (Discover)

No.     Time           Source                Destination           Protocol Length Info
     27 35.543078333   fe80::56ee:75ff:fead:cdcc ff02::2               ICMPv6   70     Router Solicitation from 54:ee:75:ad:cd:cc

Frame 27: Packet, 70 bytes on wire (560 bits), 70 bytes captured (560 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: IPv6mcast_02 (33:33:00:00:00:02)
Internet Protocol Version 6, Src: fe80::56ee:75ff:fead:cdcc, Dst: ff02::2
Internet Control Message Protocol v6

No.     Time           Source                Destination           Protocol Length Info
     28 38.182457626   172.16.1.2            172.16.1.1            TCP      54     [TCP Keep-Alive] 32798 → 80 [ACK] Seq=338 Ack=1 Win=64240 Len=0

Frame 28: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 32798, Dst Port: 80, Seq: 338, Ack: 1, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     29 38.182765687   172.16.1.1            172.16.1.2            TCP      60     [TCP Keep-Alive ACK] 80 → 32798 [ACK] Seq=1 Ack=339 Win=16046 Len=0

Frame 29: Packet, 60 bytes on wire (480 bits), 60 bytes captured (480 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 32798, Seq: 1, Ack: 339, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     30 48.421511415   172.16.1.2            172.16.1.1            TCP      54     [TCP Keep-Alive] 32798 → 80 [ACK] Seq=338 Ack=1 Win=64240 Len=0

Frame 30: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 32798, Dst Port: 80, Seq: 338, Ack: 1, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     31 48.421858815   172.16.1.1            172.16.1.2            TCP      60     [TCP Keep-Alive ACK] 80 → 32798 [ACK] Seq=1 Ack=339 Win=16046 Len=0

Frame 31: Packet, 60 bytes on wire (480 bits), 60 bytes captured (480 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 32798, Seq: 1, Ack: 339, Len: 0

---

FILELIST:
main_test04_svg_50kb.py
micropython/extmod/network_wiznet5k.c

