import machine
import network
import socket
import time
import gc

# W5500 SPI pin configuration (matching working echo server):
# GPIO 16 - MISO
# GPIO 17 - CS (SCSN)
# GPIO 18 - SCK (SCLK)
# GPIO 19 - MOSI
# GPIO 20 - RST (RSTN)

sendwithdma = 0
cs = machine.Pin(17, machine.Pin.OUT)
rst = machine.Pin(20, machine.Pin.OUT)
cs.value(1)
rst.value(0)
time.sleep_ms(100)
rst.value(1)
time.sleep_ms(500)

spi = machine.SPI(0, baudrate=20000000, polarity=0, phase=0,
                  sck=machine.Pin(18), mosi=machine.Pin(19), miso=machine.Pin(16))

# Define static IP configuration
ip = '172.16.1.1'
subnet = '255.255.255.0'
gateway = '172.16.1.1'
dns = '8.8.8.8'

# Initialize W5500
nic = network.WIZNET5K(spi, cs, rst)
nic.active(True)
nic.ifconfig((ip, subnet, gateway, dns))
time.sleep_ms(1000)

# Wait for link
print("Waiting for Ethernet link...")
while not nic.isconnected():
    time.sleep(0.1)
print("Connected. IP address:", nic.ifconfig()[0])

# Socket will be created in the main loop
print(f"Server will listen on {nic.ifconfig()[0]}:80")


def generate_small_html_response():
    """Generate HTML response just a small once."""
    
    gc.collect()
    html_content = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>W5500 Large Payload Test</title>
    <link rel="icon" href="data:,">
    <style>
        body { 
            font-family: Arial, sans-serif; 
            max-width: 800px; 
            margin: 0 auto; 
            padding: 20px; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: #333;
        }
        .container { 
            background: white; 
            padding: 30px; 
            border-radius: 10px; 
            box-shadow: 0 10px 30px rgba(0,0,0,0.2);
        }
        h1 { 
            color: #2c3e50; 
            text-align: center; 
            border-bottom: 3px solid #3498db;
            padding-bottom: 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>W5500 Small Payload Test Page</h1>
        
        
        <div class="status">
            <h2>System Status</h2>
            <p><strong>Server:</strong> W5500 Ethernet Controller</p>
            <p><strong>Payload Size:</strong> """
    
    # Add dynamic content
    gc.collect()
    free_mem = gc.mem_free()
    alloc_mem = gc.mem_alloc()
    
    html_content += f"{len(html_content.encode())} bytes and growing</p>"
    html_content += f"<p><strong>Free Memory:</strong> {free_mem} bytes</p>"
    html_content += f"<p><strong>Allocated Memory:</strong> {alloc_mem} bytes</p>"
    html_content += f"<p><strong>Timestamp:</strong> {time.ticks_ms()}</p>"
    
    html_content += """
        </div>

        <div class="metrics">
            <div class="metric-card">
                <div class="metric-value">1500+</div>
                <div>Bytes Payload</div>
            </div>
            <div class="metric-card">
                <div class="metric-value">W5500</div>
                <div>Ethernet Chip</div>
            </div>
            <div class="metric-card">
                <div class="metric-value">TCP/IP</div>
                <div>Protocol Stack</div>
            </div>
            <div class="metric-card">
                <div class="metric-value">Fragmented</div>
                <div>Packet Transfer</div>
            </div>
        </div>

    </div>
</body>
</html>"""

    return html_content.encode()


def generate_large_html_response():
    """Generate HTML response >1500 bytes with meaningful content"""
    
    gc.collect()
    html_content = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>W5500 Large Payload Test</title>
    <link rel="icon" href="data:,">
    <style>
        body { 
            font-family: Arial, sans-serif; 
            max-width: 800px; 
            margin: 0 auto; 
            padding: 20px; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: #333;
        }
        .container { 
            background: white; 
            padding: 30px; 
            border-radius: 10px; 
            box-shadow: 0 10px 30px rgba(0,0,0,0.2);
        }
        h1 { 
            color: #2c3e50; 
            text-align: center; 
            border-bottom: 3px solid #3498db;
            padding-bottom: 10px;
        }
        .status { 
            background: #ecf0f1; 
            padding: 15px; 
            border-radius: 5px; 
            margin: 20px 0;
            border-left: 5px solid #3498db;
        }
        .metrics { 
            display: grid; 
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); 
            gap: 20px; 
            margin: 20px 0;
        }
        .metric-card { 
            background: #f8f9fa; 
            padding: 15px; 
            border-radius: 8px; 
            border: 1px solid #dee2e6;
            text-align: center;
        }
        .metric-value { 
            font-size: 24px; 
            font-weight: bold; 
            color: #3498db;
        }
        .footer { 
            text-align: center; 
            margin-top: 30px; 
            padding-top: 20px;
            border-top: 1px solid #bdc3c7;
            color: #7f8c8d;
        }
        .tech-info {
            background: #2c3e50;
            color: white;
            padding: 20px;
            border-radius: 8px;
            margin: 20px 0;
        }
        .tech-info h3 {
            color: #3498db;
            margin-top: 0;
        }
        .svg-display {
            text-align: center;
            margin: 20px 0;
        }
        .svg-display img {
            border: 2px solid #3498db;
            border-radius: 8px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>W5500 Large Payload Test Page</h1>
        
        <div class="svg-display">
            <h2>Rose SVG (50kB+)</h2>
            <img src="/svg" width="300" height="300" alt="Detailed Rose SVG">
        </div>
        
        <div class="status">
            <h2>System Status</h2>
            <p><strong>Server:</strong> W5500 Ethernet Controller</p>
            <p><strong>Payload Size:</strong> """
    
    # Add dynamic content
    gc.collect()
    free_mem = gc.mem_free()
    alloc_mem = gc.mem_alloc()
    
    html_content += f"{len(html_content.encode())} bytes and growing</p>"
    html_content += f"<p><strong>Free Memory:</strong> {free_mem} bytes</p>"
    html_content += f"<p><strong>Allocated Memory:</strong> {alloc_mem} bytes</p>"
    html_content += f"<p><strong>Timestamp:</strong> {time.ticks_ms()}</p>"
    
    html_content += """
        </div>

        <div class="metrics">
            <div class="metric-card">
                <div class="metric-value">1500+</div>
                <div>Bytes Payload</div>
            </div>
            <div class="metric-card">
                <div class="metric-value">W5500</div>
                <div>Ethernet Chip</div>
            </div>
            <div class="metric-card">
                <div class="metric-value">TCP/IP</div>
                <div>Protocol Stack</div>
            </div>
            <div class="metric-card">
                <div class="metric-value">Fragmented</div>
                <div>Packet Transfer</div>
            </div>
        </div>

        <div class="tech-info">
            <h3>Technical Information</h3>
            <p>This page demonstrates the W5500 Ethernet controller's ability to handle HTTP responses larger than the standard Ethernet MTU (Maximum Transmission Unit) of 1500 bytes. The content you're reading right now is specifically designed to exceed this threshold, forcing proper TCP packet fragmentation and reassembly.</p>
            
            <p><strong>Key Features Tested:</strong></p>
            <ul>
                <li>Packet fragmentation handling by W5500 hardware</li>
                <li>TCP flow control and buffer management</li>
                <li>Large payload transmission over Ethernet</li>
                <li>Memory management in MicroPython</li>
                <li>HTTP protocol compliance for large responses</li>
                <li>Serving large SVG graphics</li>
            </ul>
            
            <p><strong>Performance Metrics:</strong></p>
            <ul>
                <li>Response generation time: < 10ms</li>
                <li>Network transmission: Variable based on MTU</li>
                <li>Memory overhead: ~20% of payload size</li>
                <li>CPU utilization: Minimal during transmission</li>
                <li>SVG size: 50kB+ for detailed graphics</li>
            </ul>
        </div>

        <div style="background: #e8f5e8; padding: 15px; border-radius: 5px; margin: 20px 0;">
            <h3 style="color: #27ae60; margin-top: 0;">Success Indicators</h3>
            <p>If you can read this complete page without loading errors, it means:</p>
            <ol>
                <li>The W5500 successfully fragmented the large response</li>
                <li>TCP reassembly worked correctly on the client side</li>
                <li>HTTP headers were processed properly</li>
                <li>Content-Length header matched actual payload</li>
                <li>Connection was handled gracefully</li>
                <li>The SVG loads and displays correctly</li>
            </ol>
        </div>

        <div style="background: #fff3cd; padding: 15px; border-radius: 5px; margin: 20px 0; border-left: 5px solid #ffc107;">
            <h3 style="color: #856404; margin-top: 0;">Debug Information</h3>
            <p>This response contains multiple paragraphs, inline CSS styling, semantic HTML structure, and comprehensive technical documentation to ensure it exceeds the 1500-byte Ethernet MTU threshold. The content is intentionally detailed to test the fragmentation capabilities of the W5500 Ethernet controller and the underlying TCP/IP stack implementation. Additionally, a large SVG graphic is served at /svg endpoint.</p>
        </div>

        <div class="footer">
            <p><em>Generated by W5500 Large Payload Test Server</em></p>
            <p><small>Testing Ethernet MTU fragmentation and reassembly capabilities</small></p>
        </div>
    </div>
</body>
</html>"""

    return html_content.encode()

def generate_svg_response():
    """Generate a large SVG response >50kB with a detailed rose graphic"""
    gc.collect()
    svg_content = '<?xml version="1.0" encoding="UTF-8"?>\n<svg width="300" height="300" viewBox="0 0 300 300" xmlns="http://www.w3.org/2000/svg">\n'
    
    # Background
    svg_content += '<rect width="300" height="300" fill="#f9f9f9"/>\n'
    
    # Stem
    svg_content += '<path d="M 150 250 L 150 180" stroke="#228B22" stroke-width="4" fill="none"/>\n'
    
    # Leaves
    svg_content += '<ellipse cx="130" cy="220" rx="15" ry="8" fill="#32CD32" transform="rotate(-30 130 220)"/>\n'
    svg_content += '<ellipse cx="170" cy="210" rx="15" ry="8" fill="#32CD32" transform="rotate(30 170 210)"/>\n'
    
    # Petal template (detailed curve)
    petal_base = '<path d="M 150 150 Q 165 135 180 150 Q 195 165 180 180 Q 165 195 150 180 Q 135 165 150 150" fill="#FF69B4" stroke="#DC143C" stroke-width="1"/>\n'
    
    # Generate many petals with variations to create a detailed rose (aiming for >50kB)
    for layer in range(15):  # 5 layers
        for petal in range(8):  # 8 petals per layer
            angle = petal * 45 + layer * 10  # Slight rotation per layer
            scale = 1 - layer * 0.1  # Smaller petals in outer layers
            x_offset = layer * 5
            y_offset = layer * 5
            svg_content += f'<g transform="translate({x_offset},{y_offset}) rotate({angle} 150 150) scale({scale})">\n{petal_base}</g>\n'
    
    # Add additional detail elements to increase size
    for detail in range(200):  # Add 200 small detail circles
        x = 100 + (detail % 20) * 5
        y = 100 + (detail // 20) * 5
        svg_content += f'<circle cx="{x}" cy="{y}" r="0.5" fill="#FF1493" opacity="0.7"/>\n'
    
    # Add thorn details
    for thorn in range(10):
        y_pos = 180 + thorn * 10
        svg_content += f'<polygon points="148,{y_pos} 152,{y_pos} 150,{y_pos+5}" fill="#8B4513"/>\n'
    
    # Add more detailed petals with more complex paths
    complex_petal = '<path d="M 150 150 C 160 140 175 145 180 155 C 185 165 180 175 170 180 C 160 185 155 180 150 175 C 145 170 140 165 145 155 C 150 145 150 150 150 150" fill="#FFB6C1" stroke="#FF6347" stroke-width="1"/>\n'
    
    for extra in range(50):  # Add 50 complex petals
        angle = extra * 7.2  # Even distribution
        scale = 0.8 + (extra % 5) * 0.05
        svg_content += f'<g transform="rotate({angle} 150 150) scale({scale})">\n{complex_petal}</g>\n'
    
    svg_content += '</svg>'
    
    return svg_content.encode()

def create_server_socket():
    """Create, bind and listen on a new server socket"""
    server = socket.socket()
    server.bind(("0.0.0.0", 80))
    server.listen(5)
    print(f"[MAIN] Server socket created and listening on port 80")
    return server

# Initial server socket
s = create_server_socket()


while True:
    try:
        gc.collect()
        print("[MAIN] ====== WAITING FOR CONNECTION ======")
        cl, addr = s.accept()
        print(f"[MAIN] accept() returned, addr={addr}")
       
        request = cl.recv(8192)
        print(f"[MAIN] recv() returned {len(request)} bytes")

        request_str = request.decode()
        print(f"[MAIN] Request decoded, length={len(request_str)}")

        request_lines = request_str.split('\r\n')
        print(f"[MAIN] Request split into {len(request_lines)} lines")

        request_line = request_lines[0]
        print(f"[MAIN] First line: {request_line[:50]}...")

        parts = request_line.split()
        print(f"[MAIN] Split into {len(parts)} parts")

        if len(parts) >= 2:
            method, path = parts[0], parts[1]
            print(f"[MAIN] Parsed: method={method}, path={path}")
        else:
            path = '/'
            print("[MAIN] Using default path=/")

        print(f"[MAIN] Request path: {path}")

        # Generate response
        if path == '/svg':
            print("[MAIN] Calling generate_svg_response()")
            response_body = generate_svg_response()
            print(f"[MAIN] SVG generated, size={len(response_body)}")
            content_type = 'image/svg+xml'
        elif path == '/1':
            print("[MAIN] Calling generate_small_html_response()")
            response_body = generate_small_html_response()
            print(f"[MAIN] HTML small generated, size={len(response_body)}")
            content_type = 'text/html; charset=UTF-8'
        else:
            print("[MAIN] Calling generate_large_html_response()")
            response_body = generate_large_html_response()
            print(f"[MAIN] HTML generated, size={len(response_body)}")
            content_type = 'text/html; charset=UTF-8'
        
        response_start = time.ticks_ms()
        response_time = time.ticks_diff(time.ticks_ms(), response_start)
            
        # Create HTTP headers
        response_header = b'HTTP/1.1 200 OK\r\nContent-Type: ' + content_type.encode() + b'\r\nContent-Length: ' + str(len(response_body)).encode() + b'\r\nConnection: close\r\nCache-Control: no-cache\r\nServer: W5500-Large-Payload-Test/1.0\r\n\r\n'
            
        full_response = response_header + response_body
        print(f"Full response size: {len(full_response)} bytes (headers: {len(response_header)}, body: {len(response_body)})")
        print(f"Response generation time: {response_time}ms")
        print("[MAIN] cl.send() is about to be executed , sendwithdma: ", sendwithdma)
        if sendwithdma > 0:
            cl.send(full_response)
        else:
            cl.send_without_dma(full_response)
        sendwithdma =sendwithdma + 1

        print("[MAIN] Calling cl.close()")
        cl.close()
        print("[MAIN] close() returned successfully")

        # CRITICAL: Close the server socket and create a new one
        # The C driver no longer auto-re-listens, so Python must handle this
        print("[MAIN] Closing server socket and creating new one. ..")
        s.close()
        time.sleep_ms(100)  # Give W5500 time to fully close
        gc.collect()  # Free memory from old socket
        s = create_server_socket()
        print("[MAIN] New server socket ready")
        del response_header
        del response_body
        del full_response

    except Exception as e:
        print("Error:", e)
         # Try to recover by creating a new server socket
        try:
            s.close()
        except Exception as _:
            pass
        time.sleep_ms(500)
        gc.collect()
        s = create_server_socket()





