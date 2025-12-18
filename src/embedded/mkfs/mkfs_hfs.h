/*
 * mkfs_hfs.h - Header file for mkfs.hfs embedded dependencies
 * Copyright (C) 1996-1998 Robert Leslie
 */

#ifndef MKFS_HFS_H
#define MKFS_HFS_H

/* Include common HFS utilities */
#include "../shared/hfs_common.h"
#include "../shared/common_utils.h"
#include "../shared/device_utils.h"
#include "../shared/error_utils.h"
#include "../shared/version.h"
#include "../shared/suid.h"

/* Standard mkfs exit codes */
#define MKFS_OK                 0   /* Success */
#define MKFS_GENERAL_ERROR      1   /* General error */
#define MKFS_USAGE_ERROR        2   /* Usage error */
#define MKFS_OPERATIONAL_ERROR  4   /* Operational error */
#define MKFS_SYSTEM_ERROR       8   /* System error */

/* Command-line options structure */
typedef struct {
    char *device_path;
    char *volume_name;
    hfs_fs_type_t filesystem_type;
    int partition_number;
    int force;
    int verbose;
    int show_version;
    int show_help;
    int show_license;
    int block_size;       /* Block size (0 = auto-calculate) */
    long long total_size; /* Total size in bytes (0 = use full device) */
    int enable_journaling; /* Enable HFS+ journaling (0 = disabled, 1 = enabled) */
} mkfs_options_t;

/* Main formatting functions */
int mkfs_hfs_format(const char *device_path, const mkfs_options_t *opts);
int mkfs_hfsplus_format(const char *device_path, const mkfs_options_t *opts);

#endif /* MKFS_HFS_H */