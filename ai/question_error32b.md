# Description

Take the main_webcamera_colorfix_dma_fps_fix.py, it is the micropython starting application, 
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



# Micropyhton log0


mpremote
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
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=10030a5d, sn=0, buf=2003ce30, len=8192

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
   READ_ADDR:        0x2003CF61
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
[W5500] w5500_send_chunk: sn=0, len=132, tx_wr_ptr=6247, offset=6247, space_to_end=10137

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2003CF61
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

[W5500] w5500_dma_write_to_txbuf: wrote 132 bytes to socket 0 offset 6247

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
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=6379, offset=6379, space_to_end=10005

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

[W5500] w5500_dma_write_to_txbuf: wrote 10005 bytes to socket 0 offset 6379

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
   READ_ADDR:        0x20005562
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

[W5500] w5500_dma_write_to_txbuf: wrote 6379 bytes to socket 0 offset 0

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
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=22763, offset=6379, space_to_end=10005

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

[W5500] w5500_dma_write_to_txbuf: wrote 10005 bytes to socket 0 offset 6379

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
   READ_ADDR:        0x20009562
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

[W5500] w5500_dma_write_to_txbuf: wrote 6379 bytes to socket 0 offset 0

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
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=39147, offset=6379, space_to_end=10005

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

[W5500] w5500_dma_write_to_txbuf: wrote 10005 bytes to socket 0 offset 6379

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
   READ_ADDR:        0x2000D562
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

[W5500] w5500_dma_write_to_txbuf: wrote 6379 bytes to socket 0 offset 0

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
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=55531, offset=6379, space_to_end=10005

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

[W5500] w5500_dma_write_to_txbuf: wrote 10005 bytes to socket 0 offset 6379

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
   READ_ADDR:        0x20011562
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

[W5500] w5500_dma_write_to_txbuf: wrote 6379 bytes to socket 0 offset 0

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
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=6379, offset=6379, space_to_end=10005

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

[W5500] w5500_dma_write_to_txbuf: wrote 10005 bytes to socket 0 offset 6379

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
   READ_ADDR:        0x20015562
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

[W5500] w5500_dma_write_to_txbuf: wrote 6379 bytes to socket 0 offset 0

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
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=22763, offset=6379, space_to_end=10005

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

[W5500] w5500_dma_write_to_txbuf: wrote 10005 bytes to socket 0 offset 6379

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
   READ_ADDR:        0x20019562
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

[W5500] w5500_dma_write_to_txbuf: wrote 6379 bytes to socket 0 offset 0

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

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 98304/153600 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=39147, offset=6379, space_to_end=10005

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
   Active[1]: socket=0 state=1 bytes=98304/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2001AE4D
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
   Active[1]: socket=0 state=1 bytes=98304/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 10005 bytes to socket 0 offset 6379

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
   Active[1]: socket=0 state=1 bytes=98304/153600
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
   Active[1]: socket=0 state=1 bytes=98304/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2001D562
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
   Active[1]: socket=0 state=1 bytes=98304/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 6379 bytes to socket 0 offset 0

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
   Active[1]: socket=0 state=1 bytes=98304/153600
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 114688/153600 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=55531, offset=6379, space_to_end=10005

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
   Active[1]: socket=0 state=1 bytes=114688/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2001EE4D
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
   Active[1]: socket=0 state=1 bytes=114688/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 10005 bytes to socket 0 offset 6379

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
   Active[1]: socket=0 state=1 bytes=114688/153600
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
   Active[1]: socket=0 state=1 bytes=114688/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x20021562
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
   Active[1]: socket=0 state=1 bytes=114688/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 6379 bytes to socket 0 offset 0

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
   Active[1]: socket=0 state=1 bytes=114688/153600
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 131072/153600 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=6379, offset=6379, space_to_end=10005

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
   Active[1]: socket=0 state=1 bytes=131072/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x20022E4D
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
   Active[1]: socket=0 state=1 bytes=131072/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 10005 bytes to socket 0 offset 6379

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
   Active[1]: socket=0 state=1 bytes=131072/153600
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
   Active[1]: socket=0 state=1 bytes=131072/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x20025562
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
   Active[1]: socket=0 state=1 bytes=131072/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 6379 bytes to socket 0 offset 0

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
   Active[1]: socket=0 state=1 bytes=131072/153600
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 147456/153600 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=6144, tx_wr_ptr=22763, offset=6379, space_to_end=10005

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
   Active[1]: socket=0 state=1 bytes=147456/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x20026E4D
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
   Active[1]: socket=0 state=1 bytes=147456/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 6144 bytes to socket 0 offset 6379

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
   Active[1]: socket=0 state=1 bytes=147456/153600
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 6144 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 153600/153600 bytes sent on socket 0
[W5500] w5500_tx_service: transfer complete, dequeuing socket 0
[W5500] socket_send: transfer complete, sent 153600 bytes
[W5500] socket_close: socket 0
[W5500] socket_close: calling WIZCHIP_EXPORT(close) on socket 0
[W5500] socket_close: before - state=0x1c (CLOSE_WAIT)
[W5500] socket_close: after - state=0x00 (CLOSED)
[W5500] socket_close: done, Python should handle re-listen
Error: 32
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
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial SR=0x14 (LISTEN)
[W5500] socket_accept: success
[W5500] socket_accept: after accept - SR=0x17 RX_RSR=305 IR=0x04
[W5500] socket_recv: ENTRY socket=0, len=8192, timeout=-1
[W5500] socket_recv: checking, RX_RSR=305
[W5500] socket_recv: data available (305 bytes), calling recv
[W5500] before MP_THREAD_GIL_EXIT
[W5500] after MP_THREAD_GIL_EXIT, before WIZCHIP_EXPORT(recv): recv=10030a5d, sn=0, buf=2003cf70, len=8192

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
   READ_ADDR:        0x2003D0A1
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
[W5500] w5500_send_chunk: sn=0, len=132, tx_wr_ptr=21156, offset=4772, space_to_end=11612

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2003D0A1
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

[W5500] w5500_dma_write_to_txbuf: wrote 132 bytes to socket 0 offset 4772

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
   READ_ADDR:        0x2002CF14
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
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=21288, offset=4904, space_to_end=11480

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: Before DMA Init Check
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2002CF14
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

[W5500] w5500_dma_write_to_txbuf: wrote 11480 bytes to socket 0 offset 4904

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
   READ_ADDR:        0x20005B25
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

[W5500] w5500_dma_write_to_txbuf: wrote 4904 bytes to socket 0 offset 0

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
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=37672, offset=4904, space_to_end=11480

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

[W5500] w5500_dma_write_to_txbuf: wrote 11480 bytes to socket 0 offset 4904

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
   READ_ADDR:        0x20009B25
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

[W5500] w5500_dma_write_to_txbuf: wrote 4904 bytes to socket 0 offset 0

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
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=54056, offset=4904, space_to_end=11480

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

[W5500] w5500_dma_write_to_txbuf: wrote 11480 bytes to socket 0 offset 4904

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
   READ_ADDR:        0x2000DB25
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

[W5500] w5500_dma_write_to_txbuf: wrote 4904 bytes to socket 0 offset 0

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
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=4904, offset=4904, space_to_end=11480

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

[W5500] w5500_dma_write_to_txbuf: wrote 11480 bytes to socket 0 offset 4904

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
   READ_ADDR:        0x20011B25
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

[W5500] w5500_dma_write_to_txbuf: wrote 4904 bytes to socket 0 offset 0

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
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=21288, offset=4904, space_to_end=11480

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

[W5500] w5500_dma_write_to_txbuf: wrote 11480 bytes to socket 0 offset 4904

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
   READ_ADDR:        0x20015B25
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

[W5500] w5500_dma_write_to_txbuf: wrote 4904 bytes to socket 0 offset 0

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
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=37672, offset=4904, space_to_end=11480

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

[W5500] w5500_dma_write_to_txbuf: wrote 11480 bytes to socket 0 offset 4904

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
   READ_ADDR:        0x20019B25
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

[W5500] w5500_dma_write_to_txbuf: wrote 4904 bytes to socket 0 offset 0

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

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 98304/153600 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=54056, offset=4904, space_to_end=11480

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
   Active[1]: socket=0 state=1 bytes=98304/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2001AE4D
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
   Active[1]: socket=0 state=1 bytes=98304/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 11480 bytes to socket 0 offset 4904

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
   Active[1]: socket=0 state=1 bytes=98304/153600
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
   Active[1]: socket=0 state=1 bytes=98304/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2001DB25
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
   Active[1]: socket=0 state=1 bytes=98304/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 4904 bytes to socket 0 offset 0

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
   Active[1]: socket=0 state=1 bytes=98304/153600
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 114688/153600 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=4904, offset=4904, space_to_end=11480

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
   Active[1]: socket=0 state=1 bytes=114688/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x2001EE4D
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
   Active[1]: socket=0 state=1 bytes=114688/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 11480 bytes to socket 0 offset 4904

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
   Active[1]: socket=0 state=1 bytes=114688/153600
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
   Active[1]: socket=0 state=1 bytes=114688/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x20021B25
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
   Active[1]: socket=0 state=1 bytes=114688/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 4904 bytes to socket 0 offset 0

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
   Active[1]: socket=0 state=1 bytes=114688/153600
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 131072/153600 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=16384, tx_wr_ptr=21288, offset=4904, space_to_end=11480

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
   Active[1]: socket=0 state=1 bytes=131072/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x20022E4D
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
   Active[1]: socket=0 state=1 bytes=131072/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 11480 bytes to socket 0 offset 4904

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
   Active[1]: socket=0 state=1 bytes=131072/153600
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
   Active[1]: socket=0 state=1 bytes=131072/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x20025B25
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
   Active[1]: socket=0 state=1 bytes=131072/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 4904 bytes to socket 0 offset 0

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
   Active[1]: socket=0 state=1 bytes=131072/153600
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 16384 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 147456/153600 bytes sent on socket 0
[W5500] w5500_send_chunk: sn=0, len=6144, tx_wr_ptr=37672, offset=4904, space_to_end=11480

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
   Active[1]: socket=0 state=1 bytes=147456/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_init: claimed TX channel 1

========================================================
           DMA CHANNEL STATE DUMP
========================================================
 Context: After DMA Channel Claimed
--------------------------------------------------------
 TX DMA Channel: 1
   READ_ADDR:        0x20026E4D
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
   Active[1]: socket=0 state=1 bytes=147456/153600
   Active Count:     1
========================================================

[W5500] w5500_dma_write_to_txbuf: wrote 6144 bytes to socket 0 offset 4904

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
   Active[1]: socket=0 state=1 bytes=147456/153600
   Active Count:     1
========================================================

[W5500] w5500_tx_service: started DMA for 6144 bytes on socket 0
[W5500] w5500_tx_service: DMA complete, SEND command issued on socket 0
[W5500] w5500_tx_service: SENDOK, 153600/153600 bytes sent on socket 0
[W5500] w5500_tx_service: transfer complete, dequeuing socket 0
[W5500] socket_send: transfer complete, sent 153600 bytes
[W5500] socket_close: socket 0
[W5500] socket_close: calling WIZCHIP_EXPORT(close) on socket 0
[W5500] socket_close: before - state=0x1c (CLOSE_WAIT)
[W5500] socket_close: after - state=0x00 (CLOSED)
[W5500] socket_close: done, Python should handle re-listen
Error: 32
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
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial SR=0x14 (LISTEN)


---

FILELIST:
main_webcamera_colorfix_dma_fps_fix.py
micropython/extmod/network_wiznet5k.c
micropython/lib/wiznet5k/Ethernet/wizchip_conf.h
micropython/lib/wiznet5k/Ethernet/W5500/w5500.h


