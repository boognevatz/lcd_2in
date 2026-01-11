import network
import socket
import time
from machine import SPI, Pin

# tested on 20250722, working
# W5500 SPI pin configuration:
# GPIO 0 - MISO
# GPIO 1 - SCSN
# GPIO 2 - SCLK
# GPIO 3 - MOSI
# GPIO 4 - RSTN 
# GPIO 5 - INTN

# Configure SPI for W5500 (adjust pins if needed)
spi = SPI(0, baudrate=2_000_000, polarity=0, phase=0,
          sck=Pin(2), mosi=Pin(3), miso=Pin(0))

cs = Pin(1, Pin.OUT)      # Chip Select
rst = Pin(4, Pin.OUT)   # Optional: Reset pin for W5500

# Reset the W5500
rst.value(0) # 0V
time.sleep(0.1)
rst.value(1) # 3V3
time.sleep(0.5)

# Define static IP configuration
ip = '192.168.4.1'
subnet = '255.255.255.0'
gateway = '192.168.4.1'
dns = '8.8.8.8'

# Initialize W5500
nic = network.WIZNET5K(spi, cs, rst)
nic.active(True)
nic.ifconfig((ip, subnet, gateway, dns))

# Wait for link
print("Waiting for Ethernet link...")
while not nic.isconnected():
    time.sleep(0.1)
print("Connected. IP address:", nic.ifconfig()[0])

# Create socket
addr = socket.getaddrinfo(ip, 80)[0][-1]
s = socket.socket()
s.bind(addr)
s.listen(1)
print("Listening on", addr)

# Web server loop
while True:
    try:
        cl, addr = s.accept()
        print("Client connected from", addr)
        request = cl.recv(1024)  # Read request data

        response = b"""\
HTTP/1.1 200 OK
Content-Type: text/html

<!DOCTYPE html>
<html>
    <head><title>Hello</title></head>
    <body><h1>Hello, world!</h1></body>
</html>
"""
        cl.send(response)
        cl.close()
    except Exception as e:
        print("Error:", e)

