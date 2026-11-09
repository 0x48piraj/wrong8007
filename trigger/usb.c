// SPDX-License-Identifier: GPL-2.0
/*
 * wrong8007: usb trigger
 *
 * Copyright (c) 2023, 03C0 (https://03c0.net/)
 */

#include <linux/usb.h>
#include <linux/string.h>
#include <linux/slab.h>

#include <wrong8007.h>
#include <parse.h>

#define MAX_USB_DEVICES 16
#define USB_RULE_MAX_LEN 64

static struct wb_usb_rule usb_rules[MAX_USB_DEVICES];
static int usb_rule_count;

static bool usb_whitelist;
module_param_named(whitelist, usb_whitelist, bool, 0000);
MODULE_PARM_DESC(whitelist, "1 fires on unlisted devices and 0 on listed devices");

static char *usb_devices;
module_param(usb_devices, charp, 0000);
MODULE_PARM_DESC(usb_devices, "comma-separated USB rules VID:PID[:insert|eject|any]");

static int parse_usb_devices(void)
{
    char *config;
    char *cursor;
    char *token;
    int ret = 0;

    usb_rule_count = 0;

    if (!usb_devices || !*usb_devices)
        return 0;

    config = kstrdup(usb_devices, GFP_KERNEL);
    if (!config)
        return -ENOMEM;

    cursor = config;

    while ((token = strsep(&cursor, ",")) != NULL) {
        char *rule = strim(token);

        if (!*rule)
            continue;

        if (usb_rule_count >= MAX_USB_DEVICES) {
            wb_err("too many USB rules (max %d)\n",
                   MAX_USB_DEVICES);
            ret = -E2BIG;
            goto out;
        }

        if (strlen(rule) >= USB_RULE_MAX_LEN) {
            wb_err("USB rule too long: '%s'\n", rule);
            ret = -EINVAL;
            goto out;
        }

        ret = wb_parse_usb_rule(rule, &usb_rules[usb_rule_count]);
        if (ret) {
            wb_err("invalid USB rule '%s' (expected VID:PID[:insert|eject|any])\n",
                   rule);
            goto out;
        }

        usb_rule_count++;
    }

out:
    kfree(config);
    return ret;
}

static bool match_rules(u16 vid, u16 pid, unsigned long action)
{
    int i;

    for (i = 0; i < usb_rule_count; i++) {
        const struct wb_usb_rule *rule = &usb_rules[i];

        if (vid != rule->vid || pid != rule->pid)
            continue;

        if (action == USB_DEVICE_ADD &&
            (rule->event == WB_USB_INSERT ||
             rule->event == WB_USB_ANY))
            return true;

        if (action == USB_DEVICE_REMOVE &&
            (rule->event == WB_USB_EJECT ||
             rule->event == WB_USB_ANY))
            return true;
    }

    return false;
}

// USB notifier callback
static int usb_notifier_callback(struct notifier_block *self, unsigned long action, void *dev)
{
    struct usb_device *udev;
    u16 vid, pid;

    bool matched;

    if (action != USB_DEVICE_ADD && action != USB_DEVICE_REMOVE)
        return NOTIFY_OK;

    udev = dev;
    vid = le16_to_cpu(udev->descriptor.idVendor);
    pid = le16_to_cpu(udev->descriptor.idProduct);

    matched = match_rules(vid, pid, action);

    if ((usb_whitelist && !matched) || (!usb_whitelist && matched)) {
        wb_info("USB trigger fired (VID=0x%04x PID=0x%04x)\n", vid, pid);
        wrong8007_activate();
    }

    return NOTIFY_OK;
}

static struct notifier_block usb_nb = {
    .notifier_call = usb_notifier_callback,
};

static int trigger_usb_init(void)
{
    int ret = parse_usb_devices();
    if (ret)
        return ret;

    if (usb_rule_count == 0) {
        wb_warn("USB trigger disabled (no USB rules)\n");
        return 0; // success, but no hook
    }

    usb_register_notify(&usb_nb); // no return value on modern kernels

    wb_info("USB trigger initialized in %s mode (%d rules)\n",
            usb_whitelist ? "whitelist" : "blacklist", usb_rule_count);

    return 0;
}

static void trigger_usb_exit(void)
{
    if (usb_rule_count > 0)
        usb_unregister_notify(&usb_nb);

    usb_rule_count = 0;

    wb_info("USB trigger exited\n");
}

struct wrong8007_trigger usb_trigger = {
    .name = "usb",
    .init = trigger_usb_init,
    .exit = trigger_usb_exit
};
