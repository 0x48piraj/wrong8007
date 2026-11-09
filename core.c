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
#include <linux/workqueue.h>
#include <linux/jiffies.h>

#include <wrong8007.h>

#define REGISTER_TRIGGER(t) &t
#define EXEC_MAX_LEN 4096

/* Retry failed starts with capped exponential backoff */
#define MAX_STRIKES 8
#define COOLDOWN_MIN (1 * HZ)
#define COOLDOWN_MAX (30 * HZ)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("03C0");
MODULE_DESCRIPTION("wrong8007 is an equivalent of a burner phone");

static char *exec;
module_param(exec, charp, 0000);
MODULE_PARM_DESC(exec, "absolute path of the executable run on activation");

static char *exec_buf;

/* Workqueue for payload startup attempts and retries */
static struct workqueue_struct *exec_wq;
static struct delayed_work exec_work;
static unsigned int strikes;
static bool scrubbed;

/* One-shot activation latch */
static atomic_t exec_armed = ATOMIC_INIT(0);

/* Trigger backends */
extern struct wrong8007_trigger keyboard_trigger;
extern struct wrong8007_trigger usb_trigger;
extern struct wrong8007_trigger network_trigger;

/*
 * Trigger interface contract:
 *
 * - The core initializes triggers in array order.
 * - init() returns 0 on success or a negative errno on failure.
 * - On init failure, successfully initialized triggers are released
 *   in reverse order.
 * - exit() is called once at module unload and must fully undo init().
 * - Triggers must not call wrong8007_activate() from init() or exit().
 * - Trigger callbacks may call wrong8007_activate() after init().
 */
static struct wrong8007_trigger *triggers[] = {
    REGISTER_TRIGGER(keyboard_trigger),
    REGISTER_TRIGGER(usb_trigger),
    REGISTER_TRIGGER(network_trigger),
};

/* Environment passed to call_usermodehelper() */
static char *env[] = {
    "HOME=/",
    "PATH=/sbin:/bin:/usr/sbin:/usr/bin",
    NULL
};

/*
 * Start the configured executable and retry failed starts.
 */
static void do_exec_work(struct work_struct *w)
{
    char *argv[] = { exec_buf, NULL };
    unsigned long delay;
    int ret;

    strikes++;
    ret = call_usermodehelper(exec_buf, argv, env, UMH_WAIT_EXEC);

    if (ret == 0) {
        wb_info("payload started\n");
        return;
    }

    if (READ_ONCE(scrubbed) || strikes >= MAX_STRIKES) {
        wb_err("payload failed to start (err=%d), giving up after %u attempt(s)\n",
               ret, strikes);
        return;
    }

    delay = min_t(unsigned long, COOLDOWN_MIN << (strikes - 1), COOLDOWN_MAX);
    wb_err("payload failed to start (err=%d), retrying in %lus (attempt %u/%u)\n",
           ret, delay / HZ, strikes, MAX_STRIKES);
    queue_delayed_work(exec_wq, &exec_work, delay);
}

/*
 * Authorize execution of the configured action by trigger
 * backends when their activation condition is satisfied.
 *
 * Only the first caller while execution is armed will schedule
 * the deferred work; all subsequent calls are ignored.
 * Safe to call from atomic context.
 */
void wrong8007_activate(void)
{
    if (atomic_cmpxchg(&exec_armed, 1, 0) == 1)
        queue_delayed_work(exec_wq, &exec_work, 0);
}

/*
 * Stop scheduling attempts and wait for any in-flight startup attempt.
 */
static void scrub(void)
{
    WRITE_ONCE(scrubbed, true);
    if (cancel_delayed_work_sync(&exec_work))
        wb_err("pending payload attempt dropped\n");
    destroy_workqueue(exec_wq);
    exec_wq = NULL;
}

/*
 * Module init.
 */
static int __init wrong8007_init(void)
{
    int i, err;

    if (!exec || !*exec) {
        wb_err("exec parameter required\n");
        return -EINVAL;
    }

    if (exec[0] != '/') {
        wb_err("exec must be an absolute path\n");
        return -EINVAL;
    }

    if (strnlen(exec, EXEC_MAX_LEN + 1) > EXEC_MAX_LEN) {
        wb_err("exec parameter too long (max: %d)\n", EXEC_MAX_LEN);
        return -EINVAL;
    }

    exec_buf = kstrdup(exec, GFP_KERNEL);
    if (!exec_buf)
        return -ENOMEM;

    exec_wq = alloc_workqueue("wrong8007", WQ_UNBOUND, 1);
    if (!exec_wq) {
        err = -ENOMEM;
        goto fail_wq;
    }

    strikes = 0;
    scrubbed = false;

    INIT_DELAYED_WORK(&exec_work, do_exec_work);

    for (i = 0; i < ARRAY_SIZE(triggers); i++) {
        err = triggers[i]->init();
        if (err) {
            wb_err("failed to init trigger: %s\n", triggers[i]->name);
            goto fail;
        }
    }

    /* Arm only once every trigger is up */
    atomic_set(&exec_armed, 1);

    wb_info("loaded\n");
    return 0;

fail:
    while (--i >= 0)
        triggers[i]->exit();

    scrub();
fail_wq:
    kfree(exec_buf);
    exec_buf = NULL;

    return err;
}

/*
 * Module exit.
 *
 * Triggers go first and nothing activates afterwards.
 */
static void __exit wrong8007_exit(void)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(triggers); i++)
        triggers[i]->exit();

    scrub();

    kfree(exec_buf);
    exec_buf = NULL;

    wb_info("unloaded\n");
}

module_init( wrong8007_init );
module_exit( wrong8007_exit );
