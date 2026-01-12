# test_diagnostic.py
import machine
import network
import socket
import time

print("=== DIAGNOSTIC TEST ===")

# Initialize (same as before)
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
s = socket.socket()
s.bind(("0.0.0.0", 8080))
s.listen(5)

print("\nListening on port 8080...")

s.settimeout(30000)
cl, addr = s.accept()
print(f"\n!!! SUCCESS! Accepted connection from {addr}")

# IMPORTANT: Set timeout to NON-BLOCKING (0) first
cl.settimeout(0)  # Non-blocking mode

print("\nTesting non-blocking recv...")
try:
    data = cl.recv(1024)
    print(f"Immediate non-blocking recv result: {data}")
except Exception as e:
    print(f"Non-blocking recv raised (expected): {e}")

print("\nNow set blocking with 5 second timeout...")
cl.settimeout(5000)

print("Calling recv(1024) - you have 5 seconds to type in telnet...")
start = time.ticks_ms()
try:
    data = cl.recv(1024)
    elapsed = time.ticks_diff(time.ticks_ms(), start)
    print(f"SUCCESS! Received after {elapsed}ms: {data}")
except Exception as e:
    elapsed = time.ticks_diff(time.ticks_ms(), start)
    print(f"FAILED after {elapsed}ms: {e}")

cl.close()
print("\n=== DIAGNOSTIC COMPLETE ===")


