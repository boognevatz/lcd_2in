"""
Barometer Module for WF5803F 01BA pressure sensor
"""

import machine
import time

# Global variables that will be imported from main
barometer_ready = None
i2c = None
BAROMETER_ADDR = None
BAROMETER_ATMOSPHERIC_RAW = None
BAROMETER_ATMOSPHERIC_BAR = None

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
        Tuple of (temperature_celsius, pressure_gauge_bar, pressure_raw)
        or (None, None, None) if read fails
    """
    if not barometer_ready:
        return (None, None, None)

    try:
        # Start measurement: Write 0x0A to CMD register (0x30)
        # 0x0A = 0x02 (measurement mode) + 0x08 (SCO bit)
        i2c.writeto_mem(BAROMETER_ADDR, 0x30, bytes([0x0A]))

        # Wait for data ready (DRDY bit in control register 0x02 goes to 0)
        for _ in range(50):
            time.sleep_ms(1)
            if not (i2c.readfrom_mem(BAROMETER_ADDR, 0x02, 1)[0] & 0x01):
                break
        else:
            print("Barometer measurement timeout")
            return (None, None, None)

        # Read pressure (24-bit signed, MSB first) from registers 0x06, 0x07, 0x08
        press_byte0 = i2c.readfrom_mem(BAROMETER_ADDR, 0x06, 1)[0]
        press_byte1 = i2c.readfrom_mem(BAROMETER_ADDR, 0x07, 1)[0]
        press_byte2 = i2c.readfrom_mem(BAROMETER_ADDR, 0x08, 1)[0]

        # Combine bytes (MSB first)
        pressure_raw = (press_byte0 << 16) | (press_byte1 << 8) | press_byte2

        # Sign extend 24-bit to 32-bit if negative (bit 23 is set)
        if pressure_raw & 0x800000:
            pressure_raw -= 0x1000000

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

        return (round(temperature_celsius, 1), round(pressure_gauge_bar, 3), pressure_raw)

    except Exception as e:
        print(f"Error reading barometer: {e}")
        return (None, None, None)