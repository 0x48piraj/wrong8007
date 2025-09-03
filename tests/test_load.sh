#!/usr/bin/env bash
# tests/test_load.sh
# Simple smoke test: load and unload the wrong8007 module

set -euo pipefail

MODULE_NAME="wrong8007.ko"
PHRASE="dummy"

echo "=== Runtime Test: Loading module ==="
if sudo insmod $MODULE_NAME phrase="$PHRASE" exec="/bin/true"; then
    echo "[+] Module loaded successfully"
else
    echo "[!] Failed to load module"
    exit 1
fi

echo "=== Checking module list ==="
lsmod | grep -q wrong8007 && echo "[*] Module appears in lsmod"

echo "=== Runtime Test: Unloading module ==="
if sudo rmmod wrong8007; then
    echo "[+] Module unloaded successfully"
else
    echo "[!] Failed to unload module"
    exit 1
fi

echo "=== Smoke test completed successfully ==="
