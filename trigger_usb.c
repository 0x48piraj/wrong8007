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
#include "wrong8007.h"

#define USB_VENDOR_ID  0x1234
#define USB_PRODUCT_ID 0x5678

// Declare the external exec_work from main module
extern struct work_struct exec_work;

// USB notifier callback
static int usb_notifier_callback(struct notifier_block *self, unsigned long action, void *dev)
{
    struct usb_device *udev = dev;

    if (action == USB_DEVICE_ADD &&
        udev->descriptor.idVendor == cpu_to_le16(USB_VENDOR_ID) &&
        udev->descriptor.idProduct == cpu_to_le16(USB_PRODUCT_ID)) {

        pr_info("wrong8007: USB trigger matched. Scheduling exec.\n");
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
    usb_register_notify(&usb_nb);
    pr_info("wrong8007: USB trigger initialized\n");
    return 0; // in recent kernels, usb_register_notify returns void
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
