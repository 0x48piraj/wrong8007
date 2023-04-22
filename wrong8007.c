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
#include <linux/moduleparam.h>
#include <linux/keyboard.h>
#include <linux/notifier.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/kmod.h>
#include <linux/workqueue.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("03C0");
MODULE_DESCRIPTION("wrong8007 is an equivalent of a burner phone");

static char *phrase;
static char *exec;
module_param(phrase, charp, 0000);
module_param(exec, charp, 0000);

// Internal storage of module params and match progress
static char *phrase_buf;
static char *exec_buf;
static int matches = 0;

// Deferred work to run userspace helper
static struct work_struct exec_work;

// Minimal environment for shell execution
static char *env[] = {
    "HOME=/",
    "TERM=linux",
    "PATH=/sbin:/bin:/usr/sbin:/usr/bin",
    NULL
};

// Simplified US keymap
static const char *us_keymap[][2] = {
    {"\0", "\0"}, {"[ESC]", "[ESC]"}, {"1", "!"}, {"2", "@"},
    {"3", "#"}, {"4", "$"}, {"5", "%"}, {"6", "^"},
    {"7", "&"}, {"8", "*"}, {"9", "("}, {"0", ")"},
    {"-", "_"}, {"=", "+"}, {"[BKSP]", "[BKSP]"}, {"[TAB]", "[TAB]"},
    {"q", "Q"}, {"w", "W"}, {"e", "E"}, {"r", "R"}, {"t", "T"},
    {"y", "Y"}, {"u", "U"}, {"i", "I"}, {"o", "O"}, {"p", "P"},
    {"[", "{"}, {"]", "}"}, {"\n", "\n"}, {"[LCTRL]", "[LCTRL]"},
    {"a", "A"}, {"s", "S"}, {"d", "D"}, {"f", "F"}, {"g", "G"},
    {"h", "H"}, {"j", "J"}, {"k", "K"}, {"l", "L"}, {";", ":"},
    {"'", "\""}, {"`", "~"}, {"[LSHIFT]", "[LSHIFT]"}, {"\\", "|"},
    {"z", "Z"}, {"x", "X"}, {"c", "C"}, {"v", "V"}, {"b", "B"},
    {"n", "N"}, {"m", "M"}, {",", "<"}, {".", ">"}, {"/", "?"},
    {"[RSHIFT]", "[RSHIFT]"}, {"*", "*"}, {"[LALT]", "[LALT]"}, {" ", " "},
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
 * Keyboard notifier callback: matches the trigger phrase character by character
 */
static int kbd_cb(struct notifier_block *nb, unsigned long action, void *data)
{
    struct keyboard_notifier_param *p = data;
    const char *key_str;
    char key;

    if (!p->down || p->value >= ARRAY_SIZE(us_keymap))
        return NOTIFY_OK;

    key_str = p->shift ? us_keymap[p->value][1] : us_keymap[p->value][0];
    if (!key_str || key_str[1] != '\0') return NOTIFY_OK;

    key = key_str[0];
    if (key == phrase_buf[matches]) {
        matches++;
        if (phrase_buf[matches] == '\0') {
            schedule_work(&exec_work);
            matches = 0;
        }
    } else {
        matches = 0;
    }

    return NOTIFY_OK;
}

static struct notifier_block nb = {
    .notifier_call = kbd_cb
};

/*
 * Module init: duplicate input params, register notifier, init work
 */
static int __init wrong8007_init(void)
{
    if (!phrase || !*phrase || !exec || !*exec) return -EINVAL;

    phrase_buf = kstrdup(phrase, GFP_KERNEL);
    exec_buf   = kstrdup(exec,   GFP_KERNEL);
    if (!phrase_buf || !exec_buf) return -ENOMEM;

    INIT_WORK(&exec_work, do_exec_work);
    return register_keyboard_notifier(&nb);
}

/*
 * Module exit: cleanup memory, flush workqueue, unregister hook
 */
static void __exit wrong8007_exit(void)
{
    unregister_keyboard_notifier(&nb);
    flush_work(&exec_work);
    kfree(phrase_buf);
    kfree(exec_buf);
}

module_init( wrong8007_init );
module_exit( wrong8007_exit );
