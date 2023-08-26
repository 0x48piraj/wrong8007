// SPDX-License-Identifier: GPL-2.0
/*
 * wrong8007: shared definitions, data structures, and function prototypes
 *
 * Copyright (c) 2023, Piyush Raj (https://piyushraj.org/)
 */
#ifndef WRONG8007_H
#define WRONG8007_H

#include <linux/workqueue.h>

extern struct work_struct exec_work;
extern char *phrase_buf;
extern char *exec_buf;

struct wrong8007_trigger {
    const char *name;
    int (*init)(void);
    void (*exit)(void);
};

#endif
