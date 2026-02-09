#!/usr/bin/env python3
"""
UDP Streaming Benchmark Client for Pico W Camera

Tests raw UDP throughput from Pico W camera stream to determine
if TCP ACK delays are the bottleneck limiting throughput.

Usage:
    python3 test_udp_stream.py <pico_ip> [num_frames] [udp_port]

Examples:
    python3 test_udp_stream.py 172.16.1.1
    python3 test_udp_stream.py 172.16.1.1 100
    python3 test_udp_stream.py 172.16.1.1 200 8889

Protocol:
    1. Client sends HTTP GET to /streamcudp?frames=N&port=P
    2. Server responds with HTTP 200 and starts UDP streaming
    3. Server sends UDP packets:
       - Frame header: "F:frame_num:frame_size\n"
       - Data chunks: [1 byte chunk_num][up to 1400 bytes data]
       - End marker: "END:frames:bytes:elapsed_ms:mbps\n"
    4. Server sends final stats via TCP

Compare results with TCP streaming:
    curl -o /dev/null -w "Speed: %{speed_download} bytes/sec\n" http://172.16.1.1/streamc
"""

import socket
import sys
import time
import argparse
from collections import defaultdict

# Frame size: 240 x 320 x 2 (RGB565)
FRAME_SIZE = 240 * 320 * 2  # 153600 bytes


def main():
    parser = argparse.ArgumentParser(description='UDP Streaming Benchmark Client')
    parser.add_argument('pico_ip', help='IP address of the Pico W (e.g., 172.16.1.1)')
    parser.add_argument('num_frames', type=int, nargs='?', default=100,
                        help='Number of frames to stream (default: 100)')
    parser.add_argument('udp_port', type=int, nargs='?', default=8889,
                        help='UDP port to receive on (default: 8889)')
    parser.add_argument('--timeout', type=float, default=30.0,
                        help='Receive timeout in seconds (default: 30)')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='Verbose output')
    args = parser.parse_args()

    pico_ip = args.pico_ip
    num_frames = args.num_frames
    udp_port = args.udp_port
    verbose = args.verbose

    print(f"UDP Streaming Benchmark")
    print(f"=" * 50)
    print(f"Pico IP:     {pico_ip}")
    print(f"Frames:      {num_frames}")
    print(f"UDP Port:    {udp_port}")
    print(f"Frame Size:  {FRAME_SIZE:,} bytes ({FRAME_SIZE // 1024} KB)")
    print(f"Expected:    {num_frames * FRAME_SIZE:,} bytes total")
    print(f"=" * 50)

    # Create UDP socket first (must be listening before triggering stream)
    udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    # Increase receive buffer size for high throughput
    try:
        udp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024 * 1024)  # 1MB
    except:
        pass
    
    #udp_sock.bind(('0.0.0.0', udp_port))
    udp_sock.bind(('172.16.1.2', udp_port))
    udp_sock.settimeout(args.timeout)
    print(f"Listening on UDP port {udp_port}...")

    # Send HTTP request to trigger UDP streaming
    print(f"Requesting stream from http://{pico_ip}/streamcudp?frames={num_frames}&port={udp_port}")
    
    tcp_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    tcp_sock.settimeout(10.0)
    
    try:
        tcp_sock.connect((pico_ip, 80))
        request = f"GET /streamcudp?frames={num_frames}&port={udp_port} HTTP/1.1\r\n"
        request += f"Host: {pico_ip}\r\n"
        request += "Connection: close\r\n"
        request += "\r\n"
        tcp_sock.send(request.encode())
        
        # Read HTTP response header
        response = b""
        while b"\r\n\r\n" not in response:
            chunk = tcp_sock.recv(1024)
            if not chunk:
                break
            response += chunk
        
        if b"200 OK" in response:
            print("Server acknowledged, starting UDP receive...")
            tcp_sock.close()          # <-- close immediately
            tcp_sock = None
        else:
            print(f"Unexpected response: {response[:200]}")
            return 1
            
    except Exception as e:
        print(f"Failed to connect to Pico: {e}")
        return 1

    # Statistics
    frames_received = 0
    total_bytes = 0
    packets_received = 0
    packets_with_data = 0
    
    # Frame reassembly tracking
    current_frame_num = -1
    current_frame_size = 0
    current_frame_bytes = 0
    frames_complete = 0
    frames_partial = 0
    
    # Per-frame chunk tracking
    chunks_expected = 0
    chunks_received = set()
    
    start_time = time.time()
    last_progress = start_time
    
    # Receive loop
    try:
        while True:
            try:
                data, addr = udp_sock.recvfrom(2048)
                packets_received += 1
            except socket.timeout:
                print("\nTimeout waiting for UDP data")
                break
            
            # Check packet type
            if data.startswith(b'F:'):
                # Frame header: "F:frame_num:frame_size\n"
                try:
                    parts = data.decode().strip().split(':')
                    new_frame_num = int(parts[1])
                    new_frame_size = int(parts[2])
                    
                    # Check if previous frame was complete
                    if current_frame_num >= 0:
                        if current_frame_bytes >= current_frame_size * 0.95:  # 95% threshold
                            frames_complete += 1
                        else:
                            frames_partial += 1
                            if verbose:
                                pct = 100 * current_frame_bytes / current_frame_size if current_frame_size else 0
                                print(f"  Frame {current_frame_num}: {current_frame_bytes}/{current_frame_size} ({pct:.1f}%)")
                    
                    # Start new frame
                    current_frame_num = new_frame_num
                    current_frame_size = new_frame_size
                    current_frame_bytes = 0
                    chunks_expected = (new_frame_size + 1399) // 1400
                    chunks_received.clear()
                    frames_received += 1
                    
                except (ValueError, IndexError) as e:
                    if verbose:
                        print(f"  Bad header: {data[:50]}")
                        
            elif data.startswith(b'END:'):
                # End marker: "END:frames:bytes:elapsed_ms:mbps\n"
                print(f"\nReceived END marker: {data.decode().strip()}")
                break
                
            elif len(data) > 1:
                # Data packet: [chunk_num][data...]
                chunk_num = data[0]
                chunk_data = data[1:]
                chunk_size = len(chunk_data)
                
                packets_with_data += 1
                total_bytes += chunk_size
                current_frame_bytes += chunk_size
                chunks_received.add(chunk_num)
            
            # Progress update every second
            now = time.time()
            if now - last_progress >= 1.0:
                elapsed = now - start_time
                mbps = (total_bytes * 8) / (elapsed * 1_000_000) if elapsed > 0 else 0
                print(f"  Progress: {frames_received} frames, {total_bytes:,} bytes, {mbps:.2f} Mbps")
                last_progress = now
                
    except KeyboardInterrupt:
        print("\nInterrupted by user")
    
    # Final stats
    elapsed = time.time() - start_time
    mbps = (total_bytes * 8) / (elapsed * 1_000_000) if elapsed > 0 else 0
    
    # Check last frame
    if current_frame_num >= 0 and current_frame_bytes > 0:
        if current_frame_bytes >= current_frame_size * 0.95:
            frames_complete += 1
        else:
            frames_partial += 1
    
    print()
    print("=" * 50)
    print("UDP Streaming Results")
    print("=" * 50)
    print(f"Frames received:    {frames_received}")
    print(f"  - Complete:       {frames_complete}")
    print(f"  - Partial:        {frames_partial}")
    print(f"Total bytes:        {total_bytes:,}")
    print(f"Expected bytes:     {num_frames * FRAME_SIZE:,}")
    print(f"Elapsed time:       {elapsed:.2f} s")
    print(f"Throughput:         {mbps:.2f} Mbps")
    print(f"Packets received:   {packets_received}")
    print(f"  - With data:      {packets_with_data}")
    print(f"  - Headers/ctrl:   {packets_received - packets_with_data}")
    
    # Calculate packet loss estimate
    expected_packets = num_frames * ((FRAME_SIZE + 1399) // 1400)  # ~110 packets per frame
    if expected_packets > 0:
        loss_pct = 100 * (1 - packets_with_data / expected_packets)
        print(f"Estimated loss:     {loss_pct:.1f}%")
    
    print("=" * 50)
    
    # Read any remaining TCP response (server stats)
    try:
        tcp_sock.settimeout(2.0)
        remaining = tcp_sock.recv(1024)
        if remaining:
            print(f"Server stats: {remaining.decode().strip()}")
    except:
        pass
    
    # Comparison note
    print()
    print("Compare with TCP streaming:")
    print(f"  curl -s http://{pico_ip}/streamc | pv > /dev/null")
    print(f"  (Previous TCP result: ~9.7 Mbps)")
    
    # Cleanup
    if tcp_sock is not None:
        tcp_sock.close()
    udp_sock.close()
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
