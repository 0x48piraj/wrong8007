// SPDX-License-Identifier: GPL-2.0
/*
 * wrong8007: keyboard trigger
 *
 * Copyright (c) 2023, 03C0 (https://03c0.net/)
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
static const char us_keymap[][2][8] = {
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

    // Skip key release or invalid index
    if (!p->down || p->value >= ARRAY_SIZE(us_keymap))
        return NOTIFY_OK;

    key_str = us_keymap[p->value][p->shift ? 1 : 0];

    // Skip if special key (length != 1)
    if (strlen(key_str) != 1) return NOTIFY_OK;

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
