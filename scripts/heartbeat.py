import socket
import time
import signal
import sys

# Config: update these to the correct host and port
HEARTBEAT_IP = "192.168.1.7"
HEARTBEAT_PORT = 12345
HEARTBEAT_INTERVAL = 10  # seconds

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def send_heartbeat():
    try:
        sock.sendto(b"heartbeat", (HEARTBEAT_IP, HEARTBEAT_PORT))
        print(f"Sent heartbeat to {HEARTBEAT_IP}:{HEARTBEAT_PORT}")
    except Exception as e:
        print(f"Failed to send heartbeat: {e}")

def signal_handler(sig, frame):
    print("\nHeartbeat stopped by user. Kernel will detect timeout soon.")
    sock.close()
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

print(f"Starting heartbeat to {HEARTBEAT_IP}:{HEARTBEAT_PORT} every {HEARTBEAT_INTERVAL}s")
while True:
    send_heartbeat()
    time.sleep(HEARTBEAT_INTERVAL)
