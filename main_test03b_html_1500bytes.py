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

# Create socket
s = socket.socket()
s.bind(("0.0.0.0", 80))
s.listen(5)
print(f"Listening on {nic.ifconfig()[0]}:80")

response_body = b"""\
<!DOCTYPE html>
<html>
    <head>
        <title>Hello</title>
        <link rel="icon" href="data:,">
    </head>
    <body><h1>Hello, world!</h1></body>
</html>
"""
content_length = str(len(response_body)).encode()
response_header0 = b"""\
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: """
response_header1 = b"""
Connection: close

"""

def generate_large_html_response():
    """Generate HTML response >1500 bytes with meaningful content"""
    
    # Large content template with semantic structure and inline styling
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
    </style>
</head>
<body>
    <div class="container">
        <h1>W5500 Large Payload Test Page</h1>
        
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
            </ul>
            
            <p><strong>Performance Metrics:</strong></p>
            <ul>
                <li>Response generation time: < 10ms</li>
                <li>Network transmission: Variable based on MTU</li>
                <li>Memory overhead: ~20% of payload size</li>
                <li>CPU utilization: Minimal during transmission</li>
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
            </ol>
        </div>

        <div style="background: #fff3cd; padding: 15px; border-radius: 5px; margin: 20px 0; border-left: 5px solid #ffc107;">
            <h3 style="color: #856404; margin-top: 0;">Debug Information</h3>
            <p>This response contains multiple paragraphs, inline CSS styling, semantic HTML structure, and comprehensive technical documentation to ensure it exceeds the 1500-byte Ethernet MTU threshold. The content is intentionally detailed to test the fragmentation capabilities of the W5500 Ethernet controller and the underlying TCP/IP stack implementation.</p>
        </div>

        <div class="footer">
            <p><em>Generated by W5500 Large Payload Test Server</em></p>
            <p><small>Testing Ethernet MTU fragmentation and reassembly capabilities</small></p>
        </div>
    </div>
</body>
</html>"""

    return html_content.encode()

while True:
    try:
        cl, addr = s.accept()
        print("Client connected from", addr)
        request = cl.recv(16384)  # Read request data
        #time.sleep_ms(50)
        print("Generating large HTML response...")
        response_start = time.ticks_ms()
        response_body = generate_large_html_response()
        response_time = time.ticks_diff(time.ticks_ms(), response_start)
            
        # Create HTTP headers
        response_header = b"""\
HTTP/1.1 200 OK
Content-Type: text/html; charset=UTF-8
Content-Length: """ + str(len(response_body)).encode() + b"""
Connection: close
Cache-Control: no-cache
Server: W5500-Large-Payload-Test/1.0

"""
            
        full_response = response_header + response_body
        print(f"Full response size: {len(full_response)} bytes (headers: {len(response_header)}, body: {len(response_body)})")
        print(f"Response generation time: {response_time}ms")
        cl.send(full_response)
        #time.sleep_ms(50)
        cl.close()
    except Exception as e:
        print("Error:", e)

