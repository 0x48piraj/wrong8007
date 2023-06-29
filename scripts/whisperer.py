#!/usr/bin/env python3
import socket
import sys

def send_magic_packet(target_ip, target_port, payload=b"MAGIC"):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.sendto(payload, (target_ip, target_port))
    sock.close()
    print(f"Sent magic payload to {target_ip}:{target_port}")

def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <target_ip> <target_port> [payload]")
        sys.exit(1)

    ip = sys.argv[1]
    port = int(sys.argv[2])
    payload = sys.argv[3].encode() if len(sys.argv) > 3 else b"MAGIC"

    send_magic_packet(ip, port, payload)

if __name__ == "__main__":
    main()
