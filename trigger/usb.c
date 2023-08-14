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

/* Arrays for multi-device configuration */
static int usb_vids[MAX_USB_DEVICES];
static int usb_pids[MAX_USB_DEVICES];
static char *usb_events[MAX_USB_DEVICES];

/* Actual number of configured devices */
static int usb_count_vid;
static int usb_count_pid;
static int usb_count_evt;

/* Module params */
module_param_array(usb_vids, int, &usb_count_vid, 0000);
MODULE_PARM_DESC(usb_vids, "Array of USB Vendor IDs");

module_param_array(usb_pids, int, &usb_count_pid, 0000);
MODULE_PARM_DESC(usb_pids, "Array of USB Product IDs");

module_param_array(usb_events, charp, &usb_count_evt, 0000);
MODULE_PARM_DESC(usb_events, "Array of USB events: insert, eject, or any");

// Declare the external exec_work from main module
extern struct work_struct exec_work;

/* Helper: whether the action matches the configured string */
static bool match_event_type(const char *event, unsigned long action)
{
    if (!event)
        return false;
    return (action == USB_DEVICE_ADD &&
            (!strcmp(event, "insert") || !strcmp(event, "any"))) ||
           (action == USB_DEVICE_REMOVE &&
            (!strcmp(event, "eject") || !strcmp(event, "any")));
}

static int usb_notifier_callback(struct notifier_block *self,
                                 unsigned long action, void *dev)
{
    struct usb_device *udev = dev;
    u16 vid = le16_to_cpu(udev->descriptor.idVendor);
    u16 pid = le16_to_cpu(udev->descriptor.idProduct);
    int i;

    for (i = 0; i < min3(usb_count_vid, usb_count_pid, usb_count_evt); i++) {
        if (vid == (u16)usb_vids[i] &&
            pid == (u16)usb_pids[i] &&
            match_event_type(usb_events[i], action)) {

            pr_info("wrong8007: USB trigger[%d] matched "
                    "(VID=0x%04x PID=0x%04x EVENT=%s). Scheduling exec.\n",
                    i, vid, pid, usb_events[i]);
            schedule_work(&exec_work);
            break;
        }
    }

    return NOTIFY_OK;
}

// Declare notifier_block
static struct notifier_block usb_nb = {
    .notifier_call = usb_notifier_callback,
};

static int trigger_usb_init(void)
{
    int i;

    if (usb_count_vid != usb_count_pid || usb_count_vid != usb_count_evt) {
        pr_err("wrong8007: usb_vids, usb_pids, and usb_events must have same length\n");
        return -EINVAL;
    }
    if (usb_count_vid <= 0) {
        pr_err("wrong8007: No USB devices configured\n");
        return -EINVAL;
    }

    for (i = 0; i < usb_count_vid; i++) {
        if (!usb_events[i]) {
            pr_err("wrong8007: usb_events[%d] is NULL\n", i);
            return -EINVAL;
        }
        pr_info("wrong8007: Configured USB trigger[%d] "
                "(VID=0x%04x PID=0x%04x EVENT=%s)\n",
                i, usb_vids[i], usb_pids[i], usb_events[i]);
    }

    usb_register_notify(&usb_nb);
    pr_info("wrong8007: USB multi-device trigger initialized\n");
    return 0;
}

static void trigger_usb_exit(void)
{
    usb_unregister_notify(&usb_nb);
    pr_info("wrong8007: USB trigger exited\n");
}

// Expose as a trigger plugin
struct wrong8007_trigger usb_trigger = {
    .name = "usb",
    .init = trigger_usb_init,
    .exit = trigger_usb_exit
};
