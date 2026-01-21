# Description

Take the main_test04_svg_50kb.py, it is the micropython starting application, 
study it thoroughly.
Take the `micropython/extmod/network_wiznet5k.c`, study it from at least 3 different angles. 

Please be  aware, the w5500 is a heavily modified driver, it does not use
lwIP, rather hardware tcp/ip. And w5500 uses a single 16kB RX 16kB TX on
socket  0, it is a single connection client-server architecture (the mcu,
ie. the  w550 is the server, and the client is a webbrowser).

Do not take any files from the internet. Everything is here included.

# The problem

Error 32. Where it comes, why it happens, how to prevent it.

# Important consideration for log reading

Please recognize, that this line:
[W5500] socket_accept: loop 100 SR=0x14 (LISTEN) IR=0x00

Means the mcu is waiting for connection, and prints out only each 100th, 
Sn_SR socket state register spi query. (listen is 0x14)

# Micropython log



Connected to MicroPython at /dev/ttyACM0
Use Ctrl-] or Ctrl-x to exit this shell
ERROR: Failed to load config.json: [Errno 2] ENOENT
[W5500] wiznet5k_active: n_args=2, IS_ACTIVE=0
[W5500] wiznet5k_active: activating...
[W5500] wiznet5k_active: calling wiznet5k_init()
[W5500] wiznet5k_init: initializing with provided TCP stack
[W5500] wiznet5k_init: calling ctlwizchip(CW_INIT_WIZCHIP, ...)
[W5500] wiznet5k_init: calling ctlnetwork(CN_SET_NETINFO, ...)
[W5500] wiznet5k_init: registering with network module
[W5500] w5500_tx_pool_init: initialized transfer pool
[W5500] wiznet5k_init: done, active=true
[W5500] wiznet5k_active: n_args=1, IS_ACTIVE=1
[W5500] wiznet5k_active: getting state
initialize CAMERA
set camera XCLK (pwm) pin: 11
call set_pwm_freq_kHz(pwm) before init_cam() to change it
set camera I2C pins: SDA: 22, SCL: 23
call set_i2c_pins(sda,scl) before init_cam() to change it
start_cam finished, camera started
setup_dma_for_capture()->DMA_CH= 0
irq_add_shared_handler: cam_handler
[W5500] socket_socket: domain=2, type=1, proto=0
[W5500] socket_socket: TCP socket
[W5500] socket_socket: assigned socket number 0
[W5500] socket_socket: success, fileno=0
[W5500] socket_bind: socket=0, port=8081, type=0x01
[W5500] socket_bind: WIZCHIP_EXPORT(socket) returned 0
[W5500] socket_bind: socket 0 state after open: 0x13 (INIT)
[W5500] socket_bind: success
[W5500] socket_listen: socket=0, backlog=5
[W5500] socket_listen: before - socket 0 state: 0x13 (INIT)
[W5500] socket_listen: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_listen: after - socket 0 state: 0x14 (LISTEN)
[W5500] socket_listen: saved state - port=8081, type=0x01
[W5500] socket_listen: success
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial SR=0x14 (LISTEN)
[W5500] socket_accept: success
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=305 IR=0x04
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=305
[W5500] socket_recv: data available (305 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=10030a65, sn=0, buf=2003d150, len=8192

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before WIZCHIP_EXPORT(recv) - Second Request
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        8
   Active Count:     0
========================================================

[W5500] before MP_THREAD_GIL_ENTER
[W5500] after MP_THREAD_GIL_ENTER
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 305
[W5500] socket_recv: success, received 305 bytes
[W5500] socket_send (DMA): socket=0, len=132

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        8
   Active Count:     0
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2003D281
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        8
   Active Count:     0
========================================================

[W5500] socket_send: using DMA transfer for 132 bytes
[W5500] socket_send: queued transfer, initial chunk size=132
[W5500] w5500_send_chunk: sn=0, len=132, tx_wr_ptr=7096, offset=7096, space_to_end=9288

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2003D281
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=0/132
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 132 bytes to socket 0 offset 7096

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=0/132
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 132 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 132/132 bytes sent on socket 0
[W5500] w5500_tx_service: transfer complete, dequeuing socket 0
[W5500] socket_send: transfer complete, sent 132 bytes
[W5500] socket_settimeout: socket=0, timeout_ms=10000
[W5500] socket_settimeout: set to 10000 ms
[W5500] socket_send (DMA): socket=0, len=153600

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        8
   Active Count:     0
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2002CAC4
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        8
   Active Count:     0
========================================================

[W5500] socket_send: using DMA transfer for 153600 bytes
[W5500] socket_send: queued transfer, initial chunk size=16384
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=7228, offset=7228, space_to_end=9156

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2002CAC4
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=0/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 9156 bytes to socket 0 offset 7228

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=0/153600
   Active Count:     1
========================================================


========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=0/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x20005211
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=0/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 7228 bytes to socket 0 offset 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=0/153600
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 16384/153600 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=23612, offset=7228, space_to_end=9156

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=16384/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x20006E4D
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=16384/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 9156 bytes to socket 0 offset 7228

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=16384/153600
   Active Count:     1
========================================================


========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=16384/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x20009211
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=16384/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 7228 bytes to socket 0 offset 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=16384/153600
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 32768/153600 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=39996, offset=7228, space_to_end=9156

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=32768/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2000AE4D
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=32768/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 9156 bytes to socket 0 offset 7228

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=32768/153600
   Active Count:     1
========================================================


========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=32768/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2000D211
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=32768/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 7228 bytes to socket 0 offset 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=32768/153600
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 49152/153600 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=56380, offset=7228, space_to_end=9156

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=49152/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2000EE4D
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=49152/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 9156 bytes to socket 0 offset 7228

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=49152/153600
   Active Count:     1
========================================================


========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=49152/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x20011211
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=49152/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 7228 bytes to socket 0 offset 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=49152/153600
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 65536/153600 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=7228, offset=7228, space_to_end=9156

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=65536/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x20012E4D
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=65536/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 9156 bytes to socket 0 offset 7228

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=65536/153600
   Active Count:     1
========================================================


========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=65536/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x20015211
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=65536/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 7228 bytes to socket 0 offset 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=65536/153600
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 81920/153600 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=13140, tx_wr_ptr=23612, offset=7228, space_to_end=9156

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=81920/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x20016E4D
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=81920/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 9156 bytes to socket 0 offset 7228

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=81920/153600
   Active Count:     1
========================================================


========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=81920/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x20019211
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=81920/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 3984 bytes to socket 0 offset 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=81920/153600
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 13140 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 95060/153600 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=36752, offset=3984, space_to_end=12400

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=95060/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2001A1A1
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=95060/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 12400 bytes to socket 0 offset 3984

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=95060/153600
   Active Count:     1
========================================================


========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=95060/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2001D211
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=95060/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 3984 bytes to socket 0 offset 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=95060/153600
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 111444/153600 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=53136, offset=3984, space_to_end=12400

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=111444/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2001E1A1
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=111444/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 12400 bytes to socket 0 offset 3984

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=111444/153600
   Active Count:     1
========================================================


========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=111444/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x20021211
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=111444/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 3984 bytes to socket 0 offset 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=111444/153600
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 127828/153600 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=3984, offset=3984, space_to_end=12400

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=127828/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x200221A1
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=127828/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 12400 bytes to socket 0 offset 3984

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=127828/153600
   Active Count:     1
========================================================


========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=127828/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x20025211
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=127828/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 3984 bytes to socket 0 offset 0

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=127828/153600
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 144212/153600 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=9388, tx_wr_ptr=20368, offset=3984, space_to_end=12400

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=144212/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x200261A1
   WRITE_ADDR:       0x40080008
   TRANS_COUNT:      0
   CTRL_TRIG:        0x00302011
   AL1_CTRL:         0x00302011
   BUSY:             NO
   ENABLED:          YES
   CHAIN_TO:         4
   RING:             NO
   INCR_READ:        YES
   INCR_WRITE:       NO
   DATA_SIZE:        0
   DREQ:             32
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=144212/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 9388 bytes to socket 0 offset 3984

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Write Completed
--------------------------------------------------------
 TX DMA Channel: NOT CLAIMED (dma_tx_chan = -1)
--------------------------------------------------------
 SPI DMACR:        0x00000003
   TX DMA Enable:    YES
   RX DMA Enable:    YES
--------------------------------------------------------
 SPI State:
   Busy:             NO
   Readable:         NO
   Writable:         YES
--------------------------------------------------------
 Transfer Pool:
   Available:        7
   Active[1]: socket=0 state=1 bytes=144212/153600
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 9388 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 153600/153600 bytes sent on socket 0
[W5500] w5500_tx_service: transfer complete, dequeuing socket 0
[W5500] socket_send: transfer complete, sent 153600 bytes
[W5500] socket_close: socket 0
[W5500] socket_close: calling WIZCHIP_EXPORT(close) on socket 0
[W5500] socket_close: before - state=0x1c (CLOSE_WAIT)
[W5500] socket_close: after - state=0x00 (CLOSED)
[W5500] socket_close: done, Python should handle re-listen
[W5500] socket_close: socket 0
[W5500] socket_close: calling WIZCHIP_EXPORT(close) on socket 0
[W5500] socket_close: before - state=0x00 (CLOSED)
[W5500] socket_close: after - state=0x00 (CLOSED)
[W5500] socket_close: done, Python should handle re-listen
[W5500] socket_socket: domain=2, type=1, proto=0
[W5500] socket_socket: TCP socket
[W5500] socket_socket: assigned socket number 0
[W5500] socket_socket: success, fileno=0
[W5500] socket_bind: socket=0, port=8081, type=0x01
[W5500] socket_bind: WIZCHIP_EXPORT(socket) returned 0
[W5500] socket_bind: socket 0 state after open: 0x13 (INIT)
[W5500] socket_bind: success
[W5500] socket_listen: socket=0, backlog=5
[W5500] socket_listen: before - socket 0 state: 0x13 (INIT)
[W5500] socket_listen: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_listen: after - socket 0 state: 0x14 (LISTEN)
[W5500] socket_listen: saved state - port=8081, type=0x01
[W5500] socket_listen: success
Error: 32
[W5500] socket_close: socket 0
[W5500] socket_close: calling WIZCHIP_EXPORT(close) on socket 0
[W5500] socket_close: before - state=0x14 (LISTEN)
[W5500] socket_close: after - state=0x00 (CLOSED)
[W5500] socket_close: done, Python should handle re-listen
[W5500] socket_socket: domain=2, type=1, proto=0
[W5500] socket_socket: TCP socket
[W5500] socket_socket: assigned socket number 0
[W5500] socket_socket: success, fileno=0
[W5500] socket_bind: socket=0, port=8081, type=0x01
[W5500] socket_bind: WIZCHIP_EXPORT(socket) returned 0
[W5500] socket_bind: socket 0 state after open: 0x13 (INIT)
[W5500] socket_bind: success
[W5500] socket_listen: socket=0, backlog=5
[W5500] socket_listen: before - socket 0 state: 0x13 (INIT)
[W5500] socket_listen: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_listen: after - socket 0 state: 0x14 (LISTEN)
[W5500] socket_listen: saved state - port=8081, type=0x01
[W5500] socket_listen: success
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial SR=0x14 (LISTEN)


---

FILELIST:
main_webcamera_colorfix_dma_fps_fix.py
micropython/extmod/network_wiznet5k.c
micropython/lib/wiznet5k/Ethernet/wizchip_conf.h
micropython/lib/wiznet5k/Ethernet/W5500/w5500.h

