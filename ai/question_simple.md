
Analyze the following file, which is the main entry point:
`main_webcamera_colorfix_dma_fps_fix.py`.

It is a camera streaming application, where it serves a 
html on port :8081 (working), and the client (browser) through 
javascript starts the streaming, then the streaming html is 
sent in python, but the actual streaming should happen in C land only.
The actual streaming do not work, but not focus there yet.

What you should focus, is the `fileno` passing to the streaming, 
which is not available. Here is the micropython console output:
```
Stream client connected
Camera handler error: 'socket' object has no attribute 'fileno'
Socket type: <class 'socket'>
Socket dir: ['__class__', 'close', 'read', 'readinto', 'readline', 'send', 'write', '__del__', 'accept', 'bind', 'connect', 'listen', 'makefile', 'recv', 'recvfrom', 'sendall', 'sendto', 'setblocking', 'setsockopt', 'settimeout']
Has fileno: False
```
Your job is to find why `fileno` is not available, where I have
specifically modified
`micropython/extmod/network_wiznet5k.c` file, and added 
`wiznet5k_socket_fileno` function to it.

For completeness I paste all the relevant codes here, specifically:
"main_webcamera_colorfix_dma_fps_fix.py"
"modules/camera/cam.c"
"modules/camera/cam.h"
"modules/camera/micropython.cmake"
"micropython/extmod/network_wiznet5k.c"
"micropython/extmod/machine_spi.c"
"micropython/extmod/modnetwork.c"
"micropython/extmod/modnetwork.h"
"micropython/extmod/modsocket.c"

---

FILELIST:
main_webcamera_colorfix_dma_fps_fix.py
modules/camera/cam.c
modules/camera/cam.h
modules/camera/micropython.cmake
micropython/extmod/network_wiznet5k.c
micropython/extmod/machine_spi.c
micropython/extmod/modnetwork.c
micropython/extmod/modnetwork.h
micropython/extmod/modsocket.c
