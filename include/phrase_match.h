// SPDX-License-Identifier: GPL-2.0
/*
 * wrong8007: phrase matcher
 *
 * Copyright (c) 2023, 03C0 (https://03c0.net/)
 */

#ifndef WB_PHRASE_MATCH_H
#define WB_PHRASE_MATCH_H

#include <linux/types.h>

#define WB_PHRASE_MAX_LEN 128

struct wb_phrase_matcher {
    const char *phrase;
    unsigned int len;
    unsigned int matched;
    unsigned char lps[WB_PHRASE_MAX_LEN]; // KMP prefix table
};

/*
 * Prepares the matcher for phrase (precompute the KMP table). The phrase
 * must outlive the matcher.
 *
 * Returns 0 on success, -EINVAL for NULL, empty, or oversized phrases.
 */
int wb_phrase_matcher_init(struct wb_phrase_matcher *m, const char *phrase);

/* Reset any partial match */
void wb_phrase_matcher_reset(struct wb_phrase_matcher *m);

/*
 * Feed one byte to the matcher.
 *
 * Returns true when the phrase is matched. Matching then restarts
 * from the beginning.
 */
bool wb_phrase_matcher_feed(struct wb_phrase_matcher *m, char c);

#endif /* WB_PHRASE_MATCH_H */
