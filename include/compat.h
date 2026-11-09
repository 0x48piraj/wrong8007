// SPDX-License-Identifier: GPL-2.0
/*
 * wrong8007: kernel compatibility helpers
 *
 * Copyright (c) 2023, 03C0 (https://03c0.net/)
 */

#ifndef WB_COMPAT_H
#define WB_COMPAT_H

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/inet.h>

/* Newer kernels declare hex_to_bin() in <linux/hex.h> */
#if __has_include(<linux/hex.h>)
#include <linux/hex.h>
#endif

static inline void wb_timer_delete_sync(struct timer_list *timer)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 15, 0)
    timer_delete_sync(timer);
#else
    del_timer_sync(timer);
#endif
}

/*
 * Parse an IPv4 address into network byte order.
 */
static inline bool wb_parse_ipv4(const char *ip, __be32 *out)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38)
    return ip && in4_pton(ip, -1, (u8 *)out, '\0', NULL);
#else
    if (!ip)
        return false;

    *out = in_aton(ip);
    return *out != 0;
#endif
}

#endif /* WB_COMPAT_H */
