#!/bin/bash

# Lists USB devices showing their VID and PID

lsusb | while read -r line; do
    # Extract the ID field, which is VID:PID
    id=$(echo "$line" | awk '{for(i=1;i<=NF;i++) if ($i=="ID") print $(i+1)}')
    vid=${id%%:*}
    pid=${id##*:}

    # Extract the name after the ID
    name=$(echo "$line" | sed -n 's/.*ID [0-9a-fA-F]\{4\}:[0-9a-fA-F]\{4\} \(.*\)/\1/p')

    echo "USB Device: \"$name\" (VID: $vid, PID: $pid)"
done
