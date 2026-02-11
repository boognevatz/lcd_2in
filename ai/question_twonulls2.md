# Description

Take the main_webcamera_colorfix_dma_fps_fix.py, it is the micropython starting application, 
study it thoroughly.
Take the `micropython/extmod/network_wiznet5k.c`, study it from at least 3 different angles. 
Read the `modules/camera/modcamera.c` and study it.
Read the `modules/camera/cam.c` and study it.

Please be  aware, the w5500 is a heavily modified driver, it does not use
lwIP, rather hardware tcp/ip. And w5500 uses a single 16kB RX 16kB TX on
socket  0, it is a single connection client-server architecture (the mcu,
ie. the  w550 is the server, and the client is a webbrowser).

Do not take any files from the internet. Everything is here included.

# Problem:
The camera stream has a repeating pattern where every 2 valid data bytes are followed by 2 null bytes:
- Pattern: `0x23 0x45 0x00 0x00 0x45 0x66 0x00 0x00 ...`
- Expected: `0x23 0x45 0x33 0x41 0x45 0x66 ...` (continuous data)

# What We Know:

The unmodified (current state) firmware accepts HTTP connections and streams data (but with the null byte bug)

Micropython log:

Here is the output from a firmware without modification (note: debug = True in main_webcamera_colorfix_dma_fps_fix.py
Without modification:
mpremote
Connected to MicroPython at /dev/ttyACM0
Use Ctrl-] or Ctrl-x to exit this shell
Waiting for Ethernet link...
Connected. IP address: 172.16.1.1
Ethernet initialized successfully, IP: 172.16.1.1
MEMORY check: After network initialization | Free: 296.3KB (87.7%)
initialize CAMERA
set camera XCLK (pwm) pin: 11
call set_pwm_freq_kHz(pwm) before init_cam() to change it
set camera I2C pins: SDA: 22, SCL: 23
call set_i2c_pins(sda,scl) before init_cam() to change it
start_cam finished, camera started
setup_dma_for_capture()->DMA_CH= 0
irq_add_shared_handler: cam_handler
Camera started
MEMORY check: After Camera Init | Free: 296.3KB (87.7%)
[DIAG] create_server_socket: starting
[DIAG] create_server_socket: socket() done
[DIAG] create_server_socket: bind() done
[DIAG] create_server_socket: listen() done
[MAIN] Server socket created and listening on port 80
[DIAG] create_server_socket: about to return
[DIAG] Top of main loop
[DIAG] gc.collect() done
[DIAG] About to call s.accept()
[MAIN] ====== WAITING FOR CONNECTION ======
[DIAG] s.accept() returned
[MAIN] recv() returned 81 bytes
[MAIN] Request split into 6 lines
[MAIN] First line: GET /streamc HTTP/1.1...
[MAIN] Split into 3 parts
[MAIN] Parsed: method=GET, path=/streamc
[MAIN] Request path: /streamc
Stream client connected - header sent
Starting C-based streaming loop


# What we tried so far:

## A) cam.c -> setup_dma_for_capture() 
Changing 
    `channel_config_set_transfer_data_size(&c0, DMA_SIZE_16);`
to:
    `channel_config_set_transfer_data_size(&c0, DMA_SIZE_32);`

OR: completely removing the above line (the exact same result)

Resulted to not accepting any connection.
Micropython log:
mpremote
Connected to MicroPython at /dev/ttyACM0
Use Ctrl-] or Ctrl-x to exit this shell
Waiting for Ethernet link...
Connected. IP address: 172.16.1.1
Ethernet initialized successfully, IP: 172.16.1.1
MEMORY check: After network initialization | Free: 296.3KB (87.7%)
initialize CAMERA
set camera XCLK (pwm) pin: 11
call set_pwm_freq_kHz(pwm) before init_cam() to change it
set camera I2C pins: SDA: 22, SCL: 23
call set_i2c_pins(sda,scl) before init_cam() to change it
start_cam finished, camera started
setup_dma_for_capture()->DMA_CH= 0
irq_add_shared_handler: cam_handler
Camera started
MEMORY check: After Camera Init | Free: 296.3KB (87.7%)
[DIAG] create_server_socket: starting
[DIAG] create_server_socket: socket() done
[DIAG] create_server_socket: bind() done
[DIAG] create_server_socket: listen() done
[MAIN] Server socket created and listening on port 80
[DIAG] create_server_socket: about to return
[DIAG] Top of main loop
[DIAG] gc.collect() done
[DIAG] About to call s.accept()
[MAIN] ====== WAITING FOR CONNECTION ======

##  B)cam.c -> setup_dma_for_capture() 
Changing:

    dma_channel_configure(DMA_CAM_RD_CH, &c0,
                          cam_ptr,               // Destination pointer
                          &pio_cam->rxf[sm_cam], // Source pointer
                          sizeof(cam_buffer) / 2,          // Number of transfers
                          false                  // Don't Start yet
to:
    dma_channel_configure(DMA_CAM_RD_CH, &c0,
                          cam_ptr,               // Destination pointer
                          &pio_cam->rxf[sm_cam], // Source pointer
                          sizeof(cam_buffer) / 4,          // Number of transfers
                          false                  // Don't Start yet

It still has the same original bug (2 valid bytes then 2 empty bytes)



# Further instructions

Make the absolute minimum modification necessary. No need to rewrite anything
big. 

This time be extra thorough read the source code at least 5 times, five different 
angles, do not use anything from the internet everything is here included.
We were talking about this 4 times, and you were hallucinating random things
from the internet, which is not even the sourcecode. So use 
the below files as the only source of truth.

---

FILELIST:
main_webcamera_colorfix_dma_fps_fix.py
micropython/extmod/network_wiznet5k.c
modules/camera/modcamera.c
modules/camera/cam.c

