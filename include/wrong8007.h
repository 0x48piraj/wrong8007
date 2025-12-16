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

#define WB_TAG "wrong8007: "

#ifdef DEBUG
    #define wb_dbg(fmt, ...)  pr_info(WB_TAG "[DEBUG] " fmt, ##__VA_ARGS__)
#else
    #define wb_dbg(fmt, ...)  do {} while (0)
#endif

#define wb_info(fmt, ...) pr_info(WB_TAG fmt, ##__VA_ARGS__)
#define wb_warn(fmt, ...) pr_warn(WB_TAG fmt, ##__VA_ARGS__)
#define wb_err(fmt, ...)  pr_err(WB_TAG fmt, ##__VA_ARGS__)

struct wrong8007_trigger {
    const char *name;
    int (*init)(void);
    void (*exit)(void);
};

void wrong8007_activate(void);

#endif
