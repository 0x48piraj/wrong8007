#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/ip.h>

#include "wrong8007.h"

static struct nf_hook_ops nfho;

static unsigned int nf_hook_fn(void *priv,
                                struct sk_buff *skb,
                                const struct nf_hook_state *state)
{
    return NF_ACCEPT;
}

static int trigger_network_init(void)
{
    nfho.hook = nf_hook_fn;
    nfho.hooknum = NF_INET_PRE_ROUTING;
    nfho.pf = PF_INET;
    nfho.priority = NF_IP_PRI_FIRST;

    if (nf_register_net_hook(&init_net, &nfho)) {
        pr_err("wrong8007: Failed to register netfilter hook\n");
        return -1;
    }

    pr_info("wrong8007: Netfilter hook registered\n");
    return 0;
}

static void trigger_network_exit(void)
{
    nf_unregister_net_hook(&init_net, &nfho);
    pr_info("wrong8007: Netfilter hook unregistered\n");
}

struct wrong8007_trigger network_trigger = {
    .name = "network",
    .init = trigger_network_init,
    .exit = trigger_network_exit
};