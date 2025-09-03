#!/bin/bash
# tests/test_cleanup.sh
# Verify that the wrong8007 module releases all resources after unload

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

# Check for leftover kobjects in sysfs
echo "[*] Checking for leftover kobjects in /sys/module..."
if [ -d "/sys/module/$MODULE_NAME" ]; then
    echo "[!] /sys/module/$MODULE_NAME still exists after unload!"
    exit 1
else
    echo "[+] No leftover kobjects."
fi

# Automatically detect custom /proc/sys/kernel entries created
echo "[*] Detecting leftover /proc/sys/kernel entries..."
CUSTOM_PROC_ENTRIES=$(grep -l "$MODULE_NAME" /proc/sys/kernel/* 2>/dev/null || true)
if [ -n "$CUSTOM_PROC_ENTRIES" ]; then
    echo "[!] Leftover proc entries detected:"
    echo "$CUSTOM_PROC_ENTRIES"
    exit 1
else
    echo "[+] No leftover /proc/sys/kernel entries."
fi

# Inspect dmesg for memory leaks or errors
echo "[*] Checking dmesg for module-related errors..."
if sudo dmesg | grep -i "$MODULE_NAME" | grep -iE "leak|error"; then
    echo "[!] Errors or warnings found in dmesg!"
    exit 1
else
    echo "[+] No errors found in dmesg."
fi

# Check for leftover workqueues
if [ -f /proc/workqueue ]; then
    echo "[*] Checking /proc/workqueue..."
    if grep -q "$MODULE_NAME" /proc/workqueue; then
        echo "[!] Found leftover workqueues!"
        exit 1
    else
        echo "[+] No leftover workqueues."
    fi
fi

# Check for leftover timers
if [ -r /proc/timer_list ]; then
    echo "[*] Checking /proc/timer_list..."
    if grep -q "$MODULE_NAME" /proc/timer_list; then
        echo "[!] Found leftover timers!"
        exit 1
    else
        echo "[+] No leftover timers."
    fi
fi

echo "[+] Cleanup test completed successfully!"
