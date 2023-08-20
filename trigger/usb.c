/*
 * Copyright (c) 2023, Piyush Raj (https://piyushraj.org/)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Gibson nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <linux/kernel.h>
#include <linux/module.h>
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

// Storage for parsed rules
static struct usb_dev_rule usb_rules[MAX_USB_DEVICES];
static int usb_rule_count;

// Whitelist/blacklist mode (default = whitelist)
static bool usb_whitelist = true;
module_param_named(whitelist, usb_whitelist, bool, 0000);
MODULE_PARM_DESC(whitelist, "true=only match listed devices, false=match all except listed");

// Device rules as strings: "VID:PID:EVENT"
static char *usb_devices[MAX_USB_DEVICES];
static int usb_devices_count;
module_param_array(usb_devices, charp, &usb_devices_count, 0000);
MODULE_PARM_DESC(usb_devices, "VID:PID:EVENT (EVENT=insert|eject|any)");

// External work from main module
extern struct work_struct exec_work;

// Notifier forward declaration
static struct notifier_block usb_nb;

// Parse module param usb_devices[] into structured rules
static int parse_usb_devices(void)
{
    int i;
    for (i = 0; i < usb_devices_count && i < MAX_USB_DEVICES; i++) {
        unsigned int vid, pid;
        char evt_str[16] = "any";
        int n = sscanf(usb_devices[i], "%i:%i:%15s", &vid, &pid, evt_str);
        if (n < 2) {
            pr_err("wrong8007: Invalid USB rule '%s'\n", usb_devices[i]);
            return -EINVAL;
        }

        usb_rules[usb_rule_count].vid = (u16)vid;
        usb_rules[usb_rule_count].pid = (u16)pid;

        if (!strcmp(evt_str, "insert"))
            usb_rules[usb_rule_count].event = USB_EVT_INSERT;
        else if (!strcmp(evt_str, "eject"))
            usb_rules[usb_rule_count].event = USB_EVT_EJECT;
        else if (!strcmp(evt_str, "any"))
            usb_rules[usb_rule_count].event = USB_EVT_ANY;
        else {
            pr_err("wrong8007: Unknown event '%s' in rule '%s'\n",
                evt_str, usb_devices[i]);
            return -EINVAL;
        }

        pr_info("wrong8007: rule[%d] VID=0x%04x PID=0x%04x EVENT=%s\n",
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

    if ((usb_whitelist && matched) || (!usb_whitelist && !matched)) {
        pr_info("wrong8007: USB trigger fired (VID=0x%04x PID=0x%04x)\n", vid, pid);
        schedule_work(&exec_work);
    }

    return NOTIFY_OK;
}

static struct notifier_block usb_nb = {
    .notifier_call = usb_notifier_callback,
};

// Init/exit
static int trigger_usb_init(void)
{
    int ret = parse_usb_devices();
    if (ret)
        return ret;

    if (usb_rule_count == 0) {
        pr_info("wrong8007: No USB rules configured, trigger disabled\n");
        return 0;
    }

    usb_register_notify(&usb_nb);
    pr_info("wrong8007: USB trigger initialized in %s mode (%d rules)\n",
            usb_whitelist ? "whitelist" : "blacklist", usb_rule_count);
    return 0;
}

static void trigger_usb_exit(void)
{
    if (usb_rule_count > 0)
        usb_unregister_notify(&usb_nb);
    pr_info("wrong8007: USB trigger exited\n");
}

// Expose as trigger plugin
struct wrong8007_trigger usb_trigger = {
    .name = "usb",
    .init = trigger_usb_init,
    .exit = trigger_usb_exit
};
