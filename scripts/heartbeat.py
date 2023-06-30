#!/usr/bin/env python3
import socket
import time
import signal
import sys
import argparse

def parse_args():
    parser = argparse.ArgumentParser(
        description="Send periodic heartbeat packets over UDP. Used to keep the wrong8007 kernel module from triggering."
    )
    parser.add_argument("ip", help="Target IP address (e.g., 192.168.1.7)")
    parser.add_argument("port", type=int, help="Target UDP port (e.g., 12345)")
    parser.add_argument("-i", "--interval", type=int, default=10,
                        help="Heartbeat interval in seconds (default: 10)")
    parser.add_argument("-m", "--message", default="heartbeat",
                        help="Payload message to send (default: 'heartbeat')")
    return parser.parse_args()

def main():
    args = parse_args()
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def signal_handler(sig, frame):
        print("\n[!] Heartbeat stopped by user. Kernel should detect timeout soon.")
        sock.close()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)

    print(f"[+] Starting heartbeat to {args.ip}:{args.port} every {args.interval}s")
    print("    Press Ctrl+C to stop.\n")

    while True:
        try:
            sock.sendto(args.message.encode(), (args.ip, args.port))
            print(f"[>] Sent heartbeat to {args.ip}:{args.port}")
        except Exception as e:
            print(f"[!] Failed to send heartbeat: {e}")
        time.sleep(args.interval)

if __name__ == "__main__":
    main()
