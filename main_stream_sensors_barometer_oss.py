"""
Main streaming script with barometer integration.
"""

import machine
import network
import socket
import time
import camera
import gc
import tempsensor
import barometer

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

# Initialize MCP4725 DAC for head LED brightness control
MCP4725_ADDR = 0x60
dac_ready = False
try:
    i2c = machine.I2C(0, sda=machine.Pin(12), scl=machine.Pin(13), freq=400000)
    devices = i2c.scan()
    if MCP4725_ADDR in devices:
        dac_ready = True
        debug_print(f"MCP4725 DAC found at 0x{MCP4725_ADDR:02x}")
    else:
        print(f"MCP4725 not found (scanned: {[hex(d) for d in devices]})")
    # Initialize tempsensor
    tempsensor.adc_ready = (0x48 in devices)
    if tempsensor.adc_ready:
        tempsensor.i2c = i2c
        tempsensor.ADS7830_ADDR = 0x48
        tempsensor.ADC_REF_VOLTAGE = 2.5
        tempsensor.NTC_R0 = 10000
        tempsensor.NTC_BETA = 3950
        tempsensor.NTC_T0 = 298.15
        tempsensor.DIVIDER_R = 10000
        tempsensor.DIVIDER_VCC = 3.3
        tempsensor.warm_up_adc()
        debug_print("ADS7830 ADC initialized for temperature sensors")
    else:
        debug_print("WARNING: ADS7830 ADC not found, external temperatures will be unavailable")
    # Initialize barometer (placeholder address 0x5D, commonly used for BMP280/BME280)
    BAROMETER_POSSIBLE_ADDRS = [0x6C, 0x6D, 0x5D, 0x76, 0x77]
    barometer_addr = None
    for addr in BAROMETER_POSSIBLE_ADDRS:
        if addr in devices:
            barometer_addr = addr
            break
    if barometer_addr is not None:
        barometer.barometer_ready = True
        barometer.i2c = i2c
        barometer.BAROMETER_ADDR = barometer_addr
        # Placeholder calibration values – these should be set according to your sensor's datasheet
        barometer.BAROMETER_ATMOSPHERIC_RAW = 5600000
        barometer.BAROMETER_ATMOSPHERIC_BAR = 1.01325
        barometer.init_barometer()
        debug_print(f"Barometer found at 0x{barometer_addr:02x}")
    else:
        barometer.barometer_ready = False
        debug_print("Barometer not detected")
except Exception as e:
    print(f"ERROR: I2C init: {e}")


def set_head_led_brightness(percent):
    """Set LED brightness using MCP4725 DAC (0-100%)."""
    global dac_ready
    if not dac_ready:
        return False
    if percent < 0:
        percent = 0
    elif percent > 100:
        percent = 100
    try:
        dac_value = int((percent / 100.0) * 4095)
        msb = (dac_value >> 4) & 0xFF
        lsb = (dac_value << 4) & 0xF0
        i2c.writeto(MCP4725_ADDR, bytes([0x40, msb, lsb]))
        return True
    except Exception as e:
        print(f"ERROR: LED brightness: {e}")
        return False

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
        #controls { margin-top: 10px; }
        #controls button { padding: 8px 16px; margin-right: 10px; font-size: 14px; cursor: pointer; }
        #btn-stream { background: #c33; color: #fff; border: none; }
        #btn-stream.stopped { background: #3a3; }
        .btn-led { background: #555; color: #fff; border: none; }
        .btn-led:disabled { opacity: 0.4; cursor: not-allowed; }
        .btn-led.active { background: #c90; }
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
    <div id="controls">
        <button id="btn-stream" onclick="toggleStream()">Stop Stream</button>
        <button class="btn-led" onclick="sendLedCommand(0)">LED 0%</button>
        <button class="btn-led" onclick="sendLedCommand(8)">LED 8%</button>
        <button class="btn-led" onclick="sendLedCommand(10)">LED 10%</button>
        <button class="btn-led" onclick="sendLedCommand(15)">LED 15%</button>
        <button class="btn-led" onclick="sendLedCommand(20)">LED 20%</button>
        <button class="btn-led" onclick="sendLedCommand(100)">LED 100%</button>
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
        // JavaScript omitted for brevity
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
            state = {'streaming': True, 'frame_count': 0, 'first_frame': True}
            boundary = b"--frame\r\nContent-Type: application/octet-stream\r\n\r\n"
            frame_boundary = b"\r\n--frame\r\nContent-Type: application/octet-stream\r\n\r\n"
            cl.settimeout(5.0)
            def send_frame_data(frame_halves):
                try:
                    if state['first_frame']:
                        cl.send(boundary)
                        state['first_frame'] = False
                    else:
                        cl.send(frame_boundary)
                    chunk_size = 16384  # 16KB chunks
                    for frame_data in frame_halves:
                        mv = memoryview(frame_data)
                        total_sent = 0
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
                    if camera.is_frame_ready():
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
        elif path == '/streamc':
            # C streaming with optional temperature and barometer data
            # Gather optional sensor data for headers
            baro_header = b""
            if barometer.barometer_ready:
                try:
                    temp_c, pressure_bar, _p_raw = barometer.read_barometer()
                    if temp_c is not None and pressure_bar is not None:
                        baro_header = f"X-barometer: {temp_c}C,{pressure_bar}bar".encode()
                except Exception:
                    pass
            try:
                header = b"HTTP/1.1 200 OK\r\n"
                header += b"Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
                header += b"Cache-Control: no-cache\r\n"
                header += b"Connection: keep-alive\r\n"
                if baro_header:
                    header += baro_header + b"\r\n"
                header += b"\r\n"
                cl.send(header)
                debug_print("Stream client connected - header sent (C streaming)")
            except OSError as e:
                print(f"Failed to send header: {e}")
                cl.close()
                continue
            cl.settimeout(5.0)
            # Temperature reading setup
            temp_sensor = machine.ADC(4)
            VREF = 3.3
            BATCH_SIZE = 100
            debug_print("Starting C-based streaming loop")
            camera.stream_start()
            total_frames = 0
            while True:
                sent = camera.stream_loop_c(cl, BATCH_SIZE)
                total_frames += sent
                if sent < BATCH_SIZE:
                    break
                # Read MCU temperature between batches (~50us, invisible)
                raw = temp_sensor.read_u16()
                voltage = raw * VREF / 65535.0
                temp_c = 27.0 - ((voltage - 0.706) / 0.001721)
                camera.set_temperature(int(temp_c * 10))
                # External temperatures
                if tempsensor.adc_ready:
                    ext = tempsensor.read_all_temperatures()
                    def fmt(entry):
                        if entry is None:
                            return -999
                        t = entry.get("temp_c")
                        return t if t is not None else -999
                    camera.set_ext_temperatures(
                        fmt(ext.get("headTemp1")),
                        fmt(ext.get("headTemp2")),
                        fmt(ext.get("headTemp3")),
                        fmt(ext.get("headTemp4")),
                        fmt(ext.get("headTemp5")),
                        fmt(ext.get("headTemp6"))
                    )
                else:
                    camera.set_ext_temperatures(-999, -999, -999, -999, -999, -999)
                
                # Read Barometer
                if barometer.barometer_ready:
                    try:
                        baro_t, baro_p, _baro_raw = barometer.read_barometer()
                        b_t = int(baro_t) if baro_t is not None else -999
                        b_p = int(baro_p * 1000) if baro_p is not None else 0
                        camera.set_barometer(b_t, b_p)
                    except Exception:
                        camera.set_barometer(-999, 0)
                else:
                    camera.set_barometer(-999, 0)
                    
            debug_print(f"Stream ended after {total_frames} frames")
            try:
                cl.close()
            except:
                pass
            s.close()
            time.sleep_ms(100)
            gc.collect()
            s = create_server_socket()
            continue
        elif path == "/":
            debug_print("[MAIN] / html stream")
            response = generate_html_root()
        elif path.startswith('/headled/'):
            # /headled/30 -> set LED to 30%
            try:
                percent = int(path.split('/')[-1])
                ok = set_head_led_brightness(percent)
                if ok:
                    body = f"OK {percent}%"
                else:
                    body = "DAC not ready"
            except ValueError:
                body = "Bad value"
            response = f"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\nContent-Length: {len(body)}\r\n\r\n{body}".encode()
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
