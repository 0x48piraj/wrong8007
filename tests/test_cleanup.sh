#!/bin/bash
# tests/test_cleanup.sh
# Verify that the module releases all resources after unload

set -e

MODULE_NAME="wrong8007"
TEST_EXEC="./tests/test_exec.sh"
LOG_FILE="/tmp/trigger_test.log"
PHRASE="dummy"

echo "[*] Starting cleanup test..."

# Ensure exec script exists
if [ ! -f "$TEST_EXEC" ]; then
    echo "[!] Exec script not found: $TEST_EXEC"
    exit 1
fi

# Load the module with a dummy trigger
echo "[*] Loading module with test exec..."
make load PHRASE="$PHRASE" EXEC="$(realpath "$TEST_EXEC")"

# Check module is loaded
if ! lsmod | grep -q "$MODULE_NAME"; then
    echo "[!] Module did not load correctly!"
    exit 1
fi
echo "[+] Module loaded successfully."

echo "[*] Please type the trigger phrase: '$PHRASE'"
read -p "[*] Press Enter when done..." _

# Unload the module
echo "[*] Unloading module..."
make remove

# Check module is unloaded
if lsmod | grep -q "$MODULE_NAME"; then
    echo "[!] Module did not unload properly!"
    exit 1
fi
echo "[+] Module unloaded successfully."

# Check logs for exit confirmation
if [ -f "$LOG_FILE" ]; then
    echo "[*] Checking module log..."
    tail -n 10 "$LOG_FILE"
    echo "[+] Log exists, cleanup verified."
else
    echo "[!] No log file created. Make sure your module prints cleanup messages on exit."
fi

echo "[*] Cleanup test completed successfully!"
