# test_accept_fix.py
import machine
import network
import socket
import time

print("=== TEST ACCEPT FIX ===")

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
print("1. In another terminal, run: telnet 172.16.1.1 8080")
print("2. Type something and press Enter")
print("3. This should now work!")

try:
    # Set a reasonable timeout
    s.settimeout(30000)  # 30 seconds
    
    print("\nWaiting for connection...")
    cl, addr = s.accept()
    print(f"\n!!! SUCCESS! Accepted connection from {addr}")
    
    # Set timeout on client socket too
    cl.settimeout(5000)  # 5 second timeout for recv
    
    # Try to receive data with polling
    print("Waiting for data from client...")
    
    # Poll for data instead of blocking
    import select
    import sys
    
    while True:
        # Check if data is available (non-blocking check)
        ready = select.select([cl], [], [], 1.0)
        if ready[0]:
            data = cl.recv(1024)
            if data:
                print(f"Received: {data}")
                # Send response
                cl.send(b"Echo: " + data + b"\r\n")
            else:
                print("Connection closed by client")
                break
        else:
            # No data yet
            print(".", end="")
            sys.stdout.flush()
    
    cl.close()
    
except Exception as e:
    print(f"\nError: {e}")

print("\n=== TEST COMPLETE ===")


