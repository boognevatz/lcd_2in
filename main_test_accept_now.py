# test_accept_now.py
import machine
import network
import socket
import time

print("=== TEST ACCEPT AFTER FIXES ===")

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

# Create socket WITHOUT calling settimeout first
print("\n1. Creating and binding socket...")
s = socket.socket()
s.bind(("0.0.0.0", 8080))
s.listen(5)

print("\n2. Socket ready. Now in another terminal run:")
print("   telnet 172.16.1.1 8080")
print("\n3. Then we'll try accept...")

# Small delay to give time to start telnet
time.sleep(2)

print("\n4. Trying accept WITHOUT settimeout first...")
try:
    # Try accept without any timeout setting
    cl, addr = s.accept()
    print(f"   Accepted from {addr}")
    cl.send(b"Hello from accept!\r\n")
    cl.close()
except Exception as e:
    print(f"   Accept error: {e}")

print("\n5. Trying accept WITH settimeout...")
try:
    s.settimeout(5.0)
    cl, addr = s.accept()
    print(f"   Accepted from {addr}")
    cl.send(b"Hello from accept with timeout!\r\n")
    cl.close()
except Exception as e:
    print(f"   Accept with timeout error: {e}")

print("\n=== TEST COMPLETE ===")


