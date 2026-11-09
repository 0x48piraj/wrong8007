// SPDX-License-Identifier: GPL-2.0
/*
 * wrong8007: parameter parsers
 *
 * Copyright (c) 2023, 03C0 (https://03c0.net/)
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <compat.h>
#include <parse.h>

/*
 * Parses 1 to 4 hex digits after an optional 0x prefix into a u16.
 */
static int parse_hex16(const char *s, size_t len, u16 *out)
{
    unsigned int v = 0;
    size_t i = 0;

    if (len >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        i = 2;

    if (len - i < 1 || len - i > 4)
        return -EINVAL;

    for (; i < len; i++) {
        int d = hex_to_bin(s[i]);

        if (d < 0)
            return -EINVAL;
        v = (v << 4) | d;
    }

    *out = (u16)v;
    return 0;
}

int wb_parse_usb_rule(const char *s, struct wb_usb_rule *out)
{
    const char *c1, *c2;
    const char *event;
    struct wb_usb_rule r;

    if (!s || !out)
        return -EINVAL;

    c1 = strchr(s, ':');
    if (!c1)
        return -EINVAL;

    c2 = strchr(c1 + 1, ':');

    if (parse_hex16(s, c1 - s, &r.vid))
        return -EINVAL;

    if (parse_hex16(c1 + 1, c2 ? (size_t)(c2 - c1 - 1) : strlen(c1 + 1), &r.pid))
        return -EINVAL;

    if (!c2) {
        r.event = WB_USB_ANY;
    } else {
        event = c2 + 1;
        if (!strcmp(event, "insert"))
            r.event = WB_USB_INSERT;
        else if (!strcmp(event, "eject"))
            r.event = WB_USB_EJECT;
        else if (!strcmp(event, "any"))
            r.event = WB_USB_ANY;
        else
            return -EINVAL;
    }

    *out = r;
    return 0;
}

int wb_parse_mac(const char *s, u8 out[ETH_ALEN])
{
    u8 mac[ETH_ALEN];
    size_t len;
    size_t step;
    char sep = 0;
    int i;

    if (!s || !out)
        return -EINVAL;

    len = strlen(s);
    if (len == 2 * ETH_ALEN) {
        step = 2;
    } else if (len == 3 * ETH_ALEN - 1) {
        step = 3;
        sep = s[2];
        if (sep != ':' && sep != '-')
            return -EINVAL;
    } else {
        return -EINVAL;
    }

    for (i = 0; i < ETH_ALEN; i++) {
        const char *p = s + i * step;
        int hi = hex_to_bin(p[0]);
        int lo = hex_to_bin(p[1]);

        if (hi < 0 || lo < 0)
            return -EINVAL;
        if (sep && i < ETH_ALEN - 1 && p[2] != sep)
            return -EINVAL;

        mac[i] = (u8)((hi << 4) | lo);
    }

    memcpy(out, mac, ETH_ALEN);
    return 0;
}
