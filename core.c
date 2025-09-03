// SPDX-License-Identifier: GPL-2.0
/*
 * wrong8007
 *
 * Copyright (c) 2023, 03C0 (https://03c0.net/)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */
#include <linux/slab.h>
#include <linux/kmod.h>

#include <wrong8007.h>

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
    &network_trigger
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
    int ret;
    struct subprocess_info *info;
    const char *argv[4] = { "/bin/sh", "-c", exec_buf, NULL };

    info = call_usermodehelper_setup(argv[0], (char **)argv, env, GFP_KERNEL, NULL, NULL, NULL);
    if (!info) {
        pr_err("wrong8007: helper setup failed\n");
        return;
    }

    ret = call_usermodehelper_exec(info, UMH_WAIT_PROC);
    wb_dbg("exec returned %d\n", ret);
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

    for (i = 0; i < ARRAY_SIZE(triggers); i++) {
        err = triggers[i]->init();
        if (err) {
            pr_err("wrong8007: failed to init trigger: %s\n", triggers[i]->name);
            goto fail;
        }
    }

    wb_info("loaded\n");
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
    for (i = 0; i < ARRAY_SIZE(triggers); i++)
        triggers[i]->exit();

    flush_work(&exec_work);
    kfree(exec_buf);
    wb_info("unloaded\n");
}

module_init( wrong8007_init );
module_exit( wrong8007_exit );
