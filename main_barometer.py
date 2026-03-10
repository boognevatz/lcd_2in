"""
Minimal self-contained barometer reader for WF5803F 01BA on RP2350A.
"""

import machine
import time

# --- Configuration ---
I2C_SDA = 12
I2C_SCL = 13
I2C_FREQ = 100_000
BAROMETER_ADDR = 0x6C

# Calibration: atmospheric reference for gauge pressure calculation
# ~5,600,000 raw counts = 1 atm (~1.01325 bar) measured from device
ATMOSPHERIC_RAW = 5600000
ATMOSPHERIC_BAR = 1.01325

# --- I2C init ---
i2c = machine.I2C(0, sda=machine.Pin(I2C_SDA), scl=machine.Pin(I2C_SCL), freq=I2C_FREQ)

# --- Check for barometer ---
devices = i2c.scan()
baro_ready = BAROMETER_ADDR in devices
print(f"I2C devices: {[hex(d) for d in devices]}")
if not baro_ready:
    print(f"WARNING: Barometer not found at 0x{BAROMETER_ADDR:02X}")

def read_barometer():
    if not baro_ready:
        return None, None, None
    try:
        # Start measurement: write 0x0A to CMD register 0x30
        i2c.writeto_mem(BAROMETER_ADDR, 0x30, bytes([0x0A]))

        # Wait for DRDY (bit 0 of register 0x02) to clear
        for _ in range(50):
            time.sleep_ms(1)
            if not (i2c.readfrom_mem(BAROMETER_ADDR, 0x02, 1)[0] & 0x01):
                break
        else:
            return None, None, None

        # Pressure: 24-bit signed from 0x06-0x08
        p0 = i2c.readfrom_mem(BAROMETER_ADDR, 0x06, 1)[0]
        p1 = i2c.readfrom_mem(BAROMETER_ADDR, 0x07, 1)[0]
        p2 = i2c.readfrom_mem(BAROMETER_ADDR, 0x08, 1)[0]
        p_raw = (p0 << 16) | (p1 << 8) | p2
        if p_raw & 0x800000:
            p_raw -= 0x1000000

        # Temperature: 16-bit signed from 0x09-0x0A
        t0 = i2c.readfrom_mem(BAROMETER_ADDR, 0x09, 1)[0]
        t1 = i2c.readfrom_mem(BAROMETER_ADDR, 0x0A, 1)[0]
        t_raw = (t0 << 8) | t1
        if t_raw > 32767:
            t_raw -= 65536

        temp_c = t_raw / 256.0
        scale = ATMOSPHERIC_BAR / ATMOSPHERIC_RAW
        p_gauge = max(0.0, p_raw * scale - ATMOSPHERIC_BAR)

        return round(temp_c, 1), p_raw, round(p_gauge, 3)

    except OSError:
        return None, None, None

# --- Main loop ---
while True:
    temp, p_raw, p_bar = read_barometer()
    print(f"Baro temp: {temp} C  p_raw: {p_raw}  gauge: {p_bar} bar")
    time.sleep(2)
