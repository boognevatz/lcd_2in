import machine
import network
import socket
import time

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

# Web server loop
while True:
    try:
        cl, addr = s.accept()
        print("Client connected from", addr)
        request = cl.recv(1024)  # Read request data
        time.sleep_ms(50)
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
        response_header = b"""\
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: """ + str(len(response_body)).encode() + b"""
Connection: close

"""
        cl.send(response_header + response_body)
        time.sleep_ms(50)
        cl.close()
    except Exception as e:
        print("Error:", e)

