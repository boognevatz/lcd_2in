# Very important considerations

The w5500 driver is a heavily modified one, with non lwIP path, 
single socket (16kB RX and 16kB TX, 32kB in total), single client-server
architecture for maximum throughput.
Please note the `network_wiznet5k.c` is not using lwIP! Ituses hardware based tcp/ip of w5500.

# Description

The actual driver which is a heavily modified w5500 driver without lwIP, it is in `micropython/extmod/network_wiznet5k.c`. 
The whole concept is to stream the camera image (one frame is 150kB) to w5500 chip via spi writeburst.
So once the streaming begins it is handled inside C-land and never returns.

Whenever I run `main_webcamera_colorfix_dma_fps_fix.py`, this is the console output. 
Also please note the driver is at `micropython/extmod/network_wiznet5k.py` and it is a heavily modified single client-server driver for maximum spi throughput. 

The current problem is it sends a frame (150kB), then after about 10sec it sends an another frame.
So the fps is abysmal around 0.02fps.
I suspect it only sends a single frame, and the watchdog kicks in with its 8sec timout, 
hence the big wait.

In theory the camera.start_streaming(sn) should start the actual streaming, and never return to python,
but maybe it only sends a single frame and thats it.
Lets investigate!

I don't really care about the 8 sec reboot, because I should receive at least 30*8 = 240 frames by then! And I only receive a single frame.
Lets concentrate the single frame issue, and once I have frames flowing on the wire, we can fix watchdog. Maybe the simplest just disable it in python for now. But not important. The frame issue is the most pressing one.

The output (:8081):
mpremote
Connected to MicroPython at /dev/ttyACM0
Use Ctrl-] or Ctrl-x to exit this shell
Connected. IP address: 172.16.1.1
[W5500] wiznet5k_active: n_args=1, IS_ACTIVE=1
[W5500] wiznet5k_active: getting state
Ethernet initialized successfully, IP: 172.16.1.1
MEMORY check: After network initialization | Free: 307.3KB (90.9%)
initialize CAMERA
set camera XCLK (pwm) pin: 11
call set_pwm_freq_kHz(pwm) before init_cam() to change it
set camera I2C pins: SDA: 22, SCL: 23
call set_i2c_pins(sda,scl) before init_cam() to change it
start_cam finished, camera started
setup_dma_for_capture()->DMA_CH= 1
irq_add_shared_handler: cam_handler
Camera started
MEMORY check: After Camera Init | Free: 307.2KB (90.9%)
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
[W5500] socket_listen: socket=0, backlog=5
[W5500] socket_listen: before - socket 0 state: 0x13 (INIT)
[W5500] socket_listen: WIZCHIP_EXPORT(listen) returned 1
[W5500] socket_listen: after - socket 0 state: 0x14 (LISTEN)
[W5500] socket_listen: saved state - port=8081, type=0x01
[W5500] socket_listen: success
Unified Server started on http://172.16.1.1:8081
Test Drum led control on http://172.16.1.1:8081/json/headled/10 or http://172.16.1.1:8081/json/headled/0
MEMORY check: After Webserver initialization | Free: 306.9KB (90.8%)
[W5500] socket_ioctl: request=0x3, arg=0x1, socket=0
[W5500] socket_ioctl: socket 0 - state=0x14 (LISTEN), RX_RSR=0, TX_FSR=16384
[W5500] socket_ioctl: listening socket, ir=0x00
[W5500] socket_ioctl: returning 0x0
[W5500] socket_ioctl: request=0x3, arg=0x1, socket=0
[W5500] socket_ioctl: socket 0 - state=0x14 (LISTEN), RX_RSR=0, TX_FSR=16384
[W5500] socket_ioctl: listening socket, ir=0x00
[W5500] socket_ioctl: returning 0x0
[W5500] socket_ioctl: request=0x3, arg=0x1, socket=0
[W5500] socket_ioctl: socket 0 - state=0x14 (LISTEN), RX_RSR=0, TX_FSR=16384
[W5500] socket_ioctl: listening socket, ir=0x00
[W5500] socket_ioctl: returning 0x0
[W5500] socket_accept: socket=0, timeout=-1
[W5500] socket_accept: initial state - socket 0: 0x17 (ESTABLISHED)
[W5500] socket_accept: socket already ESTABLISHED, accepting immediately
[W5500] socket_accept: client IP: 172.16.1.2, port: 35860
[W5500] socket_accept: success (single-client mode)
[W5500] socket_settimeout: socket=0, timeout_ms=2000
[W5500] socket_settimeout: set to 2000 ms
[W5500] socket_recv: socket=0, len=16384, timeout=2000
[W5500] socket_recv: checking, RX_RSR=282
[W5500] socket_recv: data available (282 bytes), calling recv
[W5500] socket_recv: WIZCHIP_EXPORT(recv) returned 282
[W5500] socket_recv: success, received 282 bytes
Socket type: <class 'socket'>
Socket dir: ['__class__', 'close', 'read', 'readinto', 'readline', 'send', 'write', '__del__', '_wiznet_sn', 'accept', 'bind', 'connect', 'listen', 'makefile', 'recv', 'recvfrom', 'sendall', 'sendto', 'setblocking', 'setsockopt',


Note: it is multiple micropython console output stiched together, 
it "quit", maybe too many console message at once.

---

FILELIST:
main_webcamera_colorfix_dma_fps_fix.py
micropython/extmod/network_wiznet5k.c
modules/camera/modcamera.c
modules/camera/cam.c



