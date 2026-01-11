
# debug_test.py
import machine
import network
import socket
import time
import select

print("=== W5500 DEBUG TEST ===")

# Initialize
cs = machine.Pin(17, machine.Pin.OUT)
rst = machine.Pin(20, machine.Pin.OUT)
cs.value(1)
rst.value(0)
time.sleep_ms(100)
rst.value(1)
time.sleep_ms(500)

spi = machine.SPI(
    0,
    baudrate=20000000,
    polarity=0,
    phase=0,
    sck=machine.Pin(18),
    mosi=machine.Pin(19),
    miso=machine.Pin(16),
)

nic = network.WIZNET5K(spi, cs, rst)
nic.active(True)
nic.ifconfig(("172.16.1.1", "255.255.255.0", "172.16.1.1", "8.8.8.8"))
time.sleep_ms(1000)

print(f"IP: {nic.ifconfig()[0]}")

# Create socket
print("\n1. Creating socket...")
s = socket.socket()
print(f"   Socket created, fileno: {s.fileno if hasattr(s, 'fileno') else 'N/A'}")

# Bind
print("\n2. Binding to port 8080...")
s.bind(("0.0.0.0", 8080))
print("   Bound")

# Listen
print("\n3. Listening...")
s.listen(5)
print("   Listening")

# Try to accept with timeout
print("\n4. Trying to accept with 5 second timeout...")
try:
    s.settimeout(5.0)
    cl, addr = s.accept()
    print(f"   Accepted connection from {addr}")
except Exception as e:
    print(f"   Accept failed: {e}")

# Test poll
print("\n5. Testing poll...")
poller = select.poll()
poller.register(s, select.POLLIN)
events = poller.poll(1000)  # 1 second timeout
print(f"   Poll events: {events}")

print("\n=== TEST COMPLETE ===")

