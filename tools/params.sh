#!/bin/sh
#
# wrong8007 parameter validation and local module loading.
#
# Usage:
#   params.sh check CONFIG PARAMS
#   params.sh insmod CONFIG PARAMS MODULE.ko
#
# CONFIG is the build profile. PARAMS is the module options file.
#
# Validation mirrors the module's parameter parsing and rejects
# parameters the kernel would otherwise silently ignore.
#
# Trigger phrases and payload contents are never printed.

set -eu

LC_ALL=C
export LC_ALL

MODE="${1:-}"
CONFIG="${2:-}"
PARAMS="${3:-}"
MODULE="${4:-}"

usage() {
    echo "usage: params.sh check CONFIG PARAMS"
    echo "       params.sh insmod CONFIG PARAMS MODULE.ko"
    exit 2
}

case "$MODE" in
    check)
        if [ -z "$CONFIG" ] || [ -z "$PARAMS" ]; then
            usage
        fi
        ;;
    insmod)
        if [ -z "$CONFIG" ] || [ -z "$PARAMS" ] || [ -z "$MODULE" ]; then
            usage
        fi
        ;;
    *) usage ;;
esac

[ -r "$CONFIG" ] || {
    echo "error: build profile not found: $CONFIG (run 'make defconfig')"
    exit 1
}

[ -r "$PARAMS" ] || {
    echo "error: parameter file not readable: $PARAMS"
    echo "create it with 'make params-init', then edit it with sudoedit."
    exit 1
}

fail=0

err() {
    echo "error: $*"
    fail=1
}

warn() {
    echo "warning: $*"
}

get_bool() {
    if grep -q "^CONFIG_$1=y$" "$CONFIG"; then
        echo y
    else
        echo n
    fi
}

# Tokenize wrong8007 options, honoring quoted values and continuations
tokenize() {
    awk '
    {
        if (sub(/\\$/, "")) { buf = buf $0 " "; next }
        $0 = buf $0
        buf = ""
        if ($1 != "options" || $2 != "wrong8007")
            next
        sub(/^[ \t]*options[ \t]+wrong8007[ \t]*/, "")
        s = $0
        tok = ""; have = 0; inq = 0
        for (i = 1; i <= length(s); i++) {
            c = substr(s, i, 1)
            if (c == "\"") { inq = !inq; tok = tok c; have = 1; continue }
            if (!inq && (c == " " || c == "\t")) {
                if (have) print tok
                tok = ""; have = 0
                continue
            }
            tok = tok c; have = 1
        }
        if (inq) { print "unterminated quote in options line" > "/dev/stderr"; bad = 1 }
        if (have) print tok
    }
    END { exit bad }
    ' "$PARAMS"
}

# Match the kernel's dotted-quad IPv4 parsing rules
is_ipv4() {
    s="$1"

    case "$s" in
        ''|*[!0-9.]*|.*|*.|*..*) return 1 ;;
    esac

    dots=$(printf '%s' "$s" | tr -cd '.')
    [ "${#dots}" -eq 3 ] || return 1

    o1=${s%%.*}
    r=${s#*.}
    o2=${r%%.*}
    r=${r#*.}
    o3=${r%%.*}
    o4=${r#*.}

    for o in "$o1" "$o2" "$o3" "$o4"; do
        case "$o" in
            ''|[0-9][0-9][0-9][0-9]*) return 1 ;;
        esac
        [ "$o" -le 255 ] || return 1
    done

    return 0
}

is_uint() {
    case "$1" in
        ''|*[!0-9]*) return 1 ;;
    esac
    # Refuse values too long for 32-bit sh arithmetic or the kernel's uint
    [ "${#1}" -le 9 ]
}

# Accept 1-4 hexadecimal digits, optionally prefixed with 0x
is_hex_u16() {
    s="$1"

    case "$s" in
        0[xX]*) h=${s#0[xX]} ;;
        *) h=$s ;;
    esac

    case "$h" in
        ''|*[!0-9a-fA-F]*) return 1 ;;
    esac

    [ "${#h}" -le 4 ]
}

# Validate a VID:PID[:EVENT] rule
is_usb_rule() {
    rule="$1"

    # Match the kernel's 63-byte rule limit
    [ "${#rule}" -ge 64 ] && return 1

    vid=${rule%%:*}
    rest=${rule#*:}
    [ "$rest" = "$rule" ] && return 1

    case "$rest" in
        *:*)
            pid=${rest%%:*}
            ev=${rest#*:}
            ;;
        *)
            pid=$rest
            ev=
            ;;
    esac

    is_hex_u16 "$vid" || return 1
    is_hex_u16 "$pid" || return 1

    case "$ev" in
        ''|insert|eject|any) ;;
        *) return 1 ;;
    esac

    return 0
}

check_usb() {
    rules="$1"
    count=0
    old_ifs=$IFS
    IFS=','
    for rule in $rules; do
        IFS=$old_ifs
        rule=$(printf '%s' "$rule" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
        if [ -n "$rule" ]; then
            count=$((count + 1))
            if [ "$count" -gt 16 ]; then
                err "too many USB rules (max 16)"
                IFS=$old_ifs
                return
            fi
            is_usb_rule "$rule" || err "invalid USB rule: $rule"
        fi
        IFS=','
    done
    IFS=$old_ifs
    [ "$count" -gt 0 ] || err "usb_devices is set but contains no rules"
}

kbd=$(get_bool WRONG8007_KEYBOARD)
usb=$(get_bool WRONG8007_USB)
net=$(get_bool WRONG8007_NETWORK)

tokens=$(tokenize) || {
    echo ""
    echo "Wrong Boot parameters are invalid."
    exit 1
}

exec_path=
phrase=
usb_devices=
whitelist=
match_mac=
match_ip=
match_port=
match_payload=
hb_host=
hb_interval=
hb_timeout=

# Reject parameters for disabled trigger backends
need() {
    if [ "$2" != y ]; then
        err "'$1' is set, but the $3 trigger is not compiled in (CONFIG_WRONG8007_$4); the kernel would ignore it"
    fi
}

while IFS= read -r tok; do
    [ -n "$tok" ] || continue

    case "$tok" in
        *=*)
            key=${tok%%=*}
            val=${tok#*=}
            has_val=1
            ;;
        *)
            key=$tok
            val=
            has_val=0
            ;;
    esac
    key=$(printf '%s' "$key" | tr -d '"')
    val=$(printf '%s' "$val" | tr -d '"')

    if [ "$has_val" -eq 0 ] && [ "$key" != whitelist ]; then
        err "parameter '$key' has no value"
        continue
    fi

    case "$key" in
        exec) exec_path=$val ;;
        phrase) need "$key" "$kbd" keyboard KEYBOARD; phrase=$val ;;
        usb_devices) need "$key" "$usb" USB USB; usb_devices=$val ;;
        whitelist) need "$key" "$usb" USB USB; whitelist=${val:-y} ;;
        match_mac) need "$key" "$net" network NETWORK; match_mac=$val ;;
        match_ip) need "$key" "$net" network NETWORK; match_ip=$val ;;
        match_port) need "$key" "$net" network NETWORK; match_port=$val ;;
        match_payload) need "$key" "$net" network NETWORK; match_payload=$val ;;
        heartbeat_host) need "$key" "$net" network NETWORK; hb_host=$val ;;
        heartbeat_interval) need "$key" "$net" network NETWORK; hb_interval=$val ;;
        heartbeat_timeout) need "$key" "$net" network NETWORK; hb_timeout=$val ;;
        *) err "unknown parameter '$key'; the kernel would ignore it" ;;
    esac
done <<EOF
$tokens
EOF

# Validate the executable on the host
if [ -z "$exec_path" ]; then
    err "exec is required"
elif [ "${exec_path#/}" = "$exec_path" ]; then
    err "exec must be an absolute path: $exec_path"
elif [ "${#exec_path}" -gt 4096 ]; then
    err "exec path too long (max 4096)"
elif [ ! -e "$exec_path" ]; then
    err "exec path does not exist: $exec_path"
elif [ ! -f "$exec_path" ]; then
    err "exec path is not a regular file: $exec_path"
elif [ ! -x "$exec_path" ]; then
    err "exec path is not executable: $exec_path"
else
    resolved=$(realpath -e -- "$exec_path")
    if [ "$resolved" != "$exec_path" ]; then
        err "exec must be the canonical path:"
        err "       configured: $exec_path"
        err "       realpath:   $resolved"
    fi
fi

armed=

if [ -n "$phrase" ]; then
    # Length in bytes
    plen=$(printf '%s' "$phrase" | wc -c)
    if [ "$plen" -gt 128 ]; then
        err "phrase too long (max 128 bytes)"
    fi
    [ "$kbd" = y ] && armed="$armed keyboard"
fi

if [ -n "$usb_devices" ]; then
    check_usb "$usb_devices"
    [ "$usb" = y ] && armed="$armed usb"
fi

if [ -n "$whitelist" ]; then
    case "$whitelist" in
        y|Y|1|n|N|0|on|off) ;;
        *) err "whitelist must be one of y, n, 1, 0, on, off" ;;
    esac
    [ -n "$usb_devices" ] || warn "whitelist is set but usb_devices is empty; the USB trigger stays disabled"
fi

if [ -n "$match_mac" ]; then
    # Match the MAC formats accepted by lib/parse.c
    h2='[0-9a-fA-F][0-9a-fA-F]'
    # $h2 stays unquoted on purpose as a glob character class
    # shellcheck disable=SC2254
    case "$match_mac" in
        $h2$h2$h2$h2$h2$h2) ;;
        $h2:$h2:$h2:$h2:$h2:$h2) ;;
        $h2-$h2-$h2-$h2-$h2-$h2) ;;
        *) err "invalid MAC address: $match_mac (use aa:bb:cc:dd:ee:ff)" ;;
    esac
fi

if [ -n "$match_ip" ]; then
    is_ipv4 "$match_ip" || err "invalid IPv4 address: $match_ip"
fi

if [ -n "$match_port" ]; then
    if ! is_uint "$match_port" || [ "$match_port" -gt 65535 ]; then
        err "match_port must be 0-65535"
    fi
fi

if [ -n "$match_payload" ] && [ "${#match_payload}" -gt 512 ]; then
    err "match_payload too long (max 512 bytes)"
fi

if [ -n "$hb_host" ]; then
    is_ipv4 "$hb_host" || err "invalid heartbeat_host IPv4 address: $hb_host"
fi

interval=${hb_interval:-10}
timeout=${hb_timeout:-30}
if ! is_uint "$interval" || ! is_uint "$timeout"; then
    err "heartbeat_interval and heartbeat_timeout must be whole numbers below 1000000000"
elif [ -n "$hb_host" ]; then
    [ "$interval" -ge 1 ] || err "heartbeat_interval must be >= 1 second"
    [ "$timeout" -gt "$interval" ] || err "heartbeat_timeout must be greater than heartbeat_interval"
elif [ -n "$hb_interval$hb_timeout" ]; then
    warn "heartbeat_interval/heartbeat_timeout are set but heartbeat_host is empty; they have no effect"
fi

if [ "$net" = y ]; then
    if [ -n "$match_mac$match_ip$match_payload$hb_host" ] ||
       { [ -n "$match_port" ] && [ "$match_port" != 0 ]; }; then
        armed="$armed network"
    fi
fi

if [ -z "$armed" ]; then
    err "no trigger is armed; the module would load and never fire"
fi

# Reject module options in modprobe.d
for f in /etc/modprobe.d/*.conf; do
    [ -f "$f" ] || continue
    if grep -Eq '^[[:space:]]*options[[:space:]]+wrong8007([[:space:]]|$)' "$f"; then
        err "$f sets wrong8007 options and initramfs tools copy it into the boot image; move them to $PARAMS"
    fi
done

if [ "$fail" -ne 0 ]; then
    echo ""
    echo "Wrong Boot parameters are invalid."
    exit 1
fi

echo "Wrong Boot parameters: valid (armed:$armed)."

[ "$MODE" = insmod ] || exit 0

# Load the validated local module
[ -f "$MODULE" ] || {
    echo "error: module not found: $MODULE"
    exit 1
}

set --
while IFS= read -r tok; do
    [ -n "$tok" ] && set -- "$@" "$tok"
done <<EOF
$tokens
EOF

insmod "$MODULE" "$@"
