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

/* Params: pick what you need */
static char *match_mac = NULL;       // MAC address in "aa:bb:cc:dd:ee:ff" or NULL
static char *match_ip = NULL;        // IPv4 in dotted-decimal or NULL
static int match_port = 0;           // TCP/UDP port for magic packet
static char *match_payload = NULL;   // Magic payload string to match

static char *heartbeat_host = NULL;  // Host to ping periodically (string IP)
static unsigned int heartbeat_interval = 10; // seconds
static unsigned int heartbeat_timeout   = 30; // seconds

/* Parsed forms */
static u8 mac_bytes[ETH_ALEN];
static __be32 ip_addr = 0;
static size_t payload_len = 0;

/* Netfilter hook */
static struct nf_hook_ops nfho;

/* Heartbeat tracking */
static struct timer_list hb_timer;
static unsigned long last_seen_jiffies;

/* Parse MAC address in formats:
 *   aa:bb:cc:dd:ee:ff
 *   aa-bb-cc-dd-ee-ff
 *   aabbccddeeff
 * Accepts upper or lower case hex. Returns true on success and fills out[].
 */
static bool parse_mac(const char *s, u8 *out)
{
    int i = 0;
    int hi = -1; /* high nibble accumulator, -1 means waiting for high nibble */

    if (!s || !out)
        return false;

    while (*s && i < ETH_ALEN) {
        char c = *s++;

        /* convert hex nibble; skip separators */
        int val;
        if (c >= '0' && c <= '9')
            val = c - '0';
        else if (c >= 'a' && c <= 'f')
            val = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            val = c - 'A' + 10;
        else
            continue; /* skip ':' '-' or any other separator */

        if (hi == -1) {
            hi = val;
        } else {
            out[i++] = (u8)((hi << 4) | val);
            hi = -1;
        }
    }

    /* must have exactly 6 bytes and no stray half-nibble */
    if (i == ETH_ALEN && hi == -1)
        return true;

    return false;
}

/* Convert IPv4 string to __be32 */
static bool parse_ip(const char *ip_str, __be32 *out)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
    return ip_str && in4_pton(ip_str, -1, (u8 *)out, '\0', NULL);
#else
    if (!ip_str)
        return false;
    *out = in_aton(ip_str);
    return *out != 0;
#endif
}

/* Heartbeat timer handler */
static void hb_timer_fn(struct timer_list *t)
{
    unsigned long now = jiffies;
    if (time_after(now, last_seen_jiffies + heartbeat_timeout * HZ)) {
        wb_info("heartbeat timeout reached, scheduling exec\n");
        schedule_work(&exec_work);
    } else {
        mod_timer(&hb_timer, jiffies + heartbeat_interval * HZ);
    }
}

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

/* Netfilter hook function */
static unsigned int nf_hook_fn(void *priv,
                                struct sk_buff *skb,
                                const struct nf_hook_state *state)
{
    struct ethhdr *eth;
    struct iphdr *iph;
    struct tcphdr *tcph;
    struct udphdr *udph;
    u8 *payload;
    unsigned int payload_size;

    /* L3 / IPv4 only */
    if (skb->protocol != htons(ETH_P_IP))
        goto out;

    /* Ensure IP header */
    if (!pskb_may_pull(skb, sizeof(struct iphdr)))
        goto out;

    iph = ip_hdr(skb);

    /* L2 / MAC match */
    if (match_mac) {
        if (!skb_mac_header_was_set(skb))
            goto out;

        eth = eth_hdr(skb);
        if (!ether_addr_equal(mac_bytes, eth->h_source))
            goto out;
    }

    /* L3 / IP match */
    if (match_ip && iph->saddr != match_ip_addr)
        goto out;

    /* Heartbeat tracking */
    if (heartbeat_host && iph->saddr == heartbeat_ip_addr)
        last_seen_jiffies = jiffies;

    /* L4 / Payload matching */
    if (match_port || match_payload) {

        if (!pskb_may_pull(skb, skb->len))
            goto out;

        if (iph->protocol == IPPROTO_TCP) {
            tcph = (struct tcphdr *)((u8 *)iph + iph->ihl * 4);

            if (tcph->doff * 4 < sizeof(struct tcphdr))
                goto out;

            if (match_port &&
                ntohs(tcph->source) != match_port &&
                ntohs(tcph->dest) != match_port)
                goto out;

            payload = (u8 *)tcph + tcph->doff * 4;
            payload_size = skb->len - (payload - skb->data);

        } else if (iph->protocol == IPPROTO_UDP) {
            udph = (struct udphdr *)((u8 *)iph + iph->ihl * 4);

            payload_size = ntohs(udph->len);
            if (payload_size < sizeof(struct udphdr))
                goto out;

            if (match_port &&
                ntohs(udph->source) != match_port &&
                ntohs(udph->dest) != match_port)
                goto out;

            payload = (u8 *)udph + sizeof(struct udphdr);
            payload_size -= sizeof(struct udphdr);

        } else {
            goto out;
        }

        if (match_payload && payload_size >= payload_len &&
            k_memmem(payload, payload_size, match_payload, payload_len)) {
            wb_info("magic payload matched, scheduling exec\n");
            schedule_work(&exec_work);
        }

    } else if (match_mac || match_ip) {
        /* Pure MAC/IP match triggers */
        wb_info("MAC/IP trigger matched, scheduling exec\n");
        schedule_work(&exec_work);
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
        if (!parse_ip(match_ip, &ip_addr)) {
            wb_err("invalid IP format\n");
            return -EINVAL;
        }
    }
    if (match_payload)
        payload_len = strlen(match_payload);

    /* Heartbeat setup */
    if (heartbeat_host) {
        if (!parse_ip(heartbeat_host, &ip_addr)) {
            wb_err("invalid heartbeat host IP\n");
            return -EINVAL;
        }
        last_seen_jiffies = jiffies;
        timer_setup(&hb_timer, hb_timer_fn, 0);
        mod_timer(&hb_timer, jiffies + heartbeat_interval * HZ);
    }

    /* Install Netfilter hook */
    nfho.hook = nf_hook_fn;
    nfho.hooknum = NF_INET_PRE_ROUTING;
    nfho.pf = PF_INET;
    nfho.priority = NF_IP_PRI_FIRST;

    ret = nf_register_net_hook(&init_net, &nfho);
    if (ret) {
        wb_err("failed to register net hook: %d\n", ret);
        if (heartbeat_host)
            del_timer_sync(&hb_timer);
        return ret;
    }

    wb_info("network trigger initialized\n");
    return 0;
}

static void trigger_network_exit(void)
{
    nf_unregister_net_hook(&init_net, &nfho);
    if (heartbeat_host)
        del_timer_sync(&hb_timer);
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
