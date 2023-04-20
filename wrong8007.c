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
#include <linux/version.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/kmod.h>


static char *phrase;
static char *exec;
module_param(phrase, charp, 0000);
module_param(exec, charp, 0000);

static int matches = 0;
static char *env[] = {
    "USER=root",
    "HOME=/",
    "TERM=linux",
    "SHELL=/bin/bash",
    "PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin",
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

static int shell_exec(const char *e) {
#ifdef DEBUG
    printk(KERN_INFO "wrong8007: executing %s\n", e);
#endif

    matches = 0;

    struct subprocess_info *info;
    char *argv[] = { "/bin/sh", "-c", (char *)e, NULL };

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
    info = call_usermodehelper_setup(argv[0], argv, env, GFP_ATOMIC, NULL, NULL, NULL);
#else
    info = call_usermodehelper_setup(argv[0], argv, env, GFP_ATOMIC);
#endif

    if (!info) {
#ifdef DEBUG
        printk(KERN_ERR "wrong8007: error! *info struct is NULL\n");
#endif
        return -ENOMEM;
    }

    int ret = call_usermodehelper_exec(info, UMH_NO_WAIT);
#ifdef DEBUG
    printk(KERN_INFO "wrong8007: usermodehelper_exec returned: %d\n", ret);
#endif
    return ret;
}

static int kbd_notifier_cb(struct notifier_block *nb, unsigned long action, void *data) {
    struct keyboard_notifier_param *param = data;
    const char *key_str;
    char key;

    if (!param->down || param->value >= ARRAY_SIZE(us_keymap))
        return NOTIFY_OK;

    key_str = param->shift ? us_keymap[param->value][1] : us_keymap[param->value][0];

    if (!key_str || key_str[1] != '\0')  // skip multi-char keys like [TAB]
        return NOTIFY_OK;

    key = key_str[0];

    printk(KERN_INFO "Key pressed: %c, match idx: %d\n", key, matches);

    if (key == phrase[matches]) {
        matches++;
        if (phrase[matches] == '\0') {
            printk(KERN_INFO "Phrase matched! Executing: %s\n", exec);
            shell_exec(exec);
            matches = 0;
        }
    } else {
        matches = 0;
    }

    return NOTIFY_OK;
}

static struct notifier_block nblock = {
    .notifier_call = kbd_notifier_cb
};

static int __init wrong8007_init(void) {
#ifdef DEBUG
    printk(KERN_INFO "wrong8007 loaded...");
#endif
    if (!phrase || !*phrase || !exec || !*exec) {
        pr_err("Invalid phrase or exec\n");
        return -EINVAL;
    }

    int ret = register_keyboard_notifier(&nblock);
    if (ret)
        return ret;

    return 0;
}

static void __exit wrong8007_exit(void) {
#ifdef DEBUG
    pr_info("wrong8007: unloaded\n");
#endif
    unregister_keyboard_notifier(&nblock);
}

module_init( wrong8007_init );
module_exit( wrong8007_exit );
MODULE_LICENSE("GPL");
