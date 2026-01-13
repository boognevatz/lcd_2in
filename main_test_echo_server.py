# test_echo_server.py - Echo server test
# This demonstrates two-way communication with client
import machine
import network
import socket
import time

print("=== ECHO SERVER TEST ===")

# Initialize
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

# Create echo server
s = socket.socket()
s.bind(("0.0.0.0", 8080))
s.listen(5)

print("\nEcho server ready. Connect with: telnet 172.16.1.1 8080")
print("Type messages and they'll be echoed back.\n")

try:
    cl, addr = s.accept()
    print(f"Connected to {addr}")
    cl.send(b"Echo server ready. Type messages to echo them back!\r\n")

    while True:
        try:
            cl.settimeout(5.0)
            data = cl.recv(128)
            if not data:
                break
            print(f"Received: {data}")
            cl.send(b"Echo: " + data)
        except Exception as e:
            if e.errno != 110:  # ETIMEDOUT
                print(f"Error: {e}")
                break
except Exception as e:
    print(f"Accept error: {e}")

cl.close()
print("Connection closed")
