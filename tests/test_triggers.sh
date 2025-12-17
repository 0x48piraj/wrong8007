#!/usr/bin/env bash
# tests/test_triggers.sh
# Smoke test for wrong8007 triggers (keyboard, USB, network)

set -euo pipefail

MODULE_NAME="wrong8007.ko"
TEST_SCRIPT="$(pwd)/tests/test_exec.sh"

echo "=== Trigger test: Keyboard ==="
PHRASE="nuke"
EXEC="$TEST_SCRIPT"
sudo make load PHRASE="$PHRASE" EXEC="$EXEC"
sleep 2
sudo make unload
echo "Keyboard trigger smoke test passed"

echo "=== Trigger test: USB ==="
USB_DEVICES="1234:5678:any"
WHITELIST=1
sudo make load USB_DEVICES="$USB_DEVICES" WHITELIST="$WHITELIST" EXEC="$EXEC"
sleep 2
sudo make unload
echo "USB trigger smoke test passed"

echo "=== Trigger test: Network (MAC) ==="
MATCH_MAC="aa:bb:cc:dd:ee:ff"
sudo make load MATCH_MAC="$MATCH_MAC" EXEC="$EXEC"
sleep 2
sudo make unload
echo "Network MAC trigger smoke test passed"

echo "=== Trigger test: Network (IP + Magic Packet) ==="
MATCH_IP="192.168.1.1"
MATCH_PORT=1234
MATCH_PAYLOAD="MAGIC"
sudo make load MATCH_IP="$MATCH_IP" MATCH_PORT="$MATCH_PORT" MATCH_PAYLOAD="$MATCH_PAYLOAD" EXEC="$EXEC"
sleep 2
sudo make unload
echo "Network IP/port trigger smoke test passed"

echo "=== All trigger smoke tests completed successfully ==="
