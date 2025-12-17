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
#include <linux/atomic.h>

#include <wrong8007.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("03C0");
MODULE_DESCRIPTION("wrong8007 is an equivalent of a burner phone");

static char *exec;
module_param(exec, charp, 0000);

// Internal storage of module params
static char *exec_buf;

// Deferred work to run userspace helper
static struct work_struct exec_work;

// Execution policy state
static atomic_t exec_armed = ATOMIC_INIT(1);

// Exported trigger list
extern struct wrong8007_trigger keyboard_trigger;
extern struct wrong8007_trigger usb_trigger;
extern struct wrong8007_trigger network_trigger;

/*
 * Trigger interface contract:
 *
 * - Triggers are owned and managed by the core module.
 * - init() is called once at module load, in array order, and returns 0
 *   on success or a negative errno on failure.
 * - On partial init failure, exit() is called in reverse order for
 *   triggers whose init() succeeded.
 * - exit() is called once at module unload and must fully undo init().
 * - Triggers must not call wrong8007_activate() from init() or exit().
 * - Trigger callbacks may call wrong8007_activate() after init().
 *
 * This contract enforces fail-closed behavior and one-shot execution.
 */
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
        wb_err("wrong8007: helper setup failed\n");
        return;
    }

    /* Wait for completion to ensure one-shot semantics */
    ret = call_usermodehelper_exec(info, UMH_WAIT_PROC);
    wb_dbg("exec returned %d\n", ret);
}

/*
 * Authorize execution of the configured action by trigger
 * backends when their activation condition is satisfied.
 *
 * Only the first caller while execution is armed will schedule
 * the deferred work; all subsequent calls are ignored.
 *
 */
void wrong8007_activate(void)
{
    if (atomic_cmpxchg(&exec_armed, 1, 0) == 1) {
        schedule_work(&exec_work);
    }
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

    // Re-arm once at module load
    atomic_set(&exec_armed, 1);

    INIT_WORK(&exec_work, do_exec_work);

    for (i = 0; i < ARRAY_SIZE(triggers); i++) {
        err = triggers[i]->init();
        if (err) {
            wb_err("wrong8007: failed to init trigger: %s\n", triggers[i]->name);
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
