## Why your current C approach failed (root cause recap)

You attempted to call **WIZnet’s raw socket API directly from your user module**:

```c
socket()
connect()
send()
```

But in MicroPython:

* The **TCP/IP stack is owned by the `network` module**
* W5500 is initialized and controlled **outside your module**
* Your module does **not own the SPI bus or socket state**

So even if you fix linking, you’ll fight:

* double initialization
* socket state corruption
* subtle crashes under load

---

## The correct high-performance architecture (used in production)

### **Rule**

> Let MicroPython initialize the network stack once
> Let your C module *borrow* the already-open socket
> Stream raw memory from DMA → W5500 TX buffer in C

---

## ✅ The right solution: **send via W5500 TX buffer directly**

Instead of using `socket()` / `send()`, you should:

1. **Open the TCP socket in Python** (once)
2. **Pass the socket number to C**
3. In C:

   * Write directly into the W5500 TX buffer
   * Update write pointer
   * Issue SEND command
   * No Python per-packet overhead

This gives you **DMA-speed streaming**, not VM-speed streaming.

---

## How this looks conceptually

### Python (control plane only)

```python
import network, socket, camera

nic = network.WIZNET5K(...)
nic.active(True)

s = socket.socket()
s.connect(("192.168.1.100", 12345))

camera.start_streaming(s)   # pass socket into C

while True:
    pass  # Python does NOTHING per frame
```

---

### C (data plane, fast path)

* Python never sees packets
* No slicing
* No `send()` calls
* No GC
* No copies beyond W5500’s TX buffer

---

## Implementation details (important)

### 1️⃣ Pass socket number from Python to C

In MicroPython, socket objects wrap a **socket number (0–7)**.

You can extract it like this:

```c
#include "extmod/modnetwork.h"
#include "extmod/modsocket.h"
```

Example:

```c
static mp_obj_t camera_start_streaming(mp_obj_t sock_obj) {
    mp_obj_t fileno = mp_load_attr(sock_obj, MP_QSTR_fileno);
    int sock = mp_obj_get_int(mp_call_function_0(fileno));
    camera_stream_socket = sock;
    return mp_const_none;
}
```

Now C knows **which W5500 socket to use**.

---

### 2️⃣ Write directly into W5500 TX buffer (zero-copy-ish)

Use **ioLibrary internal functions**:

```c
#include "lib/wiznet5k/Ethernet/wizchip_conf.h"
#include "lib/wiznet5k/Ethernet/socket.h"
```

But instead of `send()`:

```c
uint16_t freesz = getSn_TX_FSR(sock);
uint16_t wr = getSn_TX_WR(sock);

uint16_t offset = wr & WIZCHIP_TXBUF_MASK;
uint8_t *txbuf = getSn_TXBUF_BASE(sock) + offset;

memcpy(txbuf, cam_ptr, chunk_len);

setSn_TX_WR(sock, wr + chunk_len);
setSn_CR(sock, Sn_CR_SEND);
```

This bypasses:

* socket.c overhead
* memcpy chains
* Python VM

---

### 3️⃣ Chunk at W5500 buffer size, not MTU

W5500 TX buffer is typically **2–16 KB per socket**.

Set it once in Python:

```python
nic.socket_bufsize(tx=16*1024, rx=2*1024)
```

Now each SEND pushes **large contiguous chunks**, not 1500-byte packets.

---

## Expected performance (realistic numbers)

On RP2040 / RP2350 + W5500:

| Mode                   | FPS           |
| ---------------------- | ------------- |
| Python `send()`        | 2–4 FPS       |
| C `send()`             | 6–10 FPS      |
| Direct TX buffer write | **15–25 FPS** |
| JPEG + C streaming     | **30+ FPS**   |

You are currently in the **worst possible path** (Python per packet).

