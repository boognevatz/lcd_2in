### SIMPLIFIED ETHERNET WEBSERVER WITH W5500 AND CAMERA
import machine
import network
import socket
import time
import camera
import gc

DEBUG = False

if DEBUG:
    debug_print = print
else:
    def debug_print(*args, **kwargs):
        _ = args, kwargs
        pass


# ===== MEMORY MONITORING =====
def get_memory_stats():
    """Get memory statistics without printing"""
    gc.collect()
    free = gc.mem_free()
    alloc = gc.mem_alloc()
    total = free + alloc
    return {"total": total, "used": alloc, "free": free}

def print_memory_stats(label=""):
    """Print compact memory statistics"""
    stats = get_memory_stats()
    free_kb = stats["free"] / 1024
    total = stats["total"]
    free_pct = stats["free"] * 100 / total
    debug_print(f"MEMORY check: {label} | Free: {free_kb:.1f}KB ({free_pct:.1f}%)")
    return stats
# ============================


# W5500 Pin Configuration for RP2350A
# MISO: GPIO16
# SCSN: GPIO17
# SCLK: GPIO18
# MOSI: GPIO19
# RSTN: GPIO20
# INTN: GPIO21

# Chip Select and Reset pins - initialize first
cs = machine.Pin(17, machine.Pin.OUT)
rst = machine.Pin(20, machine.Pin.OUT)

# Initialize CS high (inactive) before any SPI activity
cs.value(1)
time.sleep_ms(10)

# Perform hardware reset on W5500
rst.value(0)  # Pull reset LOW
time.sleep_ms(100)  # Hold low for 100ms
rst.value(1)  # Pull reset HIGH to enable chip
time.sleep_ms(500)  # Wait longer for chip to fully initialize

# Initialize SPI for W5500
spi = machine.SPI(
    0,
    baudrate=40_000_000,
    polarity=0,
    phase=0,
    sck=machine.Pin(18),
    mosi=machine.Pin(19),
    miso=machine.Pin(16),
)

time.sleep_ms(100)

# Initialize network interface
nic = network.WIZNET5K(spi, cs, rst)

# Activate the network interface first
nic.active(True)
time.sleep_ms(100)

# Configure static IP: 172.16.1.1 (this device acts as gateway)
nic.ifconfig(("172.16.1.1", "255.255.255.0", "172.16.1.1", "8.8.8.8"))

# Give it a moment to apply settings
time.sleep_ms(500)

debug_print("Waiting for Ethernet link...")
while not nic.isconnected():
    time.sleep(0.1)
debug_print("Connected. IP address:", nic.ifconfig()[0])

# Check if interface is active and configured
if nic.active():
    config_net = nic.ifconfig()
    debug_print(f"Ethernet initialized successfully, IP: {config_net[0]}")
else:
    debug_print("ERROR: Ethernet failed")

print_memory_stats("After network initialization")

# Initialize Camera
try:
    camera.init_cam()
    camera.start_cam()
    debug_print("Camera started")
    print_memory_stats("After Camera Init")
except Exception as e:
    print(f"ERROR: Camera: {e}")


# Cleanup function
def cleanup():
    debug_print("\nCleaning up...")
    global nic
    try:
        # Deactivate network interface
        if nic:
            nic.active(False)
            debug_print("Network interface deactivated")
    except Exception as e:
        print(f"Error during cleanup: {e}")

def create_server_socket():
    """Create, bind and listen on a new server socket"""
    try:
        debug_print("[DIAG] create_server_socket: starting")
        server = socket.socket()
        debug_print("[DIAG] create_server_socket: socket() done")
        server.bind(("0.0.0.0", 80))
        debug_print("[DIAG] create_server_socket: bind() done")
        server.listen(5)
        debug_print("[DIAG] create_server_socket: listen() done")
        debug_print("[MAIN] Server socket created and listening on port 80")
        debug_print("[DIAG] create_server_socket: about to return")
        return server
    except Exception as e:
        print(f"[DIAG] create_server_socket: EXCEPTION: {e}")
        raise

def generate_html_root():
    """path: /"""
    gc.collect()
    html = """HTTP/1.1 200 OK
Content-Type: text/html
Connection: close

<!DOCTYPE html>
<html>
<head>
    <title>Camera Stream</title>
    <style>
        body { margin: 20px; font-family: Arial, sans-serif; background: #222; color: #fff; }
        #camera-canvas { border: 2px solid #0f0; display: block; }
        #stats { margin-top: 10px; font-size: 16px; }
        .metric { display: inline-block; margin-right: 20px; padding: 5px 10px; background: #333; }
        #color-format-options { margin-top: 10px; }
        #color-format-options label { display: block; margin: 5px 0; }
    </style>
</head>
<body>
    <h2>Camera Stream - RGB565</h2>
    <canvas id="camera-canvas" width="240" height="320"></canvas>
    <div id="stats">
        <div class="metric">FPS: <span id="fps">0.00</span></div>
        <div class="metric">Frame: <span id="frame-count">0</span></div>
        <div class="metric">Status: <span id="status">Starting...</span></div>
    </div>
    <div id="color-format-options">
        <strong>Color Format:</strong><br>
        <label><input type="radio" name="color-format" value="bgr"> b01234_g012345_r01234</label>
        <label><input type="radio" name="color-format" value="rgb" checked> r01234_g012345_b01234</label>
        <label><input type="radio" name="color-format" value="grb"> g01234_r012345_b01234</label>
        <label><input type="radio" name="color-format" value="brg"> b01234_r012345_g01234</label>
        <label><input type="radio" name="color-format" value="gbr"> g01234_b012345_r01234</label>
        <label><input type="radio" name="color-format" value="rbg"> r01234_b012345_g01234</label>
    </div>

    <script>
        const canvas = document.getElementById('camera-canvas');
        const ctx = canvas.getContext('2d');
        const width = 240;
        const height = 320;
        const frameSize = width * height * 2; // RGB565 = 2 bytes per pixel

        let frameCount = 0;
        let fpsCounter = 0;
        let lastFpsTime = Date.now();

        // Single-socket guard: only one active request at a time
        let requestInProgress = false;
        let abortController = null;

        function displayImage(arrayBuffer) {
            // Decode bit-packed data (from colorfix implementation)
            const source = new Uint8Array(arrayBuffer);
            const dest = new Uint8Array(source.length);

            /*if (source.length > 0) {
                dest[0] = source[0] >> 2;
                for (let i = 1; i < source.length; i++) {
                    dest[i] = ((source[i - 1] & 3) << 6) | (source[i] >> 2);
                }
            }*/
            const data = source;

            const imageData = ctx.createImageData(width, height);
            const format = document.querySelector('input[name="color-format"]:checked').value;

            for (let i = 0; i < width * height; i++) {
                const byte0 = data[i * 2];
                const byte1 = data[i * 2 + 1];
                const rgb565 = byte0 | (byte1 << 8);

                let r, g, b;

                switch (format) {
                    case 'bgr': // b01234_g012345_r01234
                        b = (rgb565 >> 11) & 0x1F;
                        g = (rgb565 >> 5) & 0x3F;
                        r = rgb565 & 0x1F;
                        break;
                    case 'rgb': // r01234_g012345_b01234
                        r = (rgb565 >> 11) & 0x1F;
                        g = (rgb565 >> 5) & 0x3F;
                        b = rgb565 & 0x1F;
                        break;
                    case 'grb': // g01234_r012345_b01234
                        g = (rgb565 >> 11) & 0x1F;
                        r = (rgb565 >> 5) & 0x3F;
                        b = rgb565 & 0x1F;
                        break;
                    case 'brg': // b01234_r012345_g01234
                        b = (rgb565 >> 11) & 0x1F;
                        r = (rgb565 >> 5) & 0x3F;
                        g = rgb565 & 0x1F;
                        break;
                    case 'gbr': // g01234_b012345_r01234
                        g = (rgb565 >> 11) & 0x1F;
                        b = (rgb565 >> 5) & 0x3F;
                        r = rgb565 & 0x1F;
                        break;
                    case 'rbg': // r01234_b012345_g01234
                        r = (rgb565 >> 11) & 0x1F;
                        b = (rgb565 >> 5) & 0x3F;
                        g = rgb565 & 0x1F;
                        break;
                }

                const r8 = (r << 3) | (r >> 2);
                const g8 = (g << 2) | (g >> 4);
                const b8 = (b << 3) | (b >> 2);

                const finalR = r8;
                const finalG = g8;
                const finalB = b8;

                const idx = i * 4;
                imageData.data[idx] = finalR;
                imageData.data[idx + 1] = finalG;
                imageData.data[idx + 2] = finalB;
                imageData.data[idx + 3] = 255;
            }

            ctx.putImageData(imageData, 0, 0);

            frameCount++;
            fpsCounter++;
            document.getElementById('frame-count').textContent = frameCount;

            const now = Date.now();
            if (now - lastFpsTime >= 1000) {
                const fps = fpsCounter / ((now - lastFpsTime) / 1000);
                document.getElementById('fps').textContent = fps.toFixed(2);
                fpsCounter = 0;
                lastFpsTime = now;
            }
        }

        async function startStream() {
            // Single-socket guard: block concurrent requests
            if (requestInProgress) {
                console.log('Request already in progress, skipping');
                return;
            }

            // Abort any lingering previous request
            if (abortController) {
                abortController.abort();
                await new Promise(r => setTimeout(r, 100));
            }

            abortController = new AbortController();
            requestInProgress = true;

            try {
                document.getElementById('status').textContent = 'Connecting...';
                const response = await fetch('/stream', { signal: abortController.signal });
                document.getElementById('status').textContent = 'Streaming';

                const reader = response.body.getReader();
                let buffer = new Uint8Array(0);
                const boundaryText = '--frame';
                const boundary = new TextEncoder().encode(boundaryText);

                while (true) {
                    const {done, value} = await reader.read();

                    if (done) {
                        console.log('Stream ended');
                        document.getElementById('status').textContent = 'Stream ended';
                        break;
                    }

                    // Append new data to buffer
                    const newBuffer = new Uint8Array(buffer.length + value.length);
                    newBuffer.set(buffer);
                    newBuffer.set(value, buffer.length);
                    buffer = newBuffer;

                    // Process all complete frames in buffer
                    while (true) {
                        // Find boundary marker
                        let boundaryIndex = -1;
                        for (let i = 0; i < buffer.length - boundary.length; i++) {
                            let match = true;
                            for (let j = 0; j < boundary.length; j++) {
                                if (buffer[i + j] !== boundary[j]) {
                                    match = false;
                                    break;
                                }
                            }
                            if (match) {
                                boundaryIndex = i;
                                break;
                            }
                        }

                        if (boundaryIndex === -1) break;

                        // Find data start (after \\r\\n\\r\\n)
                        let dataStart = -1;
                        for (let i = boundaryIndex; i < buffer.length - 3; i++) {
                            if (buffer[i] === 13 && buffer[i+1] === 10 &&
                                buffer[i+2] === 13 && buffer[i+3] === 10) {
                                dataStart = i + 4;
                                break;
                            }
                        }

                        if (dataStart === -1 || buffer.length - dataStart < frameSize) {
                            break; // Not enough data yet
                        }

                        // Extract and display frame
                        const frameData = buffer.slice(dataStart, dataStart + frameSize);
                        displayImage(frameData);

                        // Remove processed data
                        buffer = buffer.slice(dataStart + frameSize);
                    }
                }
            } catch (err) {
                if (err.name === 'AbortError') {
                    console.log('Request aborted');
                    document.getElementById('status').textContent = 'Aborted';
                } else {
                    console.error('Stream error:', err);
                    document.getElementById('status').textContent = 'Error: ' + err.message;
                }
            } finally {
                // Cleanup: allow next request after delay for MCU socket teardown
                requestInProgress = false;
                abortController = null;
                document.getElementById('status').textContent = 'Reconnecting...';
                // Wait 500ms for MCU to fully close socket before reconnecting
                setTimeout(startStream, 500);
            }
        }

        // Start streaming
        startStream();
    </script>
</body>
</html>
"""
    return html.encode()

def generate_html_404():
    """path: /404"""
    html = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nConnection: close\r\nKeep-Alive: timeout=0, max=0\r\n\r\n404 Not Found"
    return html.encode()


# Initial server socket
s = create_server_socket()

while True:
    try:
        debug_print("[DIAG] Top of main loop")
        gc.collect()
        debug_print("[DIAG] gc.collect() done")
        
        response = generate_html_404()
        debug_print("[DIAG] About to call s.accept()")
        debug_print("[MAIN] ====== WAITING FOR CONNECTION ======")
        cl, addr = s.accept()
        debug_print("[DIAG] s.accept() returned")
       
        request = cl.recv(8192)
        debug_print(f"[MAIN] recv() returned {len(request)} bytes")

        request_str = request.decode("utf-8")
        request_lines = request_str.split('\r\n')
        debug_print(f"[MAIN] Request split into {len(request_lines)} lines")

        request_line = request_lines[0]
        debug_print(f"[MAIN] First line: {request_line[:50]}...")

        parts = request_line.split()
        debug_print(f"[MAIN] Split into {len(parts)} parts")

        if len(parts) > 1:
            method, path = parts[0], parts[1]
            debug_print(f"[MAIN] Parsed: method={method}, path={path}")
        else:
            path = '/'
            debug_print("[MAIN] Using default path=/")

        debug_print(f"[MAIN] Request path: {path}")

        # Generate response
        if path == '/stream':
            # PYTHON-BASED STREAMING
            try:
                header = b"HTTP/1.1 200 OK\r\n"
                header += b"Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
                header += b"Cache-Control: no-cache\r\n"
                header += b"Connection: keep-alive\r\n"
                header += b"\r\n"
                cl.send(header)
                debug_print("Stream client connected - header sent")
            except OSError as e:
                print(f"Failed to send header: {e}")
                cl.close()
                continue

            # Streaming loop entirely in Python
            # Use dict for state (MicroPython nonlocal workaround)
            state = {'streaming': True, 'frame_count': 0, 'first_frame': True}
            boundary = b"--frame\r\nContent-Type: application/octet-stream\r\n\r\n"
            frame_boundary = b"\r\n--frame\r\nContent-Type: application/octet-stream\r\n\r\n"
            
            cl.settimeout(5.0)
            
            def send_frame_data(frame_data):
                try:
                    # Send boundary
                    if state['first_frame']:
                        cl.send(boundary)
                        state['first_frame'] = False
                    else:
                        cl.send(frame_boundary)
                    
                    # Send frame in chunks
                    mv = memoryview(frame_data)
                    total_sent = 0
                    chunk_size = 16384  # 16KB chunks
                    
                    while total_sent < len(frame_data):
                        end = min(total_sent + chunk_size, len(frame_data))
                        chunk = mv[total_sent:end]
                        try:
                            bytes_sent = cl.send(chunk)
                            if bytes_sent == 0:
                                state['streaming'] = False
                                return
                            total_sent += bytes_sent
                        except OSError as e:
                            debug_print(f"Stream send error: {e}")
                            state['streaming'] = False
                            return
                    
                    state['frame_count'] += 1
                    if state['frame_count'] % 30 == 0:
                        debug_print(f"Streamed {state['frame_count']} frames")
                        
                except OSError as e:
                    debug_print(f"Stream error: {e}")
                    state['streaming'] = False
            
            debug_print("Starting Python-based streaming loop")
            while state['streaming']:
                try:
                    if camera.is_buffer_ready():
                        camera.send_frame_over_eth(send_frame_data)
                except OSError as e:
                    debug_print(f"Streaming loop error: {e}")
                    state['streaming'] = False
                except KeyboardInterrupt:
                    state['streaming'] = False
            
            debug_print(f"Stream ended after {state['frame_count']} frames")
            try:
                cl.close()
            except:
                pass
            # Recreate server socket
            s.close()
            time.sleep_ms(100)
            gc.collect()
            s = create_server_socket()
            continue

        elif path == "/":
            debug_print("[MAIN] / html stream")
            response = generate_html_root()
        else:
            # 404 for unknown paths
            debug_print("[MAIN] Calling generate_html_404()")
            response = generate_html_404()
        
        response_start = time.ticks_ms()
        response_time = time.ticks_diff(time.ticks_ms(), response_start)
            
            
        debug_print(f"Response generation time: {response_time}ms")
        debug_print("[MAIN] cl.send() is about to be executed")
        cl.send(response)

        debug_print("[MAIN] Calling cl.close()")
        cl.close()
        debug_print("[MAIN] close() returned successfully")

        # CRITICAL: Close the server socket and create a new one
        debug_print("[MAIN] Closing server socket and creating new one...")
        s.close()
        time.sleep_ms(100)
        gc.collect()
        s = create_server_socket()
        debug_print("[MAIN] New server socket ready")
        del response

    except Exception as e:
        print(f"[DIAG] Exception caught: {e}")
        print(f"[DIAG] Exception type: {type(e)}")
        if hasattr(e, 'errno'):
            print(f"[DIAG] Exception errno: {e.errno}")
        print("Error:", e)
        # Try to recover by creating a new server socket
        try:
            print("[DIAG] Exception handler: calling s.close()")
            s.close()
            print("[DIAG] Exception handler: s.close() done")
        except Exception as e2:
            print(f"[DIAG] Exception handler: s.close() failed: {e2}")
            pass
        time.sleep_ms(500)
        gc.collect()
        print("[DIAG] Exception handler: calling create_server_socket()")
        s = create_server_socket()
        print("[DIAG] Exception handler: create_server_socket() done")
