"""JSON API handler for control port 8082."""

import time
import json

# Global variables (will be injected from main.py)
HEAD_ID = None
boot_time_ms = None


def _format_head_id():
    """Format HEAD_ID for JSON output (quote strings, leave numbers bare)."""
    if isinstance(HEAD_ID, (int, float)):
        return str(HEAD_ID)
    return json.dumps(str(HEAD_ID))


def _save_head_id(new_id):
    """Update HEAD_ID in memory and persist to config.json."""
    global HEAD_ID
    HEAD_ID = new_id
    try:
        with open('config.json', 'r') as f:
            config = json.load(f)
        config['headID'] = new_id
        with open('config.json', 'w') as f:
            json.dump(config, f)
        return True
    except Exception as e:
        print(f"Error saving config: {e}")
        return False


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


def _import_functions():
    """Import required functions from other modules"""
    from .tempsensor import get_mcu_temperature, read_all_temperatures
    from .barometer import read_barometer
    from .ledcontrol import set_head_led_brightness
    return get_mcu_temperature, read_barometer, read_all_temperatures, set_head_led_brightness


def handle_json_request(cl):
    """Handle requests on port 8082 - serve JSON status data or handle LED control"""
    try:
        # Set blocking mode temporarily for recv
        cl.setblocking(True)
        cl.settimeout(2.0)

        # Read the HTTP request
        request = cl.recv(1024).decode('utf-8')
        request_line = request.split('\r\n')[0]

        # Parse the request path
        path = request_line.split(' ')[1] if len(request_line.split(' ')) > 1 else '/'

        # Handle LED brightness control: /headled/{value}
        if path.startswith('/headled/'):
            try:
                # Extract brightness value from path
                value_str = path.split('/headled/')[1]
                brightness = int(value_str)

                # Get the set_head_led_brightness function
                _, _, _, set_head_led_brightness_func = _import_functions()

                # Set LED brightness
                if set_head_led_brightness_func(brightness):
                    response = f'{{"status": "ok", "brightness": {brightness}}}'
                    http_response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n" + response
                else:
                    response = '{"status": "error", "message": "DAC not available"}'
                    http_response = "HTTP/1.1 503 Service Unavailable\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n" + response

                cl.send(http_response.encode())
            except (ValueError, IndexError):
                response = '{"status": "error", "message": "Invalid brightness value"}'
                http_response = "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n" + response
                cl.send(http_response.encode())

        # Handle rename: /rename/{value}
        elif path.startswith('/rename/'):
            try:
                new_id = path.split('/rename/')[1]
                if not new_id:
                    raise ValueError("Empty ID")
                # Try to keep as int if it looks numeric
                try:
                    new_id = int(new_id)
                except ValueError:
                    pass  # Keep as string
                if _save_head_id(new_id):
                    response = f'{{"status": "ok", "headID": {_format_head_id()}}}'
                    http_response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n" + response
                else:
                    response = '{"status": "error", "message": "Failed to save config"}'
                    http_response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n" + response
                cl.send(http_response.encode())
            except (ValueError, IndexError):
                response = '{"status": "error", "message": "Invalid ID value"}'
                http_response = "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n" + response
                cl.send(http_response.encode())

        # Handle status JSON: /
        elif path == '/':
            # Get uptime
            uptime = get_uptime_seconds()

            # Get functions from other modules
            get_mcu_temperature_func, read_barometer_func, read_all_temps_func, _ = _import_functions()

            # Get MCU temperature
            mcu_temp = get_mcu_temperature_func()

            # Read barometer
            cooling_temp, cooling_press, cooling_p_raw = read_barometer_func()

            # Read LED temperatures from ADC
            led_temps = read_all_temps_func()

            # Build JSON response manually
            json_parts = [f'"headID": {_format_head_id()}', f'"upTime": {uptime}']

            # Add MCU temperature
            if mcu_temp is not None:
                json_parts.append(f'"mcuTemp": {mcu_temp}')
            else:
                json_parts.append(f'"mcuTemp": null')

            # Add cooling temperature, pressure, and raw pressure
            if cooling_temp is not None:
                json_parts.append(f'"cooling_temperature": {cooling_temp}')
            else:
                json_parts.append(f'"cooling_temperature": null')

            if cooling_press is not None:
                json_parts.append(f'"cooling_pressure": {cooling_press}')
            else:
                json_parts.append(f'"cooling_pressure": null')

            if cooling_p_raw is not None:
                json_parts.append(f'"cooling_pressure_raw": {cooling_p_raw}')
            else:
                json_parts.append(f'"cooling_pressure_raw": null')

            # Add LED temperatures in order (1-6)
            for i in range(1, 7):
                key = f"headTemp{i}"
                entry = led_temps.get(key)
                if entry is not None:
                    raw_v = entry.get("raw_v")
                    temp_c = entry.get("temp_c")
                    rv = f"{raw_v}" if raw_v is not None else "null"
                    tc = f"{temp_c}" if temp_c is not None else "null"
                    json_parts.append(f'"{key}": {{"raw_v": {rv}, "temp_c": {tc}}}')
                else:
                    json_parts.append(f'"{key}": null')

            json_data = '{' + ', '.join(json_parts) + '}'

            # Send JSON response
            response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n" + json_data
            cl.send(response.encode())

        else:
            # 404 for unknown paths
            response = '{"status": "error", "message": "Not found"}'
            http_response = "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n" + response
            cl.send(http_response.encode())

        # Always close the connection after handling the request
        cl.close()

    except Exception as e:
        print(f"JSON handler error: {e}")
        try:
            cl.close()
        except:
            pass
