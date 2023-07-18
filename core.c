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
#include <linux/slab.h>
#include <linux/kmod.h>
#include "wrong8007.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("03C0");
MODULE_DESCRIPTION("wrong8007 is an equivalent of a burner phone");

static char *exec;
module_param(exec, charp, 0000);

// Internal storage of module params
char *exec_buf;

// Deferred work to run userspace helper
struct work_struct exec_work;

// Exported trigger list
extern struct wrong8007_trigger keyboard_trigger;
extern struct wrong8007_trigger usb_trigger;
extern struct wrong8007_trigger network_trigger;

static struct wrong8007_trigger *triggers[] = {
    &keyboard_trigger,
    &usb_trigger,
    &network_trigger,
    NULL
};

// Minimal environment for shell execution
static char *env[] = {
    "HOME=/",
    "TERM=linux",
    "PATH=/sbin:/bin:/usr/sbin:/usr/bin",
    NULL
};

/*
 * Deferred work handler to execute usermode command
 */
static void do_exec_work(struct work_struct *w)
{
    struct subprocess_info *info;
    char **argv;

    argv = kmalloc_array(4, sizeof(char *), GFP_KERNEL);
    if (!argv) {
        pr_err("wrong8007: argv allocation failed\n");
        return;
    }

    argv[0] = "/bin/sh";
    argv[1] = "-c";
    argv[2] = kstrdup(exec_buf, GFP_KERNEL); // deep copy string
    argv[3] = NULL;

    if (!argv[2]) {
        kfree(argv);
        pr_err("wrong8007: command strdup failed\n");
        return;
    }

    info = call_usermodehelper_setup(argv[0], argv, env, GFP_KERNEL, NULL, NULL, NULL);
    if (!info) {
        kfree(argv[2]);
        kfree(argv);
        pr_err("wrong8007: helper setup failed\n");
        return;
    }

    int ret = call_usermodehelper_exec(info, UMH_NO_WAIT);
    pr_info("wrong8007: exec returned %d\n", ret);
}

/*
 * Module init: duplicate input params, register notifier, init work
 */
static int __init wrong8007_init(void)
{
    int i, err;
    if (!exec || !*exec)
        return -EINVAL;

    exec_buf = kstrdup(exec, GFP_KERNEL);
    if (!exec_buf) {
        return -ENOMEM;
    }

    INIT_WORK(&exec_work, do_exec_work);

    for (i = 0; triggers[i]; i++) {
        err = triggers[i]->init();
        if (err) {
            pr_err("wrong8007: failed to init trigger: %s\n", triggers[i]->name);
            goto fail;
        }
    }

    pr_info("wrong8007: loaded\n");
    return 0;

fail:
    while (--i >= 0)
        triggers[i]->exit();

    kfree(exec_buf);
    return err;
}

/*
 * Module exit: cleanup memory, flush workqueue, unregister hook
 */
static void __exit wrong8007_exit(void)
{
    int i;
    for (i = 0; triggers[i]; i++)
        triggers[i]->exit();

    flush_work(&exec_work);
    kfree(exec_buf);
    pr_info("wrong8007: unloaded\n");
}

module_init( wrong8007_init );
module_exit( wrong8007_exit );
