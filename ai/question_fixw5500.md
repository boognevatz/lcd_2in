Looking at your issue, the problem appears to be related to socket handling in the WIZNET5K/W5500 native stack implementation. The key issue is that `setsockopt` and `setblocking` fail with `EINVAL` because the native W5500 driver doesn't fully implement these functions yet.

## Root Cause Analysis

From your `modsocket.c` and `network_wiznet5k.c`, I can see:

1. **Missing `setsockopt` Implementation**: In `wiznet5k_socket_setsockopt()` (line ~366 in `network_wiznet5k.c`), only `SO_REUSEADDR` is handled as a no-op. Other socket options are rejected with `EINVAL`.

2. **Missing `settimeout` Implementation**: `wiznet5k_socket_settimeout()` returns `EINVAL` without implementation.

3. **Socket Polling Issue**: The `wiznet5k_socket_ioctl()` only checks for data in buffers but doesn't handle listening sockets properly.

## Required Fixes

Here are the minimal changes needed to make your web server work:

### 1. **Fix `modsocket.c` - Remove `setsockopt` and `setblocking` calls**

Since these aren't supported, modify `main_bareminimum.py` to avoid them:

```python
def handle_json_request(cl):
    """Handle requests on port 8082 - serve JSON status data or handle LED control"""
    try:
        # REMOVE OR COMMENT OUT THIS LINE:
        # cl.settimeout(2.0)
        
        # Read the HTTP request with a simple loop instead
        request = b""
        for _ in range(100):  # Try reading for ~2 seconds
            try:
                chunk = cl.recv(1024)
                if chunk:
                    request += chunk
                    if b'\r\n\r\n' in request:  # End of headers
                        break
            except Exception:
                pass
            time.sleep_ms(20)
        
        if not request:
            cl.close()
            return
            
        request = request.decode("utf-8", errors="ignore")
        # ... rest of your code ...
```

### 2. **Debug Script to Test Basic Connectivity**

Create a minimal test script to verify socket functionality:

```python
import machine
import network
import socket
import time
import select

# W5500 Initialization
def init_network():
    cs = machine.Pin(17, machine.Pin.OUT)
    rst = machine.Pin(20, machine.Pin.OUT)
    
    cs.value(1)
    time.sleep_ms(10)
    
    rst.value(0)
    time.sleep_ms(100)
    rst.value(1)
    time.sleep_ms(500)
    
    spi = machine.SPI(
        0,
        baudrate=20000000,
        polarity=0,
        phase=0,
        sck=machine.Pin(18),
        mosi=machine.Pin(19),
        miso=machine.Pin(16),
    )
    
    time.sleep_ms(100)
    nic = network.WIZNET5K(spi, cs, rst)
    nic.active(True)
    time.sleep_ms(100)
    nic.ifconfig(("172.16.1.1", "255.255.255.0", "172.16.1.1", "8.8.8.8"))
    time.sleep_ms(500)
    
    return nic

def test_socket():
    nic = init_network()
    
    if not nic.active():
        print("Network failed to initialize")
        return
    
    print(f"Network active: {nic.active()}")
    print(f"IP: {nic.ifconfig()[0]}")
    
    # Create socket
    s = socket.socket()
    addr = socket.getaddrinfo("0.0.0.0", 8082)[0][-1]
    s.bind(addr)
    s.listen(1)
    
    print(f"Listening on {nic.ifconfig()[0]}:8082")
    print("Ready for connections...")
    
    poller = select.poll()
    poller.register(s, select.POLLIN)
    
    while True:
        events = poller.poll(1000)
        if events:
            for sock, event in events:
                if sock == s:
                    try:
                        cl, addr = s.accept()
                        print(f"Connection from {addr}")
                        
                        # Send simple response
                        response = "HTTP/1.1 200 OK\r\n"
                        response += "Content-Type: text/plain\r\n"
                        response += "Connection: close\r\n\r\n"
                        response += "Hello from W5500!"
                        
                        cl.send(response.encode())
                        cl.close()
                        print("Response sent")
                    except Exception as e:
                        print(f"Error handling connection: {e}")
        
        time.sleep_ms(100)

if __name__ == "__main__":
    test_socket()
```

### 3. **Critical C Code Fix in `network_wiznet5k.c`**

The main issue is in the `wiznet5k_socket_ioctl()` function. It needs to properly handle listening sockets:

```c
static int wiznet5k_socket_ioctl(mod_network_socket_obj_t *socket, mp_uint_t request, mp_uint_t arg, int *_errno) {
    if (request == MP_STREAM_POLL) {
        int ret = 0;
        uint8_t sn = (uint8_t)socket->fileno;
        
        if (arg & MP_STREAM_POLL_RD) {
            // Check socket status for listening sockets
            uint8_t sr = getSn_SR(sn);
            
            if (sr == SOCK_LISTEN) {
                // For listening sockets, check if there are pending connections
                // by looking at the interrupt register
                uint8_t ir = getSn_IR(sn);
                if (ir & Sn_IR_CON) {
                    ret |= MP_STREAM_POLL_RD;
                }
            } else if (sr == SOCK_ESTABLISHED || sr == SOCK_CLOSE_WAIT) {
                // For established sockets, check RX buffer
                if (getSn_RX_RSR(sn) > 0) {
                    ret |= MP_STREAM_POLL_RD;
                }
            }
        }
        
        if (arg & MP_STREAM_POLL_WR) {
            // For write availability, check TX buffer
            if (getSn_TX_FSR(sn) > 0) {
                ret |= MP_STREAM_POLL_WR;
            }
        }
        
        // Always allow HUP/ERR checks
        if (arg & MP_STREAM_POLL_HUP) {
            uint8_t sr = getSn_SR(sn);
            if (sr == SOCK_CLOSED || sr == SOCK_CLOSE_WAIT) {
                ret |= MP_STREAM_POLL_HUP;
            }
        }
        
        return ret;
    } else {
        *_errno = MP_EINVAL;
        return MP_STREAM_ERROR;
    }
}
```

### 4. **Add Missing `setsockopt` Support**

In `network_wiznet5k.c`, update `wiznet5k_socket_setsockopt()`:

```c
static int wiznet5k_socket_setsockopt(mod_network_socket_obj_t *socket, mp_uint_t level, mp_uint_t opt, const void *optval, mp_uint_t optlen, int *_errno) {
    // Handle socket-level options
    if (level == MOD_NETWORK_SOL_SOCKET) {
        switch (opt) {
            case MOD_NETWORK_SO_REUSEADDR:
                // Accept but ignore for W5500
                return 0;
                
            case MOD_NETWORK_SO_RCVTIMEO:
            case MOD_NETWORK_SO_SNDTIMEO:
                // Handle timeouts if needed
                return 0;
                
            default:
                *_errno = MP_EINVAL;
                return -1;
        }
    }
    
    // TCP-level options (if needed)
    // if (level == IPPROTO_TCP) { ... }
    
    *_errno = MP_ENOPROTOOPT;
    return -1;
}
```

### 5. **Enhanced Debug Version of Main Script**

Here's a debug version that adds logging and handles the W5500 limitations:

```python
import machine
import network
import socket
import time
import select
import json
import gc

print("=== W5500 WEBSERVER DEBUG VERSION ===")

# Network initialization with more debugging
cs = machine.Pin(17, machine.Pin.OUT)
rst = machine.Pin(20, machine.Pin.OUT)

cs.value(1)
time.sleep_ms(10)

rst.value(0)
print("Resetting W5500...")
time.sleep_ms(100)
rst.value(1)
time.sleep_ms(500)
print("W5500 reset complete")

spi = machine.SPI(
    0,
    baudrate=20000000,
    polarity=0,
    phase=0,
    sck=machine.Pin(18),
    mosi=machine.Pin(19),
    miso=machine.Pin(16),
)

time.sleep_ms(100)

nic = network.WIZNET5K(spi, cs, rst)
print("Network object created")

nic.active(True)
time.sleep_ms(100)
print(f"Network active: {nic.active()}")

nic.ifconfig(("172.16.1.1", "255.255.255.0", "172.16.1.1", "8.8.8.8"))
time.sleep_ms(500)

config_net = nic.ifconfig()
print(f"Network configured: {config_net}")

# Create socket with error handling
try:
    addr_json = socket.getaddrinfo("0.0.0.0", 8082)[0][-1]
    s_json = socket.socket()
    print("Socket created")
    
    s_json.bind(addr_json)
    print(f"Socket bound to {addr_json}")
    
    s_json.listen(5)
    print("Socket listening")
    
except Exception as e:
    print(f"Socket error: {e}")
    raise

print(f"\nServer ready: http://{config_net[0]}:8082")
print("Waiting for connections...")

poller = select.poll()
poller.register(s_json, select.POLLIN)

connection_count = 0

while True:
    try:
        events = poller.poll(100)  # 100ms timeout
        
        for sock, event in events:
            if sock == s_json:
                connection_count += 1
                print(f"\n[{connection_count}] New connection detected")
                
                try:
                    cl, addr = s_json.accept()
                    print(f"[{connection_count}] Accepted from {addr}")
                    
                    # Try to read request (non-blocking approach)
                    request_data = b""
                    for i in range(10):  # Try for ~200ms
                        try:
                            chunk = cl.recv(1024)
                            if chunk:
                                request_data += chunk
                                if b'\r\n\r\n' in request_data:
                                    break
                        except:
                            pass
                        time.sleep_ms(20)
                    
                    if request_data:
                        print(f"[{connection_count}] Received {len(request_data)} bytes")
                        
                        # Simple response
                        response = "HTTP/1.1 200 OK\r\n"
                        response += "Content-Type: application/json\r\n"
                        response += "Connection: close\r\n\r\n"
                        response += '{"status": "ok", "message": "W5500 Server Working"}'
                        
                        cl.send(response.encode())
                        print(f"[{connection_count}] Response sent")
                    else:
                        print(f"[{connection_count}] No data received")
                    
                    cl.close()
                    print(f"[{connection_count}] Connection closed")
                    
                except Exception as e:
                    print(f"[{connection_count}] Error: {e}")
    
    except KeyboardInterrupt:
        print("\nShutting down...")
        break
    except Exception as e:
        print(f"Loop error: {e}")
        time.sleep_ms(1000)

print("Server stopped")
```

## Immediate Next Steps

1. **Run the debug script** first to see if basic socket functionality works
2. **Check W5500 initialization** - the reset sequence and SPI communication
3. **Monitor with a packet sniffer** to see if TCP SYN packets reach the W5500
4. **Add debug prints to the C code** to trace socket state changes

The most likely issue is that the W5500 socket isn't transitioning properly between states (LISTEN → ESTABLISHED) when connections arrive. The polling mechanism might not be detecting the state change correctly.

If the debug script works but your original doesn't, the issue is likely in the HTTP request handling logic. If neither works, the issue is in the W5500 driver's socket state management.
