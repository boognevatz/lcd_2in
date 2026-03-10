"""
Temperature Sensor Module for ADC readings and temperature conversions
"""

import machine
import math
import time

# Global variables that will be imported from main
adc_ready = None
i2c = None
ADS7830_ADDR = None
ADC_REF_VOLTAGE = None
NTC_R0 = None
NTC_BETA = None
NTC_T0 = None
DIVIDER_R = None
DIVIDER_VCC = None

def warm_up_adc():
    """
    Initialize and warm up the ADS7830 ADC by performing initial reads on all channels
    This primes the ADC conversion pipeline and clears any stale data (0xFF)
    Should be called once after I2C initialization
    """
    if not adc_ready:
        return False
    try:
        # Perform initial conversion on all 6 temperature channels
        for channel in range(6):
            command = 0x8C | (channel << 4)

            # Start conversion
            i2c.writeto(ADS7830_ADDR, bytes([command]))
            time.sleep_ms(2)

            # Read and discard first result (may be stale 0xFF)
            data = i2c.readfrom(ADS7830_ADDR, 1)
            first_value = data[0]

            # If we got 0xFF, do another conversion to get valid data
            if first_value == 255:
                i2c.writeto(ADS7830_ADDR, bytes([command]))
                time.sleep_ms(2)
                i2c.readfrom(ADS7830_ADDR, 1)

            time.sleep_ms(1)

        return True
    except Exception as e:
        print(f"ADC warm-up failed: {e}")
        return False

def read_adc_channel(channel, max_retries=3):
    """
    Read a single channel from ADS7830 with retry logic for invalid readings

    Args:
        channel: Channel number (0-7)
        max_retries: Maximum number of retry attempts (default: 3)

    Returns:
        Voltage value in volts, or None if all retries fail
    """
    if not adc_ready:
        return None

    command = 0x8C | (channel << 4)

    for attempt in range(max_retries):
        try:
            # Command byte format for ADS7830:
            # Bit 7: SD=1 (single-ended mode)
            # Bits 6-4: channel select (direct mapping)
            # Bits 3-2: PD1-PD0=11 (internal reference ON, ADC ON)
            # Bits 1-0: Don't care

            # Write command byte to start conversion
            i2c.writeto(ADS7830_ADDR, bytes([command]))

            # Delay for conversion (datasheet: typ 32us, max 64us)
            time.sleep_ms(2)

            # Read result (8-bit value)
            adc_value = i2c.readfrom(ADS7830_ADDR, 1)[0]

            # Check if result is invalid (0xFF indicates stale/uninitialized data)
            if adc_value == 255:
                if attempt < max_retries - 1:
                    time.sleep_ms(5)
                    continue
                else:
                    return None

            # Convert to voltage (8-bit ADC, 0-255 maps to 0-ADC_REF_VOLTAGE)
            voltage = (adc_value / 255.0) * ADC_REF_VOLTAGE

            return voltage

        except OSError:
            if attempt < max_retries - 1:
                time.sleep_ms(5)
                continue
            else:
                return None

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
    # 0.0 indicates failed ADC read after retries
    if voltage <= 0.01 or voltage > (DIVIDER_VCC - 0.01):
        return None

    try:
        # Calculate thermistor resistance from voltage divider
        # NTC is on bottom (connected to GND), R_fixed is on top (connected to 3.3V)
        # R_NTC = (V_out * R_fixed) / (VCC - V_out)
        r_ntc = (voltage * DIVIDER_R) / (DIVIDER_VCC - voltage)

        # Beta equation: 1/T = 1/T0 + (1/B) * ln(R/R0)
        temp_kelvin = 1.0 / (1.0 / NTC_T0 + (1.0 / NTC_BETA) * math.log(r_ntc / NTC_R0))
        temp_celsius = temp_kelvin - 273.15

        # Return as integer (no decimals)
        return int(round(temp_celsius))
    except Exception as e:
        print(f"Error converting voltage to temperature: {e}")
        return None

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

def read_all_temperatures():
    """
    Read all 6 temperature channels from ADC and convert to Celsius

    Returns:
        Dictionary with headTemp1 through headTemp6, each containing
        {"raw_v": voltage_float_or_None, "temp_c": int_or_None}
    """
    temps = {}
    for i in range(6):
        voltage = read_adc_channel(i)
        temp_c = voltage_to_temperature(voltage)
        temps[f"headTemp{i+1}"] = {"raw_v": voltage, "temp_c": temp_c}
    return temps