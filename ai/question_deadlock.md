# Action Plan: W5500 Camera Streaming Deadlock Fix

## Application description


It is a webpage serving application, where it serves a very big (~53kB) .svg file 
at the /svg endpoint. It is clearly bigger then the TX buffer (16kB).

Give extra attention to DMA handling. The w5500 should use DMA. 


## Problem Summary

First 172.16.1.1/svg is successful, I receive the whole .svg file nothing truncated.
The second call (loading again) fails.
This is the last message on the micropython console:
[W5500] socket_accept: socket ESTABLISHED!
(see the full console output)

The MCU hard locks with no timeout messages or error output.

Study the problem from at least 9 different angles. Study the sourcecode thoroughly.

I also attached a wireshark network packet dump. See that chapter also.

# Architecture

| Component | Details |
|-----------|---------|
| MCU | RP2350A (Raspberry Pi Pico 2) |
| Ethernet | W5500 via SPI0 @ 20MHz |
| Camera | OV5640 via PIO + DMA |
| Frame size | 240x320 RGB565 = 153.6KB |
| TX buffer | 16KB (Socket 0) |
| RX buffer | 16KB (Socket 0) |


# Micropython console output with some explanations between different runs

Note: when "device disconnected" appears, it is the result of me physically 
unplugging the usb cable, and mpremote recognize the device disconnected (usb device disappears).

This log corresponds with two 172.16.1.1/svg calls. The first one successful, the second one freezes
the device complete. I mark in the console when I start the second request.
### this is my comment. (I will put this inside the output.)

mpremote
Connected to MicroPython at /dev/ttyACM0
Use Ctrl-] or Ctrl-x to exit this shell
[W5500] w5500_dma_init: claimed TX channel 0
[W5500] wiznet5k_active: calling wiznet5k_init()
[W5500] wiznet5k_init: initializing with provided TCP stack
[W5500] wiznet5k_init: calling ctlwizchip(CW_INIT_WIZCHIP, ...)
[W5500] wiznet5k_init: calling ctlnetwork(CN_SET_NETINFO, ...)
[W5500] wiznet5k_init: registering with network module
[W5500] w5500_tx_pool_init: initialized transfer pool
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
[MAIN] ====== WAITING FOR CONNECTION ======
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial state - socket 0: 0x14 (LISTEN)
[W5500] socket_accept: loop 100 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 100)
### Device booted up, and actively waits for the first request
[W5500] socket_accept: loop 200 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 200)
[W5500] socket_accept: socket ESTABLISHED!
[W5500] socket_accept: client IP: 172.16.1.2, port: 34338
[W5500] socket_accept: success (single-client mode)
[MAIN] accept() returned, addr=('172.16.1.2', 34338)
[W5500] socket_recv: socket=0, len=16384, timeout=-1
[W5500] socket_recv: checking, RX_RSR=333
[W5500] socket_recv: data available (333 bytes), calling recv
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 333
[W5500] socket_recv: success, received 333 bytes
[MAIN] recv() returned 333 bytes
[MAIN] Request decoded, length=333
[MAIN] Request split into 11 lines
[MAIN] First line: GET /svg HTTP/1.1...
[MAIN] Split into 3 parts
[MAIN] Parsed: method=GET, path=/svg
[MAIN] Request path: /svg
[MAIN] Calling generate_svg_response()
[MAIN] SVG generated, size=52314
Full response size: 52467 bytes (headers: 153, body: 52314)
Response generation time: 0ms
[MAIN] cl.send() is about to be executed
[W5500] socket_send: socket=0, len=52467
[W5500] socket_send: using DMA transfer for 52467 bytes
[W5500] socket_send: queued transfer, initial chunk size=16384
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=42245, offset=9477, space_to_end=6907
[W5500] w5500_dma_write_to_txbuf: wrote 6907 bytes to socket 0 offset 9477
[W5500] w5500_dma_write_to_txbuf: wrote 9477 bytes to socket 0 offset 0
[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 16384/52467 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=58629, offset=9477, space_to_end=6907
[W5500] w5500_dma_write_to_txbuf: wrote 6907 bytes to socket 0 offset 9477
[W5500] w5500_dma_write_to_txbuf: wrote 9477 bytes to socket 0 offset 0
[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 32768/52467 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=9477, offset=9477, space_to_end=6907
[W5500] w5500_dma_write_to_txbuf: wrote 6907 bytes to socket 0 offset 9477
[W5500] w5500_dma_write_to_txbuf: wrote 9477 bytes to socket 0 offset 0
[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 49152/52467 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=3315, tx_wr_ptr=25861, offset=9477, space_to_end=6907
[W5500] w5500_dma_write_to_txbuf: wrote 3315 bytes to socket 0 offset 9477
[W5500] w5500_tx_service: started DMA for 3315 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 52467/52467 bytes sent on socket 0
[W5500] socket_send: transfer complete, sent 52467 bytes
[MAIN] Calling cl.close()
[W5500] socket_close: socket 0
[W5500] socket_close: cancelling active transfer on socket 0
[W5500] socket_close: calling WIZCHIP_EXPORT(close) on socket 0
[W5500] socket_close: before - state=0x1c (CLOSE_WAIT)
[W5500] socket_close: saved state - port=80, type=0x01, was_listening=1
[W5500] socket_close: after - state=0x00 (CLOSED)
[W5500] socket_close: re-listening on port 80
[W5500] socket_close: WIZCHIP_EXPORT(socket) returned 0
[W5500] socket_close: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_close: successfully re-listened
[MAIN] close() returned successfully
[MAIN] ====== WAITING FOR CONNECTION ======
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial state - socket 0: 0x14 (LISTEN)
[W5500] socket_accept: loop 100 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 100)
### First /svg request successfully generated, sent to the client, and here we are already waiting for the next to arrive.
[W5500] socket_accept: loop 200 - socket 0 IR: 0x00, SR: 0x14 (LISTEN)
[W5500] socket_accept: still waiting... (loop 200)
[W5500] socket_accept: socket ESTABLISHED!
[W5500] socket_accept: client IP: 172.16.1.2, port: 39410
[W5500] socket_accept: success (single-client mode)
[MAIN] accept(device disconnected
### The second /svg request arrives, but freezes the device. After some minutes, I disconnect physically, 
### the "device disconnected" error message is the result of me being physically pulling off the usb cable.



# Wireshark network packet

Packet number 5 - 80 is the whole log of the ethernet interface.
The first request is 5-75, the second request (where the device freezes) are packets 76-80.


No.     Time           Source                Destination           Protocol Length Info
      5 8.874375616    172.16.1.2            172.16.1.1            TCP      74     35664 → 80 [SYN] Seq=0 Win=64240 Len=0 MSS=1460 SACK_PERM TSval=2547448318 TSecr=0 WS=1024

Frame 5: Packet, 74 bytes on wire (592 bits), 74 bytes captured (592 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 0, Len: 0

No.     Time           Source                Destination           Protocol Length Info
      6 8.874626626    172.16.1.1            172.16.1.2            TCP      60     80 → 35664 [SYN, ACK] Seq=0 Ack=1 Win=16384 Len=0 MSS=1460

Frame 6: Packet, 60 bytes on wire (480 bits), 60 bytes captured (480 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 0, Ack: 1, Len: 0

No.     Time           Source                Destination           Protocol Length Info
      7 8.874780425    172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=1 Ack=1 Win=64240 Len=0

Frame 7: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 1, Ack: 1, Len: 0

No.     Time           Source                Destination           Protocol Length Info
      8 8.875363494    172.16.1.2            172.16.1.1            HTTP     387    GET /svg HTTP/1.1 

Frame 8: Packet, 387 bytes on wire (3096 bits), 387 bytes captured (3096 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 1, Ack: 1, Len: 333
Hypertext Transfer Protocol

No.     Time           Source                Destination           Protocol Length Info
      9 9.077772547    172.16.1.1            172.16.1.2            TCP      60     80 → 35664 [PSH, ACK] Seq=1 Ack=334 Win=16384 Len=0

Frame 9: Packet, 60 bytes on wire (480 bits), 60 bytes captured (480 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 1, Ack: 334, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     10 10.754369705   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=1 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 10: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 1, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     11 10.754452529   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=1461 Win=65535 Len=0

Frame 11: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 1461, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     12 10.754659067   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=1461 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 12: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 1461, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     13 10.754696464   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=2921 Win=65535 Len=0

Frame 13: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 2921, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     14 10.754659632   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=2921 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 14: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 2921, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     15 10.754724135   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=4381 Win=65535 Len=0

Frame 15: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 4381, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     16 10.754891321   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=4381 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 16: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 4381, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     17 10.754922570   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=5841 Win=65535 Len=0

Frame 17: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 5841, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     18 10.754891601   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=5841 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 18: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 5841, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     19 10.754948631   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=7301 Win=65535 Len=0

Frame 19: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 7301, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     20 10.755150786   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=7301 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 20: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 7301, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     21 10.755175739   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=8761 Win=65535 Len=0

Frame 21: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 8761, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     22 10.755151066   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=8761 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 22: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 8761, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     23 10.755199920   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=10221 Win=65535 Len=0

Frame 23: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 10221, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     24 10.755414531   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=10221 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 24: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 10221, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     25 10.755457355   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=11681 Win=65535 Len=0

Frame 25: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 11681, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     26 10.755414836   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=11681 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 26: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 11681, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     27 10.755482442   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=13141 Win=65535 Len=0

Frame 27: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 13141, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     28 10.755678650   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=13141 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 28: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 13141, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     29 10.755717612   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=14601 Win=65535 Len=0

Frame 29: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 14601, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     30 10.755680638   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=14601 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 30: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 14601, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     31 10.755745654   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=16061 Win=65535 Len=0

Frame 31: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 16061, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     32 10.755680913   172.16.1.1            172.16.1.2            TCP      378    80 → 35664 [PSH, ACK] Seq=16061 Ack=334 Win=16384 Len=324 [TCP PDU reassembled in 72]

Frame 32: Packet, 378 bytes on wire (3024 bits), 378 bytes captured (3024 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 16061, Ack: 334, Len: 324

No.     Time           Source                Destination           Protocol Length Info
     33 10.755767991   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=16385 Win=65535 Len=0

Frame 33: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 16385, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     34 10.771328490   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=16385 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 34: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 16385, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     35 10.771445360   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=17845 Win=65535 Len=0

Frame 35: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 17845, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     36 10.771599280   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=17845 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 36: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 17845, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     37 10.771676154   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=19305 Win=65535 Len=0

Frame 37: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 19305, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     38 10.771599854   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=19305 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 38: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 19305, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     39 10.771707053   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=20765 Win=65535 Len=0

Frame 39: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 20765, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     40 10.771874494   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=20765 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 40: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 20765, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     41 10.771911185   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=22225 Win=65535 Len=0

Frame 41: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 22225, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     42 10.771874955   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=22225 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 42: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 22225, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     43 10.771938764   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=23685 Win=65535 Len=0

Frame 43: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 23685, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     44 10.772134482   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=23685 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 44: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 23685, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     45 10.772186950   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=25145 Win=65535 Len=0

Frame 45: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 25145, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     46 10.772134910   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=25145 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 46: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 25145, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     47 10.772216846   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=26605 Win=65535 Len=0

Frame 47: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 26605, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     48 10.772394679   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=26605 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 48: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 26605, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     49 10.772429772   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=28065 Win=65535 Len=0

Frame 49: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 28065, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     50 10.772394982   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=28065 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 50: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 28065, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     51 10.772457528   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=29525 Win=65535 Len=0

Frame 51: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 29525, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     52 10.772692770   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=29525 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 52: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 29525, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     53 10.772730621   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=30985 Win=65535 Len=0

Frame 53: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 30985, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     54 10.772693069   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=30985 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 54: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 30985, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     55 10.772693264   172.16.1.1            172.16.1.2            TCP      378    80 → 35664 [PSH, ACK] Seq=32445 Ack=334 Win=16384 Len=324 [TCP PDU reassembled in 72]

Frame 55: Packet, 378 bytes on wire (3024 bits), 378 bytes captured (3024 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 32445, Ack: 334, Len: 324

No.     Time           Source                Destination           Protocol Length Info
     56 10.772887646   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=32769 Win=65535 Len=0

Frame 56: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 32769, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     57 10.785420147   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=32769 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 57: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 32769, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     58 10.785680389   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=34229 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 58: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 34229, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     59 10.785680969   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=35689 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 59: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 35689, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     60 10.785945923   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=37149 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 60: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 37149, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     61 10.785946388   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=38609 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 61: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 38609, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     62 10.786205501   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=40069 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 62: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 40069, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     63 10.786205876   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=41529 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 63: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 41529, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     64 10.786497076   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=42989 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 64: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 42989, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     65 10.786497558   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=44449 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 65: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 44449, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     66 10.786672003   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=45909 Win=65535 Len=0

Frame 66: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 45909, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     67 10.794308874   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=45909 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 67: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 45909, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     68 10.794575353   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=47369 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 68: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 47369, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     69 10.794663493   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=48829 Win=65535 Len=0

Frame 69: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 48829, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     70 10.794575838   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=48829 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 70: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 48829, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     71 10.794839799   172.16.1.1            172.16.1.2            TCP      1514   80 → 35664 [PSH, ACK] Seq=50289 Ack=334 Win=16384 Len=1460 [TCP PDU reassembled in 72]

Frame 71: Packet, 1514 bytes on wire (12112 bits), 1514 bytes captured (12112 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 50289, Ack: 334, Len: 1460

No.     Time           Source                Destination           Protocol Length Info
     72 10.794840159   172.16.1.1            172.16.1.2            HTTP/XML 773    HTTP/1.1 200 OK 

Frame 72: Packet, 773 bytes on wire (6184 bits), 773 bytes captured (6184 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 51749, Ack: 334, Len: 719
[38 Reassembled TCP Segments (52467 bytes): #10(1460), #12(1460), #14(1460), #16(1460), #18(1460), #20(1460), #22(1460), #24(1460), #26(1460), #28(1460), #30(1460), #32(324), #34(1460), #36(1460), #38(1460), #40(1460), #42(1460), #44(1460)]
Hypertext Transfer Protocol
eXtensible Markup Language

No.     Time           Source                Destination           Protocol Length Info
     73 10.794982410   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [ACK] Seq=334 Ack=52468 Win=65535 Len=0

Frame 73: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 52468, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     74 10.796320619   172.16.1.2            172.16.1.1            TCP      54     35664 → 80 [FIN, ACK] Seq=334 Ack=52468 Win=65535 Len=0

Frame 74: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 35664, Dst Port: 80, Seq: 334, Ack: 52468, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     75 10.796668591   172.16.1.1            172.16.1.2            TCP      60     80 → 35664 [ACK] Seq=52468 Ack=335 Win=16384 Len=0

Frame 75: Packet, 60 bytes on wire (480 bits), 60 bytes captured (480 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 35664, Seq: 52468, Ack: 335, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     76 29.042707484   172.16.1.2            172.16.1.1            TCP      74     40040 → 80 [SYN] Seq=0 Win=64240 Len=0 MSS=1460 SACK_PERM TSval=2547468486 TSecr=0 WS=1024

Frame 76: Packet, 74 bytes on wire (592 bits), 74 bytes captured (592 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 40040, Dst Port: 80, Seq: 0, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     77 29.043041961   172.16.1.1            172.16.1.2            TCP      60     80 → 40040 [SYN, ACK] Seq=0 Ack=1 Win=16384 Len=0 MSS=1460

Frame 77: Packet, 60 bytes on wire (480 bits), 60 bytes captured (480 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 40040, Seq: 0, Ack: 1, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     78 29.043147400   172.16.1.2            172.16.1.1            TCP      54     40040 → 80 [ACK] Seq=1 Ack=1 Win=64240 Len=0

Frame 78: Packet, 54 bytes on wire (432 bits), 54 bytes captured (432 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 40040, Dst Port: 80, Seq: 1, Ack: 1, Len: 0

No.     Time           Source                Destination           Protocol Length Info
     79 29.043995886   172.16.1.2            172.16.1.1            HTTP     387    GET /svg HTTP/1.1 

Frame 79: Packet, 387 bytes on wire (3096 bits), 387 bytes captured (3096 bits) on interface enp0s31f6, id 0
Ethernet II, Src: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc), Dst: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea)
Internet Protocol Version 4, Src: 172.16.1.2, Dst: 172.16.1.1
Transmission Control Protocol, Src Port: 40040, Dst Port: 80, Seq: 1, Ack: 1, Len: 333
Hypertext Transfer Protocol

No.     Time           Source                Destination           Protocol Length Info
     80 29.246401825   172.16.1.1            172.16.1.2            TCP      60     80 → 40040 [PSH, ACK] Seq=1 Ack=334 Win=16051 Len=0

Frame 80: Packet, 60 bytes on wire (480 bits), 60 bytes captured (480 bits) on interface enp0s31f6, id 0
Ethernet II, Src: MS-NLB-PhysServer-13_bf:ee:95:ea (02:0d:bf:ee:95:ea), Dst: WistronInfoC_ad:cd:cc (54:ee:75:ad:cd:cc)
Internet Protocol Version 4, Src: 172.16.1.1, Dst: 172.16.1.2
Transmission Control Protocol, Src Port: 80, Dst Port: 40040, Seq: 1, Ack: 334, Len: 0

---

FILELIST:
main_test04_svg_50kb.py
modules/camera/cam.c
modules/camera/cam.h
modules/camera/micropython.cmake
micropython/extmod/network_wiznet5k.c
micropython/extmod/machine_spi.c
micropython/extmod/modnetwork.c
micropython/extmod/modnetwork.h
micropython/extmod/modsocket.c

