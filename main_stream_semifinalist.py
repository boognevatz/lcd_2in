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
import endpoints

DEBUG = False

endpoints.set_debug(DEBUG)

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
    import ov5640_i2c
    ov5640_i2c.init_cam(format="jpeg", resolution="vga", test_pattern=False)
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
    # Initialize barometer (WF5803F at 0x6C)
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

# Initial server socket
s = create_server_socket()

while True:
    try:
        debug_print("[DIAG] Top of main loop")
        gc.collect()
        debug_print("[DIAG] gc.collect() done")
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
        response, s, should_continue = endpoints.handle_request(
            path, cl, s, create_server_socket, set_head_led_brightness
        )
        if should_continue:
            continue
        response_start = time.ticks_ms()
        response_time = time.ticks_diff(time.ticks_ms(), response_start)
        debug_print(f"Response generation time: {response_time}ms")
        debug_print("[MAIN] cl.send() is about to be executed")
        if response:
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
