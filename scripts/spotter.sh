#!/bin/bash

# Lists USB devices showing their VID and PID

echo "Detecting USB flash drives..."

# Loop over all /sys block devices
for dev in /sys/block/sd*; do
    dev_name=$(basename "$dev")

    # Check if it's removable
    if [[ "$(cat "$dev/removable")" != "1" ]]; then
        continue
    fi

    # Check if it's connected via USB
    udev_path=$(udevadm info -q path -n "/dev/$dev_name")
    if [[ "$udev_path" != *usb* ]]; then
        continue
    fi

    # Get detailed udev info
    info=$(udevadm info -a -n "/dev/$dev_name")

    # Extract fields
    model=$(echo "$info" | grep -m1 "ATTRS{model}" | awk -F'==' '{print $2}' | tr -d '" ')
    vendor=$(echo "$info" | grep -m1 "ATTRS{vendor}" | awk -F'==' '{print $2}' | tr -d '" ')
    serial=$(echo "$info" | grep -m1 "ATTRS{serial}" | awk -F'==' '{print $2}' | tr -d '" ')
    vid=$(echo "$info" | grep -m1 "ATTRS{idVendor}" | awk -F'==' '{print $2}' | tr -d '" ')
    pid=$(echo "$info" | grep -m1 "ATTRS{idProduct}" | awk -F'==' '{print $2}' | tr -d '" ')

    echo "USB Drive: /dev/$dev_name"
    echo "  Vendor: $vendor"
    echo "  Model : $model"
    echo "  Serial: $serial"
    echo "  VID:PID = $vid:$pid"
    echo ""
done
