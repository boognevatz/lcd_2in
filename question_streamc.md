# Description

Take the main_webcamera_colorfix_dma_fps_fix.py, it is the micropython starting application, 
study it thoroughly.
Take the `micropython/extmod/network_wiznet5k.c`, study it from at least 3 different angles. 

Please be  aware, the w5500 is a heavily modified driver, it does not use
lwIP, rather hardware tcp/ip. And w5500 uses a single 16kB RX 16kB TX on
socket  0, it is a single connection client-server architecture (the mcu,
ie. the  w550 is the server, and the client is a webbrowser).

Do not take any files from the internet. Everything is here included.

# Your job

Explain how the /stream endpoints work. 
Trace thoroughly how it flows, how it goes to C land
and back to python land.
Also compare it to the /streamc endpoints. It is a c based implementation
which freeze currently (/stream, the python based one works flawlessly)

THe /streamc console output looks like this:
mpremote
Connected to MicroPython at /dev/ttyACM0
Use Ctrl-] or Ctrl-x to exit this shell
[STREAMC] Pre-restored SPI DMACR to 0x03
[STREAMC] Entering streaming_conly_loop, socket=0
[STREAMC] Calling getSn_TxMAX(0)...
[STREAMC] TX buffer: 16384 bytes
[STREAMC] Checking cam_ptr...
[STREAMC] cam_ptr OK: 20002e51
[STREAMC] Checking tx_buffer_size...
[STREAMC] tx_buffer_size OK: 16384
[STREAMC] Calling getSn_MR(0)...
[STREAMC] getSn_MR returned 0x01
[STREAMC] Calling getSn_SR(0)...
[STREAMC] getSn_SR returned 0x17
[STREAMC] Calling getSn_IR(0)...
[STREAMC] getSn_IR returned 0x04
[STREAMC] Waiting for Python header drain (need 16384 bytes free)...
[STREAMC] Python header drained OK
[STREAMC] Waiting for TX space for boundary (51 bytes)...
[STREAMC]


Please be aware, that the console log just halts, but it does not mean 
the program freezes exactly there. It means the program freeze somehwere 
more than this line, the console is realtively slow compared to the 
actual program flow. So the exact freezing point is somehwere later in the program.

The first request successful (/), the second request (/streamc) only arrives 
22.7kB, which I believe header + 16kB (first) chunk.

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
modules/camera/w5500_tx.c

