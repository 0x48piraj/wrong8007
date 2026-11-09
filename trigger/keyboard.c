// SPDX-License-Identifier: GPL-2.0
/*
 * wrong8007: keyboard trigger
 *
 * Copyright (c) 2023, 03C0 (https://03c0.net/)
 */

#include <linux/keyboard.h>
#include <linux/notifier.h>
#include <linux/string.h>
#include <linux/spinlock.h>

#include <wrong8007.h>
#include <phrase_match.h>

static char *phrase;
module_param(phrase, charp, 0000);
MODULE_PARM_DESC(phrase, "case-sensitive phrase that activates the keyboard trigger");

/* Keyboard trigger state */
static struct wb_phrase_matcher matcher;
static bool kbd_registered;
static DEFINE_SPINLOCK(match_lock);

/*
 * Encode a Unicode codepoint as UTF-8.
 *
 * Returns the number of bytes written, or 0 if the codepoint
 * is outside the supported range.
 */
static int utf8_encode(char *out, unsigned int cp)
{
    if (cp < 0x80) {
        out[0] = (char)cp;
        return 1;
    }
    if (cp < 0x800) {
        out[0] = (char)(0xc0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3f));
        return 2;
    }
    if (cp < 0x10000) {
        out[0] = (char)(0xe0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3f));
        out[2] = (char)(0x80 | (cp & 0x3f));
        return 3;
    }
    return 0;
}

/*
 * Decode a printable character from a KBD_KEYSYM notification.
 *
 * Returns the UTF-8 encoding of the resolved character, or 0 if the
 * notification does not represent printable input.
 */
static int decode_keysym(unsigned int value, char *out)
{
    unsigned char type = KTYP(value);
    unsigned char v = KVAL(value);

    if (type >= 0xf0)
        type -= 0xf0;

    if (type != KT_LATIN && type != KT_LETTER)
        return 0;

    return utf8_encode(out, v);
}

/*
 * Decode a printable character from a KBD_UNICODE notification.
 *
 * Returns the UTF-8 encoding of the resolved character, or 0 if the
 * notification does not represent printable input.
 */
static int decode_unicode(unsigned int value, char *out)
{
    if (KTYP(value) != KT_LATIN)
        return 0;

    return utf8_encode(out, KVAL(value));
}

/*
 * Match the configured trigger phrase against the printable characters
 * produced by the keyboard notifier.
 *
 * By operating on resolved input rather than raw keycodes, phrase
 * matching follows the active keyboard layout automatically.
 */
static int kbd_cb(struct notifier_block *nb, unsigned long action, void *data)
{
    struct keyboard_notifier_param *p = data;
    char bytes[3];
    int clen;
    int i;
    unsigned long flags;

    if (p->down != 1)
        return NOTIFY_OK;

    /* Ignore raw keycodes; match only resolved characters */
    if (action == KBD_KEYSYM)
        clen = decode_keysym(p->value, bytes);
    else if (action == KBD_UNICODE)
        clen = decode_unicode(p->value, bytes);
    else
        return NOTIFY_OK;

    if (clen <= 0)
        return NOTIFY_OK;

    spin_lock_irqsave(&match_lock, flags);

    for (i = 0; i < clen; i++) {
        if (wb_phrase_matcher_feed(&matcher, bytes[i])) {
            wb_info("keyboard trigger matched, scheduling exec\n");
            wrong8007_activate();
            break;
        }
    }

    spin_unlock_irqrestore(&match_lock, flags);

    return NOTIFY_OK;
}

static struct notifier_block nb = {
    .notifier_call = kbd_cb
};

static int trigger_keyboard_init(void)
{
    int ret;

    if (!phrase || !*phrase) {
        wb_warn("keyboard trigger disabled (no phrase)\n");
        return 0; // success, no hook
    }

    ret = wb_phrase_matcher_init(&matcher, phrase);
    if (ret) {
        wb_err("phrase too long (max %d bytes)\n", WB_PHRASE_MAX_LEN);
        return ret;
    }

    ret = register_keyboard_notifier(&nb);
    if (ret) {
        wb_err("failed to register keyboard notifier (err=%d)\n", ret);
        return ret;
    }

    kbd_registered = true;

    wb_info("keyboard trigger initialized\n");
    return 0;
}

static void trigger_keyboard_exit(void)
{
    if (!kbd_registered)
        return;

    unregister_keyboard_notifier(&nb);
    kbd_registered = false;
    wb_phrase_matcher_reset(&matcher);
    wb_info("keyboard trigger exited\n");
}

struct wrong8007_trigger keyboard_trigger = {
    .name = "keyboard",
    .init = trigger_keyboard_init,
    .exit = trigger_keyboard_exit
};
