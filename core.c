// SPDX-License-Identifier: GPL-2.0
/*
 * wrong8007
 *
 * Copyright (c) 2023, Piyush Raj (https://piyushraj.org/)
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kmod.h>
#include <linux/workqueue.h>
#include <linux/init.h>

#include <wrong8007.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("03C0");
MODULE_DESCRIPTION("wrong8007 is an equivalent of a burner phone");

static char *exec = NULL;
module_param(exec, charp, 0000);
MODULE_PARM_DESC(exec, "Fallback command to run when no per-trigger command is given");

/* private workqueue for all exec work */
static struct workqueue_struct *wrong8007_wq;

// Exported trigger list
extern struct wrong8007_trigger keyboard_trigger;
extern struct wrong8007_trigger usb_trigger;
extern struct wrong8007_trigger network_trigger;

static struct wrong8007_trigger *triggers[] = {
    &keyboard_trigger,
    &usb_trigger,
    &network_trigger
};

/* Minimal environment for shell execution (NULL-terminated) */
static char *exec_env[] = {
    "HOME=/",
    "TERM=linux",
    "PATH=/sbin:/bin:/usr/sbin:/usr/bin",
    NULL
};

/* Per-scheduled-exec context allocated at schedule time */
struct exec_context {
    struct work_struct work;
    const char *cmd;
};

/* Worker that runs the provided command in userspace */
static void do_exec_work(struct work_struct *w)
{
    struct exec_context *ctx = container_of(w, struct exec_context, work);
    struct subprocess_info *info;
    char *argv[4];

    if (!ctx || !ctx->cmd) {
        pr_err("wrong8007: do_exec_work: no ctx or cmd\n");
        kfree(ctx);
        return;
    }

    argv[0] = "/bin/sh";
    argv[1] = "-c";
    argv[2] = (char *)ctx->cmd; /* call_usermodehelper expects char * */
    argv[3] = NULL;

    info = call_usermodehelper_setup(argv[0], argv, exec_env,
                                     GFP_KERNEL, NULL, NULL, NULL);
    if (!info) {
        pr_err("wrong8007: helper setup failed for cmd: %s\n", ctx->cmd);
        kfree(ctx);
        return;
    }

    /* Synchronous until exec completes copying argv/env */
    int ret = call_usermodehelper_exec(info, UMH_WAIT_PROC);
    pr_info("wrong8007: exec returned %d\n", ret);

    /* free our context */
    kfree(ctx);
}

/*
 * Schedule a userspace command. Exported so triggers can call their own command.
 */
void wrong8007_schedule_exec(const char *cmd)
{
    struct exec_context *ctx;

    /* Fallback to global exec param */
    if (!cmd || !*cmd) {
        if (!exec || !*exec)
            return; /* nothing to run */
        cmd = exec;
    }

    ctx = kmalloc(sizeof(*ctx), GFP_KERNEL);
    if (!ctx)
        return;

    ctx->cmd = cmd;
    INIT_WORK(&ctx->work, do_exec_work);
    queue_work(wrong8007_wq, &ctx->work);
}
EXPORT_SYMBOL_GPL(wrong8007_schedule_exec);

/*
 * Module init: init triggers and workqueue
 */
static int __init wrong8007_init(void)
{
    int i, err;

    /* allocate private workqueue first */
    wrong8007_wq = alloc_workqueue("wrong8007_wq",
                                   WQ_UNBOUND | WQ_HIGHPRI, 0);
    if (!wrong8007_wq)
        return -ENOMEM;

    for (i = 0; i < ARRAY_SIZE(triggers); i++) {
        if (!triggers[i] || !triggers[i]->init) {
            pr_err("wrong8007: missing trigger or init for index %d\n", i);
            err = -EINVAL;
            goto fail;
        }
        err = triggers[i]->init();
        if (err) {
            pr_err("wrong8007: failed to init trigger: %s (%d)\n",
                   triggers[i]->name, err);
            goto fail;
        }
    }

    pr_info("wrong8007: loaded\n");
    return 0;

fail:
    while (--i >= 0)
        triggers[i]->exit();
    destroy_workqueue(wrong8007_wq);
    return err;
}

/*
 * Module exit: tell triggers to exit, then wait for outstanding scheduled work
 */
static void __exit wrong8007_exit(void)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(triggers); i++) {
        if (triggers[i] && triggers[i]->exit)
            triggers[i]->exit();
    }

    /* wait for any scheduled do_exec_work() invocations to complete */
    flush_workqueue(wrong8007_wq);
    destroy_workqueue(wrong8007_wq);

    pr_info("wrong8007: unloaded\n");
}

module_init( wrong8007_init );
module_exit( wrong8007_exit );
