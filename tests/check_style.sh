#!/bin/bash
# tests/check_style.sh
# Run static analysis and kernel coding style checks for wrong8007

set -euo pipefail

echo "=== Running checkpatch.pl (kernel coding style) ==="
if [ ! -f scripts/checkpatch.pl ]; then
    echo "Downloading checkpatch.pl from kernel tree..."
    wget https://raw.githubusercontent.com/torvalds/linux/master/scripts/checkpatch.pl -O scripts/checkpatch.pl
    chmod +x scripts/checkpatch.pl
fi

# Run checkpatch on all C source files
scripts/checkpatch.pl --no-tree --terse --no-summary -f *.c */*.c

echo "=== Running smatch (semantic checks / NULL pointer checks) ==="
if ! command -v smatch &> /dev/null; then
    echo "smatch not found! Install smatch before running this script."
else
    smatch --null --enable-uninit .
fi

echo "=== Running cppcheck (general C/C++ safety issues) ==="
if ! command -v cppcheck &> /dev/null; then
    echo "cppcheck not found! Install cppcheck before running this script."
else
    cppcheck --enable=all --inconclusive --quiet .
fi

echo "[+] Static analysis completed!"
