// SPDX-License-Identifier: GPL-2.0
/*
 * wrong8007: shared definitions, data structures, and function prototypes
 *
 * Copyright (c) 2023, 03C0 (https://03c0.net/)
 */
#ifndef WRONG8007_H
#define WRONG8007_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>

// Declare the external exec_work from core module
extern struct work_struct exec_work;
extern char *phrase_buf;
extern char *exec_buf;

struct wrong8007_trigger {
    const char *name;
    int (*init)(void);
    void (*exit)(void);
};

#endif
