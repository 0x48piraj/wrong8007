#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/etherdevice.h>

#include "wrong8007.h"

static char *match_mac = NULL;
module_param(match_mac, charp, 0000);
MODULE_PARM_DESC(match_mac, "MAC address to match");

extern struct work_struct exec_work;

static u8 mac_bytes[ETH_ALEN];
static struct nf_hook_ops nfho;

static bool parse_mac(const char *mac_str, u8 *out)
{
    return mac_str && mac_pton(mac_str, out) == 0;
}

static unsigned int nf_hook_fn(void *priv,
                                struct sk_buff *skb,
                                const struct nf_hook_state *state)
{
    struct ethhdr *eth;

    eth = eth_hdr(skb);
    if (match_mac && !ether_addr_equal(mac_bytes, eth->h_source))
        return NF_ACCEPT;

    pr_info("wrong8007: MAC trigger matched. Scheduling exec.\n");
    schedule_work(&exec_work);

    return NF_ACCEPT;
}

static int trigger_network_init(void)
{
    if (match_mac) {
        if (!parse_mac(match_mac, mac_bytes)) {
            pr_err("wrong8007: Invalid MAC address\n");
            return -EINVAL;
        }
    }

    nfho.hook = nf_hook_fn;
    nfho.hooknum = NF_INET_PRE_ROUTING;
    nfho.pf = PF_INET;
    nfho.priority = NF_IP_PRI_FIRST;

    if (nf_register_net_hook(&init_net, &nfho)) {
        pr_err("wrong8007: Failed to register netfilter hook\n");
        return -1;
    }

    pr_info("wrong8007: Netfilter hook with MAC matching initialized\n");
    return 0;
}

static void trigger_network_exit(void)
{
    nf_unregister_net_hook(&init_net, &nfho);
    pr_info("wrong8007: Network trigger exited\n");
}

struct wrong8007_trigger network_trigger = {
    .name = "network",
    .init = trigger_network_init,
    .exit = trigger_network_exit
};