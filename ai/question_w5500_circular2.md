# Very important considerations

The w5500 driver is a heavily modified one, with non lwIP path, 
single socket (16kB RX and 16kB TX, 32kB in total), single client-server
architecture for maximum throughput.
Please note the `network_wiznet5k.c` is not using lwIP! Ituses hardware based tcp/ip of w5500.

# Description

The actual driver which is a heavily modified w5500 driver without lwIP, it is in `micropython/extmod/network_wiznet5k.c`. 

Whenever I run `main_webcamera_colorfix_dma_fps_fix.py`, this is the console output. Also please not the driver is at `micropython/extmod/network_wiznet5k.py` and it is a heavily modified single client-server driver for mayimum spi throughput. 

I can connect on :8081, but /stream endpoint do not work (0GB received), I can not connect to :8082


The output (:8082):
❯ mpremote
Connected to MicroPython at /dev/ttyACM0
Use Ctrl-] or Ctrl-x to exit this shell
Waiting for Ethernet link...
Connected. IP address: 172.16.1.1
[W5500] wiznet5k_active: n_args=1, IS_ACTIVE=1
[W5500] wiznet5k_active: getting state
Ethernet initialized successfully, IP: 172.16.1.1
MEMORY check: After network initialization | Free: 307.6KB (91.0%)
initialize CAMERA
set camera XCLK (pwm) pin: 11
call set_pwm_freq_kHz(pwm) before init_cam() to change it
set camera I2C pins: SDA: 22, SCL: 23
call set_i2c_pins(sda,scl) before init_cam() to change it
start_cam finished, camera started
setup_dma_for_capture()->DMA_CH= 1
irq_add_shared_handler: cam_handler
Camera started
MEMORY check: After Camera Init | Free: 307.5KB (91.0%)
[W5500] wiznet5k_active: n_args=1, IS_ACTIVE=1
[W5500] wiznet5k_active: getting state
[W5500] socket_socket: domain=2, type=1, proto=0
[W5500] socket_socket: TCP socket
[W5500] socket_socket: assigned socket number 0
[W5500] socket_socket: success, fileno=0
[W5500] socket_bind: socket=0, port=8081, type=0x01
[W5500] socket_bind: WIZCHIP_EXPORT(socket) returned 0
[W5500] socket_bind: socket 0 state after open: 0x13 (INIT)
[W5500] socket_bind: success
[W5500] socket_listen: socket=0, backlog=3
[W5500] socket_listen: before - socket 0 state: 0x13 (INIT)
[W5500] socket_listen: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_listen: after - socket 0 state: 0x14 (LISTEN)
[W5500] socket_listen: saved state - port=8081, type=0x01
[W5500] socket_listen: success
[W5500] socket_socket: domain=2, type=1, proto=0
[W5500] socket_socket: TCP socket
[W5500] socket_socket: assigned socket number 1
[W5500] socket_socket: success, fileno=1
[W5500] socket_bind: socket=1, port=8082, type=0x01
[W5500] socket_bind: WIZCHIP_EXPORT(socket) returned 1
[W5500] socket_bind: socket 1 state after open: 0x13 (INIT)
[W5500] socket_bind: success
[W5500] socket_listen: socket=1, backlog=5
[W5500] socket_listen: before - socket 1 state: 0x13 (INIT)
[W5500] socket_listen: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_listen: after - socket 1 state: 0x14 (LISTEN)
[W5500] socket_listen: saved state - port=8082, type=0x01
[W5500] socket_listen: success
Camera Server started on http://172.16.1.1:8081
Sensor JSON Server started on http://172.16.1.1:8082
Test Drum led control on http://172.16.1.1:8082/headled/10 or http://172.16.1.1:8082/headled/0
MEMORY check: After Webserver initialization | Free: 307.2KB (90.9%)
[W5500] socket_ioctl: request=0x3, arg=0x1, socket=0
[W5500] socket_ioctl: socket 0 - state=0x14 (LISTEN), RX_RSR=0, TX_FSR=16384
[W5500] socket_ioctl: listening socket, ir=0x00
[W5500] socket_ioctl: returning 0x0
[W5500] socket_ioctl: request=0x3, arg=0x1, socket=1
[W5500] socket_ioctl: socket 1 - state=0x14 (LISTEN), RX_RSR=0, TX_FSR=0
[W5500] socket_ioctl: listening socket, ir=0x00
[W5500] socket_ioctl: returning 0x0
[W5500] socket_ioctl: request=0x3, arg=0x1, socket=0
[W5500] socket_ioctl: socket 0 - state=0x14 (LISTEN), RX_RSR=0, TX_FSR=16384
[W5500] socket_ioctl: listening socket, ir=0x00
[W5500] socket_ioctl: returning 0x0
[W5500] socket_ioctl: request=0x3, arg=0x1, socket=1
[W5500] socket_ioctl: socket 1 - state=0x14 (LISTEN), RX_RSR=0, TX_FSR=0
[W5500] socket_ioctl: listening socket, ir=0x00
[W5500] socket_ioctl: returning 0x0
[W5500] socket_ioctl: request=0x3, arg=0x1, socket=0
[W5500] socket_ioctl: socket 0 - state=0x14 (LISTEN), RX_RSR=0, TX_FSR=16384
[W5500] socket_ioctl: listening socket, ir=0x00
[W5500] socket_ioctl: returning 0x0
[W5500] socket_ioctl: request=0x3, arg=0x1, socket=1
[W5500] socket_ioctl: socket 1 - state=0x14 (LISTEN), RX_RSR=0, TX_FSR=0
[W5500] socket_ioctl: listening socket, ir=0x00
[W5500] socket_ioctl: returning 0x0
[W5500] socket_ioctl: request=0x3, arg=0x1, socket=0
[W5500] socket_ioctl: socket 0 - state=0x14 (LISTEN), RX_RSR=0, TX_FSR=16384 

(and the last 7 linex repeat for infinity)


---

FILELIST:
main_webcamera_colorfix_dma_fps_fix.py
micropython/extmod/network_wiznet5k.c




