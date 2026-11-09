#!/bin/sh
#
# Checks a wrong8007 .config build profile.
#
# The build profile selects compiled-in features and holds no secrets.
#
# usage: validate-config.sh [--summary] CONFIG

set -eu

SUMMARY=0
if [ "${1:-}" = --summary ]; then
    SUMMARY=1
    shift
fi

CONFIG="${1:?usage: validate-config.sh [--summary] CONFIG}"

LC_ALL=C
export LC_ALL

[ -r "$CONFIG" ] || {
    echo "error: configuration file not found: $CONFIG"
    exit 1
}

get_bool() {
    if grep -q "^CONFIG_$1=y$" "$CONFIG"; then
        echo y
    else
        echo n
    fi
}

onoff() {
    if [ "$1" = y ]; then
        echo on
    else
        echo off
    fi
}

# Master switch; a disabled module has nothing to check
if [ "$(get_bool WRONG8007)" != y ]; then
    echo "Wrong Boot is disabled in this profile; the build produces no module."
    exit 0
fi

kbd=$(get_bool WRONG8007_KEYBOARD)
usb=$(get_bool WRONG8007_USB)
net=$(get_bool WRONG8007_NETWORK)
wl=$(get_bool WRONG8007_WHITELIST)
dbg=$(get_bool WRONG8007_DEBUG)

if [ "$SUMMARY" -eq 1 ]; then
    echo "Wrong Boot build profile:"
    echo "  keyboard trigger: $(onoff "$kbd")"
    if [ "$usb" = y ]; then
        if [ "$wl" = y ]; then
            echo "  usb trigger:      on (default policy: whitelist)"
        else
            echo "  usb trigger:      on (default policy: blacklist)"
        fi
    else
        echo "  usb trigger:      off"
    fi
    echo "  network trigger:  $(onoff "$net")"
    echo "  debug logging:    $(onoff "$dbg")"
    echo "  trigger values are load-time parameters; see configs/wrong8007.conf.example"
fi

if [ "$kbd$usb$net" = "nnn" ]; then
    echo "error: no trigger is enabled (enable at least one of keyboard, USB or network)"
    echo ""
    echo "Wrong Boot build configuration is invalid."
    exit 1
fi

[ "$SUMMARY" -eq 1 ] || echo "Wrong Boot build configuration: valid."
