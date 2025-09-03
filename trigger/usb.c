// SPDX-License-Identifier: GPL-2.0
/*
 * wrong8007: usb trigger
 *
 * Copyright (c) 2023, 03C0 (https://03c0.net/)
 */
#include <linux/usb.h>
#include <linux/string.h>

#include <wrong8007.h>

#define MAX_USB_DEVICES 16

// Device rule structure
struct usb_dev_rule {
    u16 vid;
    u16 pid;
    enum { USB_EVT_INSERT, USB_EVT_EJECT, USB_EVT_ANY } event;
};

// Table of valid USB events and their enum values
static const struct {
    const char *name;
    int value;
} evt_map[] = {
    { "insert", USB_EVT_INSERT },
    { "eject",  USB_EVT_EJECT  },
    { "any",    USB_EVT_ANY    },
};

// Storage for parsed rules
static struct usb_dev_rule usb_rules[MAX_USB_DEVICES];
static int usb_rule_count;

// Whitelist/blacklist mode (default = blacklist)
static bool usb_whitelist = false;
module_param_named(whitelist, usb_whitelist, bool, 0000);
MODULE_PARM_DESC(whitelist, "true=match all except listed, false=only match listed devices");

// Device rules as strings: "VID:PID:EVENT"
static char *usb_devices[MAX_USB_DEVICES];
static int usb_devices_count;
module_param_array(usb_devices, charp, &usb_devices_count, 0000);
MODULE_PARM_DESC(usb_devices, "VID:PID:EVENT (EVENT=insert|eject|any)");

// Parse module param usb_devices[] into structured rules
static int parse_usb_devices(void)
{
    int i, j;
    int limit = min(usb_devices_count, MAX_USB_DEVICES);

    for (i = 0; i < limit; i++) {
        unsigned int vid, pid;
        char buf[64];         // local bounded copy of param string
        char evt_str[16] = "any"; // default: "any", can be "eject" or "insert"
        int n;

        // Ensure bounded copy of the module param string
        strscpy(buf, usb_devices[i], sizeof(buf));

        n = sscanf(buf, "%x:%x:%15s", &vid, &pid, evt_str);
        if (n < 2) {
            wb_err("invalid USB rule '%s'\n", buf);
            return -EINVAL;
        }

        usb_rules[usb_rule_count].vid = (u16)vid;
        usb_rules[usb_rule_count].pid = (u16)pid;

        // Map event string to enum
        for (j = 0; j < ARRAY_SIZE(evt_map); j++) {
            if (!strcmp(evt_str, evt_map[j].name)) {
                usb_rules[usb_rule_count].event = evt_map[j].value;
                goto valid_event;
            }
        }

        wb_err("unknown event '%s' in rule '%s'\n", evt_str, buf);
        return -EINVAL;

valid_event:
        wb_dbg("rule[%d] VID=0x%04x PID=0x%04x EVENT=%s\n",
                usb_rule_count, vid, pid, evt_str);
        usb_rule_count++;
    }

    return 0;
}

// Match a device/action against rules
static bool match_rules(u16 vid, u16 pid, unsigned long action)
{
    int i;
    for (i = 0; i < usb_rule_count; i++) {
        if (vid == usb_rules[i].vid && pid == usb_rules[i].pid) {
            if ((action == USB_DEVICE_ADD &&
                 (usb_rules[i].event == USB_EVT_INSERT || usb_rules[i].event == USB_EVT_ANY)) ||
                (action == USB_DEVICE_REMOVE &&
                 (usb_rules[i].event == USB_EVT_EJECT || usb_rules[i].event == USB_EVT_ANY)))
                return true;
        }
    }
    return false;
}

// USB notifier callback
static int usb_notifier_callback(struct notifier_block *self, unsigned long action, void *dev)
{
    struct usb_device *udev = dev;
    u16 vid = le16_to_cpu(udev->descriptor.idVendor);
    u16 pid = le16_to_cpu(udev->descriptor.idProduct);

    bool matched = match_rules(vid, pid, action);

    if ((usb_whitelist && !matched) || (!usb_whitelist && matched)) {
        wb_info("USB trigger fired (VID=0x%04x PID=0x%04x)\n", vid, pid);
        schedule_work(&exec_work);
    }

    return NOTIFY_OK;
}

// Declare notifier_block
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

    usb_register_notify(&usb_nb);
    wb_info("USB trigger initialized in %s mode (%d rules)\n",
            usb_whitelist ? "whitelist" : "blacklist", usb_rule_count);
    return 0; // in recent kernels, usb_register_notify returns void
}

static void trigger_usb_exit(void)
{
    if (usb_rule_count > 0)
        usb_unregister_notify(&usb_nb);
    wb_info("USB trigger exited\n");
}

// Expose as trigger plugin
struct wrong8007_trigger usb_trigger = {
    .name = "usb",
    .init = trigger_usb_init,
    .exit = trigger_usb_exit
};
