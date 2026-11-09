// SPDX-License-Identifier: GPL-2.0
/*
 * wrong8007: parameter parsers
 *
 * Strict string parsers for trigger parameters. They hold no module state.
 *
 * Copyright (c) 2023, 03C0 (https://03c0.net/)
 */

#ifndef WB_PARSE_H
#define WB_PARSE_H

#include <linux/types.h>
#include <linux/if_ether.h>

enum wb_usb_event {
    WB_USB_INSERT,
    WB_USB_EJECT,
    WB_USB_ANY,
};

struct wb_usb_rule {
    u16 vid;
    u16 pid;
    enum wb_usb_event event;
};

/*
 * Parses one USB rule (VID:PID[:EVENT]).
 *
 * VID and PID are 1 to 4 hex digits after an optional 0x prefix. EVENT is
 * insert, eject or any (case-sensitive) and defaults to any. Surrounding
 * whitespace and trailing characters are refused.
 *
 * Returns 0 or -EINVAL.
 */
int wb_parse_usb_rule(const char *s, struct wb_usb_rule *out);

/*
 * Parses a MAC address of six hex pairs. The pairs are contiguous
 * ("aabbccddeeff") or joined by one consistent ':' or '-'.
 *
 * Returns 0 or -EINVAL.
 */
int wb_parse_mac(const char *s, u8 out[ETH_ALEN]);

#endif /* WB_PARSE_H */
