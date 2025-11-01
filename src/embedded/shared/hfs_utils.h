/*
 * hfs_utils.h - Additional HFS utility functions
 * Copyright (C) 2025 Pablo Lezaeta
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef HFS_UTILS_H
#define HFS_UTILS_H

#include <time.h>

/* Time conversion functions */
unsigned long d_mtime(time_t unix_time);
unsigned long d_ltime(unsigned long mac_time);

/* User interaction */
int ask(const char *question, ...);

/* Global options variable */
extern int options;

#endif /* HFS_UTILS_H */