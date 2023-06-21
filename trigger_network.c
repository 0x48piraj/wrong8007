#include <linux/module.h>
#include <linux/kernel.h>

#include "wrong8007.h"

static int trigger_network_init(void)
{
    pr_info("wrong8007: Network trigger init\n");
    return 0;
}

static void trigger_network_exit(void)
{
    pr_info("wrong8007: Network trigger exit\n");
}

struct wrong8007_trigger network_trigger = {
    .name = "network",
    .init = trigger_network_init,
    .exit = trigger_network_exit
};
