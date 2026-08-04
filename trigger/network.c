// SPDX-License-Identifier: GPL-2.0
/*
 * wrong8007: network trigger
 *
 * Copyright (c) 2023, 03C0 (https://03c0.net/)
 */

#include <linux/netdevice.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/etherdevice.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/inet.h>
#include <linux/version.h>

#include <wrong8007.h>
#include <compat.h>

#define PAYLOAD_SCAN_WIN 512

static char *match_mac;
static char *match_ip;
static int match_port;
static char *match_payload;

static char *heartbeat_host;
static unsigned int heartbeat_interval = 10;
static unsigned int heartbeat_timeout = 30;

/* Parsed trigger configuration */
static u8 mac_bytes[ETH_ALEN];
static __be32 match_ip_addr = 0;
static __be32 heartbeat_ip_addr = 0;
static size_t payload_len = 0;

/* Netfilter hook */
static struct nf_hook_ops nfho;

/* Tracks netfilter hook ownership across init/exit */
static bool hook_registered;

/* Heartbeat state */
static struct timer_list hb_timer;
static unsigned long last_seen_jiffies;

static DEFINE_SPINLOCK(hb_lock);

/*
 * Parse a MAC address into binary form.
 *
 * Accepts colon-separated, dash-separated and contiguous hexadecimal
 * representations.
 */
static bool parse_mac(const char *s, u8 *out)
{
    int i = 0;
    int hi = -1; /* High nibble accumulator */

    if (!s || !out)
        return false;

    while (*s && i < ETH_ALEN) {
        char c = *s++;

        int nibble;
        if (c >= '0' && c <= '9')
            nibble = c - '0';
        else if (c >= 'a' && c <= 'f')
            nibble = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            nibble = c - 'A' + 10;
        else
            continue; /* Ignore non-hex characters */

        if (hi == -1) {
            hi = nibble;
        } else {
            out[i++] = (u8)((hi << 4) | nibble);
            hi = -1;
        }
    }

    return i == ETH_ALEN && hi == -1;
}

/*
 * Convert a packet pointer into an skb-relative offset.
 */
static inline size_t skb_offset(const struct sk_buff *skb, const void *ptr)
{
    return (const u8 *)ptr - (const u8 *)skb->data;
}

/*
 * Monitor heartbeat liveness.
 *
 * Trigger execution once the configured heartbeat expires.
 */
static void hb_timer_fn(struct timer_list *t)
{
    unsigned long now = jiffies;
    unsigned long last;
    unsigned long flags;

    spin_lock_irqsave(&hb_lock, flags);
    last = last_seen_jiffies;
    spin_unlock_irqrestore(&hb_lock, flags);

    if (time_after(now, last + (unsigned long)heartbeat_timeout * HZ)) {
        wb_info("heartbeat timeout reached, scheduling exec\n");
        wrong8007_activate();
    } else {
        mod_timer(&hb_timer, jiffies + (unsigned long)heartbeat_interval * HZ);
    }
}

/*
 * Naive bounded substring search (O(n * m)).
 *
 * Payloads are typically short (MTU-bounded), so a hand-rolled search is sufficient.
 * This is kept intentionally simple, as more complex algorithms (KMP/BM)
 * would add branches and state with no measurable benefit in this use case.
 */
static void *k_memmem(const void *haystack, size_t haystack_len,
                      const void *needle, size_t needle_len)
{
    const u8 *h = haystack;
    size_t i;

    if (!needle_len || haystack_len < needle_len)
        return NULL;

    for (i = 0; i <= haystack_len - needle_len; i++) {
        if (!memcmp(h + i, needle, needle_len))
            return (void *)(h + i);
    }
    return NULL;
}

/*
 * Search a packet payload for the configured magic string.
 *
 * Payload inspection is independent of skb layout and safely spans
 * fragmented buffers.
 */
static bool payload_contains(const struct sk_buff *skb, size_t offset,
                             size_t payload_size, const char *needle,
                             size_t needle_len)
{
    u8 buf[PAYLOAD_SCAN_WIN];
    size_t win = min_t(size_t, payload_size, PAYLOAD_SCAN_WIN);
    size_t advance = win > needle_len ? win - needle_len + 1 : 1;
    size_t pos;

    if (!needle_len || payload_size < needle_len)
        return false;

    for (pos = 0; pos + needle_len <= payload_size; pos += advance) {
        size_t len = min(win, payload_size - pos);

        if (skb_copy_bits(skb, offset + pos, buf, len))
            return false;

        if (k_memmem(buf, len, needle, needle_len))
            return true;

        if (len < win)
            break;
    }

    return false;
}

/*
 * Evaluate incoming packets against the configured network triggers.
 *
 * Matching progresses from L2 through L4, allowing increasingly
 * specific trigger conditions while execution remains owned by
 * the core.
 */
static unsigned int nf_hook_fn(void *priv,
                                struct sk_buff *skb,
                                const struct nf_hook_state *state)
{
    struct ethhdr *eth;
    struct iphdr *iph;
    struct tcphdr *tcph;
    struct udphdr *udph;
    unsigned int payload_size;
    unsigned int iph_len;
    size_t offset;

    if (skb->protocol != htons(ETH_P_IP))
        goto out;

    if (!pskb_may_pull(skb, sizeof(struct iphdr)))
        goto out;

    iph = ip_hdr(skb);

    if (iph->ihl < 5)
        goto out;

    iph_len = iph->ihl * 4;

    if (!pskb_may_pull(skb, iph_len))
        goto out;

    iph = ip_hdr(skb);

    /* Refresh heartbeat liveness before evaluating trigger conditions */
    if (heartbeat_host && iph->saddr == heartbeat_ip_addr) {
        unsigned long flags;
        spin_lock_irqsave(&hb_lock, flags);
        last_seen_jiffies = jiffies;
        spin_unlock_irqrestore(&hb_lock, flags);
    }

    if (match_mac) {
        if (!skb_mac_header_was_set(skb) || skb->mac_len < ETH_HLEN)
            goto out;

        if (!pskb_may_pull(skb, ETH_HLEN))
            goto out;

        iph = ip_hdr(skb);
        eth = eth_hdr(skb);
        if (!ether_addr_equal(mac_bytes, eth->h_source))
            goto out;
    }

    if (match_ip && iph->saddr != match_ip_addr)
        goto out;

    if (match_port || match_payload) {

        if (iph->protocol == IPPROTO_TCP) {
            if (!pskb_may_pull(skb, iph_len + sizeof(struct tcphdr)))
                goto out;

            iph = ip_hdr(skb);
            tcph = (struct tcphdr *)((u8 *)iph + iph_len);

            if (tcph->doff < 5)
                goto out;

            if (!pskb_may_pull(skb, iph_len + tcph->doff * 4))
                goto out;

            iph = ip_hdr(skb);
            tcph = (struct tcphdr *)((u8 *)iph + iph_len);

            if (match_port &&
                ntohs(tcph->source) != match_port &&
                ntohs(tcph->dest) != match_port)
                goto out;

            offset = skb_offset(skb, (u8 *)tcph + tcph->doff * 4);
            if (offset > skb->len)
                goto out;
            payload_size = skb->len - offset;

        } else if (iph->protocol == IPPROTO_UDP) {
            if (!pskb_may_pull(skb, iph_len + sizeof(struct udphdr)))
                goto out;

            iph = ip_hdr(skb);
            udph = (struct udphdr *)((u8 *)iph + iph_len);

            if (ntohs(udph->len) < sizeof(struct udphdr))
                goto out;

            if (match_port &&
                ntohs(udph->source) != match_port &&
                ntohs(udph->dest) != match_port)
                goto out;

            if (!pskb_may_pull(skb, iph_len + ntohs(udph->len)))
                goto out;

            iph = ip_hdr(skb);
            udph = (struct udphdr *)((u8 *)iph + iph_len);

            offset = skb_offset(skb, (u8 *)udph + sizeof(struct udphdr));
            if (offset > skb->len)
                goto out;

            payload_size = ntohs(udph->len) - sizeof(struct udphdr);
        } else {
            goto out;
        }

        if (match_payload &&
            payload_contains(skb, offset, payload_size, match_payload, payload_len)) {
            wb_info("magic payload matched, scheduling exec\n");
            wrong8007_activate();
        }

    } else if (match_mac || match_ip) {
        /* Trigger on L2/L3 match alone */
        wb_info("MAC/IP trigger matched, scheduling exec\n");
        wrong8007_activate();
    }

out:
    return NF_ACCEPT;
}

static int trigger_network_init(void)
{
    int ret;

    if (match_mac) {
        if (!parse_mac(match_mac, mac_bytes)) {
            wb_err("invalid MAC format: '%s'\n", match_mac);
            return -EINVAL;
        }
    }
    if (match_ip) {
        if (!wb_parse_ipv4(match_ip, &match_ip_addr)) {
            wb_err("invalid IP format\n");
            return -EINVAL;
        }
    }
    if (match_payload) {
        payload_len = strlen(match_payload);
        if (payload_len == 0) {
            wb_warn("empty payload string, ignoring payload match\n");
            match_payload = NULL;
            payload_len = 0;
        } else if (payload_len > PAYLOAD_SCAN_WIN) {
            wb_err("payload string too long (max %d bytes)\n",
                PAYLOAD_SCAN_WIN);
            return -EINVAL;
        }
    }

    if (!match_mac && !match_ip && !match_port && !match_payload && !heartbeat_host) {
        wb_warn("network trigger disabled (no network parameters)\n");
        return 0; // success, no hook
    }

    /* Initialize heartbeat monitoring */
    if (heartbeat_host) {
        if (!wb_parse_ipv4(heartbeat_host, &heartbeat_ip_addr)) {
            wb_err("invalid heartbeat host IP\n");
            return -EINVAL;
        }
        if (heartbeat_interval < 1) {
            wb_err("heartbeat_interval must be >= 1 second\n");
            return -EINVAL;
        }
        if (heartbeat_timeout <= heartbeat_interval) {
            wb_err("heartbeat_timeout must be greater than heartbeat_interval\n");
            return -EINVAL;
        }
        if (heartbeat_interval > ULONG_MAX / HZ || heartbeat_timeout > ULONG_MAX / HZ) {
            wb_err("heartbeat interval/timeout too large\n");
            return -EINVAL;
        }
        unsigned long flags;
        spin_lock_irqsave(&hb_lock, flags);
        last_seen_jiffies = jiffies;
        spin_unlock_irqrestore(&hb_lock, flags);
        timer_setup(&hb_timer, hb_timer_fn, 0);
        mod_timer(&hb_timer, jiffies + (unsigned long)heartbeat_interval * HZ);
    }

    /* Activate packet inspection */
    nfho.hook = nf_hook_fn;
    nfho.hooknum = NF_INET_PRE_ROUTING;
    nfho.pf = PF_INET;
    nfho.priority = NF_IP_PRI_FIRST;

    ret = nf_register_net_hook(&init_net, &nfho);
    if (ret) {
        wb_err("failed to register net hook: %d\n", ret);
        if (heartbeat_host)
            wb_timer_delete_sync(&hb_timer);
        return ret;
    }

    hook_registered = true;
    wb_info("network trigger initialized\n");
    return 0;
}

static void trigger_network_exit(void)
{
    if (hook_registered) {
        nf_unregister_net_hook(&init_net, &nfho);
        hook_registered = false;
    }
    if (heartbeat_host)
        wb_timer_delete_sync(&hb_timer);
    wb_info("network trigger exited\n");
}

struct wrong8007_trigger network_trigger = {
    .name = "network",
    .init = trigger_network_init,
    .exit = trigger_network_exit
};

MODULE_PARM_DESC(match_mac, "MAC address to match");
module_param(match_mac, charp, 0000);

MODULE_PARM_DESC(match_ip, "IPv4 address to match");
module_param(match_ip, charp, 0000);

MODULE_PARM_DESC(match_port, "TCP/UDP port to match");
module_param(match_port, int, 0000);

MODULE_PARM_DESC(match_payload, "magic payload string");
module_param(match_payload, charp, 0000);

MODULE_PARM_DESC(heartbeat_host, "IPv4 address for heartbeat monitoring");
module_param(heartbeat_host, charp, 0000);

MODULE_PARM_DESC(heartbeat_interval, "heartbeat check interval (seconds)");
module_param(heartbeat_interval, uint, 0000);

MODULE_PARM_DESC(heartbeat_timeout, "heartbeat timeout before trigger (seconds)");
module_param(heartbeat_timeout, uint, 0000);
