// SPDX-License-Identifier: GPL-2.0
/*
 * wrong8007: phrase matcher
 *
 * Copyright (c) 2023, 03C0 (https://03c0.net/)
 */

#include <linux/errno.h>
#include <linux/types.h>
#include <linux/string.h>

#include <phrase_match.h>

int wb_phrase_matcher_init(struct wb_phrase_matcher *m, const char *phrase)
{
    unsigned int len;
    unsigned int i;
    unsigned int k = 0;

    if (!phrase)
        return -EINVAL;

    len = strnlen(phrase, WB_PHRASE_MAX_LEN + 1);
    if (len == 0 || len > WB_PHRASE_MAX_LEN)
        return -EINVAL;

    m->phrase = phrase;
    m->len = len;
    m->matched = 0;

    m->lps[0] = 0;
    for (i = 1; i < len; i++) {
        while (k > 0 && phrase[i] != phrase[k])
            k = m->lps[k - 1];
        if (phrase[i] == phrase[k])
            k++;
        m->lps[i] = k;
    }

    return 0;
}

void wb_phrase_matcher_reset(struct wb_phrase_matcher *m)
{
    m->matched = 0;
}

bool wb_phrase_matcher_feed(struct wb_phrase_matcher *m, char c)
{
    while (m->matched > 0 && c != m->phrase[m->matched])
        m->matched = m->lps[m->matched - 1];

    if (c == m->phrase[m->matched])
        m->matched++;

    if (m->matched == m->len) {
        m->matched = 0;
        return true;
    }

    return false;
}
