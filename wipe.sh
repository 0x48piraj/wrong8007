#!/bin/bash

# title: Wipe Disks
# description: Runs wipefs -af, sgdisk -Z and dd's the bootsector of every disk

find /dev -maxdepth 1 -type b -regextype "posix-extended" -regex '.*/(sd[a-z]$|nvme[0-9]n[0-9])$'|xargs -rn1 -P1 bash -c '
[[ $(command -v wipefs) ]] && { wipefs -af ${0}; }
[[ $(command -v sgdisk) ]] && { sgdisk -Z ${0}; }
[[ $(command -v sg_dd) ]] && { sg_dd if=/dev/zero of=${0} bs=446 count=1; }
'
exit 0
