#!/bin/bash
#
# wipe.sh - Emergency disk wipe payload for wrong8007
# Author: 03C0
#
# Description:
#   Wipes partition tables, signatures, and MBR data from all block devices
#   matching /dev/sdX and /dev/nvmeXnY. Intended for rapid erasure.
#
# Tools used:
#   - wipefs (to remove filesystem signatures)
#   - sgdisk (to destroy GPT/MBR structures)
#   - dd (to zero boot sectors)

# Require root
[[ $EUID -ne 0 ]] && exit 1

# Find disks and apply destructive operations
find /dev -maxdepth 1 -type b -regextype "posix-extended" \
  -regex '.*/(sd[a-z]$|nvme[0-9]n[0-9])$' | \
xargs -rn1 -P4 bash -c '
device="$1"
echo "[*] Wiping $device"
[[ $(command -v wipefs) ]] && wipefs -af "$device"
[[ $(command -v sgdisk) ]] && sgdisk -Z "$device"
dd if=/dev/zero of="$device" bs=1M count=4 conv=fsync status=none
' _
