# test_single_client_server.py - Correct single-client server usage
# This test demonstrates the proper pattern for using the W5500 in single-client mode
# After accepting a connection, you must create a NEW socket to accept another connection
import machine
import network
import socket
import time

print("=== SINGLE-CLIENT SERVER TEST ===")

# Initialize W5500
print("\nInitializing W5500...")
cs = machine.Pin(17, machine.Pin.OUT)
rst = machine.Pin(20, machine.Pin.OUT)
cs.value(1)
rst.value(0)
time.sleep_ms(100)
rst.value(1)
time.sleep_ms(500)

spi = machine.SPI(0, baudrate=20000000, polarity=0, phase=0,
                  sck=machine.Pin(18), mosi=machine.Pin(19), miso=machine.Pin(16))

nic = network.WIZNET5K(spi, cs, rst)
nic.active(True)
nic.ifconfig(("172.16.1.1", "255.255.255.0", "172.16.1.1", "8.8.8.8"))
time.sleep_ms(1000)

print(f"IP: {nic.ifconfig()[0]}")

# Test 1: First connection
print("\n" + "="*50)
print("Test 1: First Connection (blocking)")
print("="*50)
print("1. Creating and binding socket...")
s = socket.socket()
s.bind(("0.0.0.0", 8080))
s.listen(5)

print("2. Ready for connection. In another terminal run:")
print("   telnet 172.16.1.1 8080")
print("3. Waiting 20 seconds for you to connect...")
time.sleep(20)

print("4. Accepting connection (blocking)...")
try:
    cl, addr = s.accept()
    print(f"   Accepted from {addr}")
    cl.send(b"Hello from accept! Connection 1 established.\r\n")
    cl.close()
    print("   Client connection closed")
except Exception as e:
    print(f"   Error: {e}")

# Test 2: Second connection (requires recreating socket)
print("\n" + "="*50)
print("Test 2: Second Connection (with timeout)")
print("="*50)
print("1. Creating NEW socket for next connection...")
s = socket.socket()
s.bind(("0.0.0.0", 8080))
s.listen(5)

print("2. Ready for connection. Run telnet again:")
print("   telnet 172.16.1.1 8080")
print("3. Waiting 20 seconds for you to connect...")
time.sleep(20)

print("4. Accepting connection (with 50 second timeout)...")
try:
    s.settimeout(50.0)
    cl, addr = s.accept()
    print(f"   Accepted from {addr}")
    cl.send(b"Hello from accept! Connection 2 established.\r\n")
    cl.close()
    print("   Client connection closed")
except Exception as e:
    print(f"   Error: {e}")

# Test 3: Third connection (non-blocking)
print("\n" + "="*50)
print("Test 3: Non-blocking Accept Test")
print("="*50)
print("1. Creating socket for non-blocking test...")
s = socket.socket()
s.bind(("0.0.0.0", 8080))
s.listen(5)

print("2. Setting non-blocking mode (timeout=0)...")
s.settimeout(0)

print("3. Trying accept without connection (should fail immediately)...")
try:
    cl, addr = s.accept()
    print(f"   Accepted from {addr} (unexpected!)")
    cl.close()
except Exception as e:
    print(f"   Expected error (no connection): {e}")

print("\n4. Now connect with telnet (quick!), waiting 50 seconds...")
time.sleep(50)

print("5. Trying accept again...")
try:
    cl, addr = s.accept()
    print(f"   Accepted from {addr}")
    cl.send(b"Hello from non-blocking accept!\r\n")
    cl.close()
    print("   Client connection closed")
except Exception as e:
    print(f"   Error: {e}")

print("\n" + "="*50)
print("=== TEST COMPLETE ===")
print("="*50)
print("\nKey Takeaways:")
print("- After accept(), the listening socket becomes the connected socket")
print("- After closing connection, you MUST create a NEW socket to accept again")
print("- This single-client design maximizes throughput with 16kB buffers")
print("- Each new client requires a new socket.socket() call")
