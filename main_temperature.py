"""
Minimal self-contained temperature reader for ADS7830 + NTC thermistors on RP2350A.
"""

import machine
import math
import time

# --- Configuration ---
I2C_SDA = 12
I2C_SCL = 13
I2C_FREQ = 100_000
ADS7830_ADDR = 0x48
ADC_REF_VOLTAGE = 2.5
NTC_R0 = 10_000
NTC_BETA = 3950
NTC_T0 = 298.15  # 25°C in Kelvin
DIVIDER_R = 10_000
DIVIDER_VCC = 3.3

# --- I2C init ---
i2c = machine.I2C(0, sda=machine.Pin(I2C_SDA), scl=machine.Pin(I2C_SCL), freq=I2C_FREQ)

# --- Check for ADS7830 ---
devices = i2c.scan()
adc_ready = ADS7830_ADDR in devices
if not adc_ready:
    print("WARNING: ADS7830 not found at 0x48")

def _adc_read(channel):
    if not adc_ready:
        return None
    cmd = 0x8C | (channel << 4)
    for _ in range(3):
        try:
            i2c.writeto(ADS7830_ADDR, bytes([cmd]))
            time.sleep_ms(2)
            val = i2c.readfrom(ADS7830_ADDR, 1)[0]
            if val != 255:
                return (val / 255.0) * ADC_REF_VOLTAGE
            time.sleep_ms(5)
        except OSError:
            time.sleep_ms(5)
    return None

def _voltage_to_temp(v):
    if v is None or v <= 0.01 or v > (DIVIDER_VCC - 0.01):
        return None
    r_ntc = (v * DIVIDER_R) / (DIVIDER_VCC - v)
    t_k = 1.0 / (1.0 / NTC_T0 + (1.0 / NTC_BETA) * math.log(r_ntc / NTC_R0))
    return int(round(t_k - 273.15))

def mcu_temp():
    v = (machine.ADC(4).read_u16() / 65535.0) * 3.3
    return int(round(27 - (v - 0.706) / 0.001721))

# --- Warm up ADC (discard first reads) ---
if adc_ready:
    for ch in range(6):
        _adc_read(ch)

# --- Main loop ---
while True:
    for ch in range(6):
        t = _voltage_to_temp(_adc_read(ch))
        print(f"headTemp{ch+1}: {t} C")
    print(f"MCU: {mcu_temp()} C")
    print("---")
    time.sleep(2)
