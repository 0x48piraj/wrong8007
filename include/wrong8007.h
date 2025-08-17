// SPDX-License-Identifier: GPL-2.0
/*
 * wrong8007: shared definitions, data structures, and function prototypes
 *
 * Copyright (c) 2023, Piyush Raj (https://piyushraj.org/)
 */
#ifndef WRONG8007_H
#define WRONG8007_H

/* API for triggers to schedule a userspace exec */
void wrong8007_schedule_exec(const char *cmd);

struct wrong8007_trigger {
    const char *name;
    int (*init)(void);
    void (*exit)(void);

    /* Optional per-trigger exec command (module_param storage) */
    const char *exec_cmd;
};

#endif
