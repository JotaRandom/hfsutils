/*
 * common_utils.h - Common utilities for embedded HFS tools
 * Copyright (C) 2025 Pablo Lezaeta
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include "hfs_detect.h"

/* Program types for automatic detection */
typedef enum {
    PROGRAM_UNKNOWN = 0,
    PROGRAM_MKFS_HFS,
    PROGRAM_MKFS_HFSPLUS,
    PROGRAM_FSCK_HFS,
    PROGRAM_FSCK_HFSPLUS,
    PROGRAM_MOUNT_HFS,
    PROGRAM_MOUNT_HFSPLUS
} program_type_t;

/* Common initialization and cleanup */
int common_init(const char *program_name, int verbose);
void common_cleanup(void);

/* Program type detection */
program_type_t common_detect_program_type(const char *program_name);
hfs_fs_type_t common_get_fs_type_from_program(program_type_t prog_type);
int common_validate_fs_type(program_type_t prog_type, hfs_fs_type_t detected_type);

/* Version and license information */
void common_print_version(const char *program_name);
void common_print_license(void);

/* Utility functions */
int common_parse_partition_number(const char *str, int *partition);
char *common_resolve_device_path(const char *path);
int common_check_root_required(const char *device_path, int write_access);

#endif /* COMMON_UTILS_H */