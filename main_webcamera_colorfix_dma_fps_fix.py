### ETHERNET WEBSERVER WITH W5500 AND CAMERA - OPTIMIZED NON-BLOCKING
import machine
import network
import socket
import time
import camera
import select
import json
import gc

DEBUG = False

if DEBUG:
    debug_print = print
else:
    def debug_print(*args, **kwargs):
        _ = args, kwargs
        pass


# ===== WATCHDOG TIMER SETUP =====
# Initialize watchdog timer to prevent system freezes
# If the system doesn't call wdt.feed() within 8 seconds, it will reset
#wdt = machine.WDT(timeout=8000)  # 8 second timeout
debug_print("Watchdog initialized with 8s timeout")
# ================================


# ===== LOAD CONFIGURATION =====
def load_config():
    """Load configuration from config.json file"""
    try:
        with open("config.json", "r") as f:
            config = json.load(f)
            return config
    except Exception as e:
        print(f"ERROR: Failed to load config.json: {e}")
        # Return default config
        return {
            "headID": 1117,
            "i2c": {
                "adc_address": "0x48",
                "barometer_address": "0x6C",
                "dac_address": "0x60",
            },
            "adc": {"ref_voltage": 2.5},
            "ntc": {
                "r0": 10000,
                "beta": 3380,
                "t0_kelvin": 298.15,
                "divider_r": 10000,
                "divider_vcc": 3.3,
            },
            "barometer": {"atmospheric_raw": 5463198, "atmospheric_bar": 1.0},
        }


config = load_config()

# Feed watchdog during initialization
#wdt.feed()

# Main configuration
HEAD_ID = config.get("headID", 1117)

# I2C addresses (convert hex strings to integers)
ADS7830_ADDR = int(config.get("i2c", {}).get("adc_address", "0x48"), 16)
BAROMETER_ADDR = int(config.get("i2c", {}).get("barometer_address", "0x6C"), 16)
MCP4725_ADDR = int(config.get("i2c", {}).get("dac_address", "0x60"), 16)

# ADC configuration
ADC_REF_VOLTAGE = config.get("adc", {}).get("ref_voltage", 2.5)

# NTC Thermistor configuration
NTC_R0 = config.get("ntc", {}).get("r0", 10000)
NTC_BETA = config.get("ntc", {}).get("beta", 3380)
NTC_T0 = config.get("ntc", {}).get("t0_kelvin", 298.15)
DIVIDER_R = config.get("ntc", {}).get("divider_r", 10000)
DIVIDER_VCC = config.get("ntc", {}).get("divider_vcc", 3.3)

# WF5803F Barometer calibration
BAROMETER_ATMOSPHERIC_RAW = config.get("barometer", {}).get("atmospheric_raw", 5463198)
BAROMETER_ATMOSPHERIC_BAR = config.get("barometer", {}).get("atmospheric_bar", 1.0)
# =========================

# Record boot time for uptime calculation
boot_time_ms = time.ticks_ms()

# Initialize I2C for ADS7830 ADC, WF5803F Barometer, and MCP4725 DAC
i2c = machine.I2C(0, scl=machine.Pin(13), sda=machine.Pin(12), freq=100000)

# Check if devices are present
adc_ready = False
barometer_ready = False
dac_ready = False
try:
    devices = i2c.scan()

    if ADS7830_ADDR in devices:
        adc_ready = True
    else:
        debug_print("WARNING: ADS7830 ADC not found")

    if BAROMETER_ADDR in devices:
        barometer_ready = True
    else:
        debug_print("WARNING: WF5803F Barometer not found")

    if MCP4725_ADDR in devices:
        dac_ready = True
    else:
        debug_print("WARNING: MCP4725 DAC not found")
except Exception as e:
    print(f"ERROR: I2C initialization failed: {e}")
    adc_ready = False
    barometer_ready = False
    dac_ready = False

# Feed watchdog after I2C initialization
#wdt.feed()


def read_adc_channel(channel):
    """
    Read a single channel from ADS7830

    Args:
        channel: Channel number (0-7)

    Returns:
        Voltage value in volts, or None if read fails
    """
    if not adc_ready:
        return None

    try:
        # Command byte format for ADS7830:
        # Bit 7: SD=1 (single-ended mode)
        # Bits 6-4: C2-C1-C0 (channel select)
        # Bits 3-2: PD1-PD0=11 (internal reference ON, ADC ON between conversions)
        # Bits 1-0: Don't care
        command = 0x8C | (channel << 4)  # 0x8C = 10001100

        # Write command byte
        i2c.writeto(ADS7830_ADDR, bytes([command]))

        # Delay for conversion (datasheet: typ 32µs, max 64µs)
        time.sleep_ms(2)

        # Read result (8-bit value)
        data = i2c.readfrom(ADS7830_ADDR, 1)
        adc_value = data[0]

        # Convert to voltage (8-bit ADC, 0-255 maps to 0-2.5V)
        voltage = (adc_value / 255.0) * ADC_REF_VOLTAGE

        return voltage
    except Exception as e:
        debug_print(f"Error reading ADC channel {channel}: {e}")
        return None


def voltage_to_temperature(voltage):
    """
    Convert voltage reading to temperature in Celsius using NTC thermistor

    Voltage divider configuration (per schematic):
    3.3V --- R_fixed (10k) --- V_out (measured) --- R_NTC --- GND

    V_out = VCC * R_NTC / (R_fixed + R_NTC)
    Therefore: R_NTC = (V_out * R_fixed) / (VCC - V_out)

    Args:
        voltage: Measured voltage in volts

    Returns:
        Temperature in Celsius (integer), or None if calculation fails
    """
    if voltage is None:
        return None

    # Check for invalid voltage range (with some tolerance)
    if voltage < 0.01 or voltage > (DIVIDER_VCC - 0.01):
        return None

    try:
        # Calculate thermistor resistance from voltage divider
        # NTC is on bottom (connected to GND), R_fixed is on top (connected to 3.3V)
        # R_NTC = (V_out * R_fixed) / (VCC - V_out)
        r_ntc = (voltage * DIVIDER_R) / (DIVIDER_VCC - voltage)

        # Beta equation: 1/T = 1/T0 + (1/B) * ln(R/R0)
        import math

        temp_kelvin = 1.0 / (1.0 / NTC_T0 + (1.0 / NTC_BETA) * math.log(r_ntc / NTC_R0))
        temp_celsius = temp_kelvin - 273.15

        # Return as integer (no decimals)
        return int(round(temp_celsius))
    except Exception as e:
        print(f"Error converting voltage to temperature: {e}")
        return None


def get_uptime_seconds():
    """
    Get uptime since boot in seconds

    Returns:
        Uptime in seconds (integer)
    """
    current_time_ms = time.ticks_ms()
    uptime_ms = time.ticks_diff(current_time_ms, boot_time_ms)
    uptime_sec = uptime_ms // 1000
    return uptime_sec


def get_mcu_temperature():
    """
    Read RP2350A internal temperature sensor

    Returns:
        Temperature in Celsius (integer), or None if read fails
    """
    try:
        # ADC4 is connected to the internal temperature sensor
        adc_temp = machine.ADC(4)

        # Read raw ADC value (16-bit: 0-65535)
        adc_value = adc_temp.read_u16()

        # Convert to voltage (3.3V reference)
        voltage = (adc_value / 65535.0) * 3.3

        # RP2040/RP2350 temperature sensor formula:
        # T = 27 - (ADC_voltage - 0.706) / 0.001721
        temp_celsius = 27 - (voltage - 0.706) / 0.001721

        # Return as integer (no decimals)
        return int(round(temp_celsius))
    except Exception as e:
        print(f"Error reading MCU temperature: {e}")
        return None


def init_barometer():
    """
    Initialize the WF5803F 01BA barometer sensor
    Model: WF5803F 01BA L8 DT
    Range: 30kPa - 1.1bar (0.3 - 1.1 bar)
    TCO: 1.5 Pa/K

    CMD register: 0x30
    CTRL register: 0x02

    Returns:
        True if initialization successful, False otherwise
    """
    if not barometer_ready:
        return False

    try:
        return True
    except Exception as e:
        print(f"ERROR: Barometer init: {e}")
        return False


def read_barometer():
    """
    Read temperature and pressure from WF5803F 01BA barometer
    Model: WF5803F 01BA L8 DT
    Range: 30-110 kPa (0.3-1.1 bar)

    Protocol based on working C implementation:
    - CMD register 0x30: Write 0x0A (0x02 + SCO bit) to start measurement
    - CTRL register 0x02: Check bit 0 (DRDY) for data ready
    - Pressure: 24-bit signed at 0x06, 0x07, 0x08 (MSB first)
    - Temperature: 16-bit at 0x09, 0x0A (MSB first)

    Returns:
        Tuple of (temperature_celsius, pressure_bar) or (None, None) if read fails
    """
    if not barometer_ready:
        return (None, None)

    try:
        # Start measurement: Write 0x0A to CMD register (0x30)
        # 0x0A = 0x02 (measurement mode) + 0x08 (SCO bit)
        SCO_BITMASK = 0x08
        DRDY_BITMASK = 0x01

        i2c.writeto_mem(BAROMETER_ADDR, 0x30, bytes([0x02 | SCO_BITMASK]))

        # Wait for data ready (DRDY bit in control register 0x02 goes to 0)
        delay_count = 0
        while delay_count < 50:
            time.sleep_ms(1)
            ctrl_reg = i2c.readfrom_mem(BAROMETER_ADDR, 0x02, 1)[0]
            if not (ctrl_reg & DRDY_BITMASK):
                break
            delay_count += 1

        if delay_count >= 50:
            debug_print("Barometer measurement timeout")
            return (None, None)

        # Read pressure (24-bit signed, MSB first) from registers 0x06, 0x07, 0x08
        press_byte0 = i2c.readfrom_mem(BAROMETER_ADDR, 0x06, 1)[0]
        press_byte1 = i2c.readfrom_mem(BAROMETER_ADDR, 0x07, 1)[0]
        press_byte2 = i2c.readfrom_mem(BAROMETER_ADDR, 0x08, 1)[0]

        # Combine bytes (MSB first)
        pressure_raw = (press_byte0 << 16) | (press_byte1 << 8) | press_byte2

        # Sign extend 24-bit to 32-bit if negative (bit 23 is set)
        if pressure_raw & 0x800000:
            pressure_raw |= 0xFF000000
            # Convert to signed integer
            pressure_raw = pressure_raw - 0x100000000

        # Read temperature (16-bit, MSB first) from registers 0x09, 0x0A
        temp_byte0 = i2c.readfrom_mem(BAROMETER_ADDR, 0x09, 1)[0]
        temp_byte1 = i2c.readfrom_mem(BAROMETER_ADDR, 0x0A, 1)[0]

        # Combine bytes (MSB first)
        temp_raw = (temp_byte0 << 8) | temp_byte1

        # Sign extend if negative
        if temp_raw > 32767:
            temp_raw -= 65536

        # Convert raw values to actual units
        # Temperature: raw / 256.0 (from datasheet specification)
        temperature_celsius = temp_raw / 256.0

        # Calculate scale factor (bar per count) from calibration constants
        PRESSURE_SCALE_FACTOR = BAROMETER_ATMOSPHERIC_BAR / BAROMETER_ATMOSPHERIC_RAW

        # Convert raw count to absolute pressure
        pressure_absolute_bar = pressure_raw * PRESSURE_SCALE_FACTOR

        # Convert absolute pressure to gauge pressure (relative to atmosphere)
        pressure_gauge_bar = pressure_absolute_bar - BAROMETER_ATMOSPHERIC_BAR

        # Clamp to minimum 0.0 (no negative pressure readings)
        if pressure_gauge_bar < 0.0:
            pressure_gauge_bar = 0.0

        # Return as integer for temperature, 3 decimals for pressure
        return (int(round(temperature_celsius)), round(pressure_gauge_bar, 3))

    except Exception as e:
        print(f"Error reading barometer: {e}")
        return (None, None)


def init_head_led():
    """
    Initialize MCP4725 DAC for LED brightness control
    Set to 0% brightness (off) on startup

    Returns:
        True if initialization successful, False otherwise
    """
    if not dac_ready:
        return False

    try:
        # Initialize DAC to 0 (LED off)
        # Command: 0x60 (write DAC and EEPROM), data: 0x00 0x00
        i2c.writeto(MCP4725_ADDR, bytes([0x60, 0x00, 0x00]))
        time.sleep_ms(10)
        return True
    except Exception as e:
        print(f"ERROR: LED DAC init: {e}")
        return False


def set_head_led_brightness(percent):
    """
    Set LED brightness using MCP4725 DAC

    Args:
        percent: Brightness level 0-100 (integer percentage)

    Returns:
        True if successful, False otherwise
    """
    if not dac_ready:
        return False

    # Validate and clamp input
    if percent < 0:
        percent = 0
    elif percent > 100:
        percent = 100

    try:
        # Convert percentage to 12-bit DAC value (0-4095)
        dac_value = int((percent / 100.0) * 4095)

        # Split into MSB and LSB
        # MCP4725 format: MSB = upper 8 bits, LSB = lower 4 bits in upper nibble
        msb = (dac_value >> 4) & 0xFF
        lsb = (dac_value << 4) & 0xF0

        # Write to DAC (fast write mode)
        # Command: 0x40 (write DAC register only, not EEPROM)
        i2c.writeto(MCP4725_ADDR, bytes([0x40, msb, lsb]))

        return True
    except Exception as e:
        print(f"ERROR: LED brightness: {e}")
        return False


def read_all_led_temps():
    """
    Read all 6 temperature channels and convert to Celsius

    Returns:
        Dictionary with headTemp1 through headTemp6 values in Celsius (integers)
    """
    temps = {}
    for i in range(6):
        voltage = read_adc_channel(i)
        temp_c = voltage_to_temperature(voltage)
        temps[f"headTemp{i + 1}"] = temp_c
    return temps


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

# ===== UNIFIED SERVER (SINGLE CORE, NON-BLOCKING) =====


# Initialize barometer if present
if barometer_ready:
    if not init_barometer():
        debug_print("ERROR: Barometer initialization failed")
        barometer_ready = False

# Initialize LED DAC if present
if dac_ready:
    if not init_head_led():
        debug_print("ERROR: LED DAC initialization failed")
        dac_ready = False

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

# Initialize SPI for W5500 - can use higher speed with optimizations
spi = machine.SPI(
    0,
    baudrate=80000000,  # 2->20->80 MHz
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

# Feed watchdog after network initialization
#wdt.feed()

# Initialize Camera
camera_ready = False
try:
    camera.init_cam()
    camera.start_cam()
    debug_print("Camera started")

    print_memory_stats("After Camera Init")
except Exception as e:
    print(f"ERROR: Camera: {e}")

# Feed watchdog after camera initialization
#wdt.feed()


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

def generate_html_root(url):
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

            if (source.length > 0) {
                dest[0] = source[0] >> 2;
                for (let i = 1; i < source.length; i++) {
                    dest[i] = ((source[i - 1] & 3) << 6) | (source[i] >> 2);
                }
            }
            const data = dest;

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

                const finalR = Math.round(b8 * 0.85);
                const finalG = Math.round(r8 * 0.85);
                const finalB = Math.round(g8 * 0.85);

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
                const response = await fetch('__STREAM_URL__', { signal: abortController.signal });
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
    html = html.replace("__STREAM_URL__", url)
    return html.encode()

def generate_html_poll():
    """path: /poll"""
    
    html = """HTTP/1.1 200 OK
Content-Type: text/html
Connection: close
Keep-Alive: timeout=0, max=0

<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Camera polling</title>
    <!-- no favicon.ico -->
    <link rel="icon" href="data:,">
</head>
<body>
    <canvas id="camera-canvas" width="240" height="320"></canvas>
    <div id="color-format-options">
        <label><input type="radio" name="color-format" value="bgr" onclick="refreshImage()"> b01234_g012345_r01234</label><br>
        <label><input type="radio" name="color-format" value="rgb" checked onclick="refreshImage()"> r01234_g012345_b01234</label><br>
        <label><input type="radio" name="color-format" value="grb" onclick="refreshImage()"> g01234_r012345_b01234</label><br>
        <label><input type="radio" name="color-format" value="brg" onclick="refreshImage()"> b01234_r012345_g01234</label><br>
        <label><input type="radio" name="color-format" value="gbr" onclick="refreshImage()"> g01234_b012345_r01234</label><br>
        <label><input type="radio" name="color-format" value="rbg" onclick="refreshImage()"> r01234_b012345_g01234</label><br>
    </div>
    <script>
        const canvas = document.getElementById('camera-canvas');
        const ctx = canvas.getContext('2d');
        const width = 240;
        const height = 320;
        let lastImageBuffer = null;

        // Single-socket guard: only one active request at a time
        let requestInProgress = false;
        
        function displayImage(arrayBuffer) {
            const source = new Uint8Array(arrayBuffer);
            const dest = new Uint8Array(source.length);

            if (source.length > 0) {
                dest[0] = source[0] >> 2;
                for (let i = 1; i < source.length; i++) {
                    dest[i] = ((source[i - 1] & 3) << 6) | (source[i] >> 2);
                }
            }
            const data = dest;
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
                
                let finalR = Math.round(b8 * 0.85);
                let finalG = Math.round(r8 * 0.85);
                let finalB = Math.round(g8 * 0.85);
                
                const idx = i * 4;
                imageData.data[idx] = finalR;
                imageData.data[idx + 1] = finalG;
                imageData.data[idx + 2] = finalB;
                imageData.data[idx + 3] = 255;
            }
            
            ctx.putImageData(imageData, 0, 0);
        }
        
        async function refreshImage() {
            // Single-socket guard: block concurrent requests
            if (requestInProgress) {
                console.log('Request in progress, skipping');
                return;
            }

            requestInProgress = true;

            try {
                const response = await fetch('/image.raw?t=' + Date.now());
                const arrayBuffer = await response.arrayBuffer();
                displayImage(arrayBuffer);
            } catch (err) {
                console.error('Image fetch error:', err);
            } finally {
                // Cleanup: allow next request after delay for MCU socket teardown
                requestInProgress = false;
                // Chain next request after current completes + 100ms delay
                setTimeout(refreshImage, 5000);
            }
        }
        
        // Start the sequential polling chain (no setInterval!)
        window.addEventListener('load', () => {
            setTimeout(refreshImage, 15000);
        });
    </script>
</body>
</html>
"""
    return html.encode()

def generate_html_404():
    """path: /404"""
    html = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nConnection: close\r\nKeep-Alive: timeout=0, max=0\r\n\r\n404 Not Found"
    return html.encode()


def handle_json_request(path):
    """Handle JSON requests - serve JSON status data or handle LED control"""
    try:

        http_response = "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\nConnection: close\r\nKeep-Alive: timeout=0, max=0\r\n\r\n"
        # Handle LED brightness control: /headled/{value}
        if path.startswith("/headled/"):
            try:
                # Extract brightness value from path
                value_str = path.split("/headled/")[1]
                brightness = int(value_str)

                # Set LED brightness
                if set_head_led_brightness(brightness):
                    response = f'{{"status": "ok", "brightness": {brightness}}}'
                    http_response = (
                        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nKeep-Alive: timeout=0, max=0\r\n\r\n"
                        + response
                    )
                else:
                    response = '{"status": "error", "message": "DAC not available"}'
                    http_response = (
                        "HTTP/1.1 503 Service Unavailable\r\nContent-Type: application/json\r\nConnection: close\r\nKeep-Alive: timeout=0, max=0\r\n\r\n"
                        + response
                    )

            except (ValueError, IndexError):
                response = '{"status": "error", "message": "Invalid brightness value"}'
                http_response = (
                    "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\nConnection: close\r\nKeep-Alive: timeout=0, max=0\r\n\r\n"
                    + response
                )

        # Handle status JSON: /
        elif path == "/":
            # Get uptime
            uptime = get_uptime_seconds()

            # Get MCU temperature
            mcu_temp = get_mcu_temperature()

            # Read barometer
            cooling_temp, cooling_press = read_barometer()

            # Read LED temperatures from ADC
            led_temps = read_all_led_temps()

            # Build JSON response manually
            json_parts = [f'"headID": {HEAD_ID}', f'"upTime": {uptime}']

            # Add MCU temperature
            if mcu_temp is not None:
                json_parts.append(f'"mcuTemp": {mcu_temp}')
            else:
                json_parts.append('"mcuTemp": null')

            # Add cooling temperature and pressure
            if cooling_temp is not None:
                json_parts.append(f'"cooling_temperature": {cooling_temp}')
            else:
                json_parts.append('"cooling_temperature": null')

            if cooling_press is not None:
                json_parts.append(f'"cooling_pressure": {cooling_press}')
            else:
                json_parts.append('"cooling_pressure": null')

            # Add LED temperatures in order (1-6)
            for i in range(1, 7):
                key = f"headTemp{i}"
                value = led_temps.get(key)
                if value is not None:
                    json_parts.append(f'"{key}": {value}')
                else:
                    json_parts.append(f'"{key}": null')

            json_data = "{" + ", ".join(json_parts) + "}"

            # Send JSON response
            http_response = (
                "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nKeep-Alive: timeout=0, max=0\r\n\r\n"
                + json_data
            )

        else:
            # 404 for unknown paths
            response = '{"status": "error", "message": "Not found"}'
            http_response = (
                "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\nConnection: close\r\nKeep-Alive: timeout=0, max=0\r\n\r\n"
                + response
            )
        return http_response.encode()

    except Exception as e:
        print(f"JSON handler error: {e}")


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
        if path.startswith("/json"):
            debug_print("[MAIN] Handle JSON request")
            adjusted_path = path[5:] if len(path) > 5 else "/"
            response = handle_json_request(adjusted_path)
        elif path.startswith("/poll"):
            debug_print("[MAIN] Handle camera request")
            # Serve minimal HTML page with just the camera view
            response = generate_html_poll()
        elif path == '/stream':
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
                    
                    # Send frame in chunks (like /image.raw does)
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
                    #time.sleep_ms(33)  # ~30fps
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
            # C-BASED STREAMING - uses C function to send frame data
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

            # Streaming loop using C function for frame data sending
            # NOTE: state dict kept for backward compatibility (used in debug print)
            state = {'streaming': True, 'frame_count': 0, 'first_frame': True}
            
            cl.settimeout(5.0)
            
            debug_print("Starting C-based streaming loop")
            # Port to C: the while loop is now in camera.stream_loop_c()
            # It returns frame_count when stream ends (disconnect or error)
            frame_count = camera.stream_loop_c(cl)
            state['frame_count'] = frame_count
            
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
        elif path.startswith("/image.raw"):
            debug_print("Serve raw camera image data")
            if camera.is_buffer_ready():
                debug_print ("Send HTTP header first")
                frame_len = 240 * 320 * 2  # Known size
                header = f"HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: {frame_len}\r\nConnection: close\r\nKeep-Alive: timeout=0, max=0\r\n\r\n"
                cl.send(header.encode())
                debug_print("./image.raw header sent.")

                # Define callback to send frame data in chunks
                def send_callback(frame_data):
                    # Optimized transfer - use larger chunks for efficiency
                    chunk_size = 153600 # 8192  # 8KB chunks for maximum throughput
                    total_sent = 0
                    mv = memoryview(frame_data)

                    # Set a reasonable timeout for the transfer
                    cl.settimeout(10.0)

                    while total_sent < len(frame_data):
                        end = min(total_sent + chunk_size, len(frame_data))
                        chunk = mv[total_sent:end]
                        try:
                            bytes_sent = cl.send(chunk)
                            if bytes_sent == 0:
                                break
                            total_sent += bytes_sent

                            # Feed watchdog during long image transfer
                            #if total_sent % (chunk_size * 4) == 0:  # Every ~32KB
                                #wdt.feed()
                        except OSError as e:
                            print(f"Socket error: {e}")
                            break

                debug_print("[MAIN] New server socket ready2")
                debug_print("just before camera.send_frame_over_eth")
                # Send frame using the new callback mechanism
                camera.send_frame_over_eth(send_callback)
                debug_print("[MAIN] Closing server socket and creating new one2. ..")
                s.close()
                time.sleep_ms(100)  # Give W5500 time to fully close
                gc.collect()  # Free memory from old socket
                s = create_server_socket()
                continue  # Skip to next iteration - don't fall through to cl.send()
        elif path == "/": # default
            debug_print("[MAIN] / html stream python version")
            response = generate_html_root("/stream")
        elif path == "/py":
            debug_print("[MAIN] / html stream py version")
            response = generate_html_root("/stream")
        elif path == "/c":
            debug_print("[MAIN] / html stream c version")
            response = generate_html_root("/streamc")
        else:
            # 404 for unknown paths
            debug_print("[MAIN] Calling generate_html_404()")
            response = generate_html_404()
        
        response_start = time.ticks_ms()
        response_time = time.ticks_diff(time.ticks_ms(), response_start)
            
            
        debug_print(f"Response generation time: {response_time}ms")
        debug_print("[MAIN] cl.send() is about to be executed , sendwithdma ")
        cl.send(response)

        debug_print("[MAIN] Calling cl.close()")
        cl.close()
        debug_print("[MAIN] close() returned successfully")

        # CRITICAL: Close the server socket and create a new one
        # The C driver no longer auto-re-listens, so Python must handle this
        debug_print("[MAIN] Closing server socket and creating new one. ..")
        s.close()
        time.sleep_ms(100)  # Give W5500 time to fully close
        gc.collect()  # Free memory from old socket
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


