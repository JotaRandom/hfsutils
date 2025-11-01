/*
 * device_utils.h - Device detection and partitioning utilities
 * Copyright (C) 2025 Pablo Lezaeta
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef DEVICE_UTILS_H
#define DEVICE_UTILS_H

#include <sys/types.h>

/* Partition table types */
typedef enum {
    PARTITION_UNKNOWN = 0,
    PARTITION_APPLE,    /* Apple Partition Map */
    PARTITION_MBR,      /* Master Boot Record */
    PARTITION_GPT       /* GUID Partition Table */
} partition_type_t;

/* Device validation and access */
int device_validate(const char *path, int flags);
off_t device_get_size(const char *path);
int device_is_mounted(const char *path);

/* Partition detection */
partition_type_t partition_detect_type(const char *path);
int partition_count(const char *path);

/* Internal partition counting functions (implemented in .c file) */
int partition_count_apple(const char *path);
int partition_count_mbr(const char *path);
int partition_count_gpt(const char *path);

#endif /* DEVICE_UTILS_H */