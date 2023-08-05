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
#include <linux/keyboard.h>
#include <linux/notifier.h>
#include <linux/string.h>

#include <wrong8007.h>

static char *phrase;

module_param(phrase, charp, 0000);
MODULE_PARM_DESC(phrase, "Keyboard input to trigger on (e.g., 'nuke')");

// Internal storage of module params
char *phrase_buf;

// Declare the external exec_work from main module
extern struct work_struct exec_work;

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

// Internal storage of match progress
static int matches = 0;

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

    // Skip if special key (length != 1)
    if (!key_str || key_str[1] != '\0') return NOTIFY_OK;

    // Match only printable single chars
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

static int trigger_keyboard_init(void)
{
    int ret;

    if (!phrase || !*phrase) {
        pr_info("wrong8007: keyboard trigger disabled (no phrase)\n");
        return 0; // success, no hook
    }

    phrase_buf = kstrdup(phrase, GFP_KERNEL);
    if (!phrase_buf)
        return -ENOMEM;

    matches = 0; // reset match progress

    ret = register_keyboard_notifier(&nb);
    if (ret) {
        pr_err("wrong8007: failed to register keyboard notifier (err=%d)\n", ret);
        kfree(phrase_buf);
        phrase_buf = NULL;
        return ret;
    }

    pr_info("wrong8007: keyboard trigger initialized (PHRASE=%s)\n", phrase);
    return 0;
}

static void trigger_keyboard_exit(void)
{
    if (!phrase || !*phrase)
        return; // never registered

    unregister_keyboard_notifier(&nb);
    kfree(phrase_buf);
    phrase_buf = NULL;
    pr_info("wrong8007: keyboard trigger exited\n");
}

// Expose as a trigger plugin
struct wrong8007_trigger keyboard_trigger = {
    .name = "keyboard",
    .init = trigger_keyboard_init,
    .exit = trigger_keyboard_exit
};
