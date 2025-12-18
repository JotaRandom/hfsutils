/*
 * mount_common.h - Common declarations for mount.hfs/mount.hfs+
 * Copyright (C) 2025
 */

#ifndef MOUNT_COMMON_H
#define MOUNT_COMMON_H

#include <sys/types.h>

/* Standard mount exit codes */
#define MOUNT_OK                0   /* Success */
#define MOUNT_USAGE_ERROR       1   /* Incorrect invocation or permissions */
#define MOUNT_SYSTEM_ERROR      2   /* System error */
#define MOUNT_INTERNAL_ERROR    4   /* Internal mount bug */
#define MOUNT_USER_INTERRUPT    8   /* User interrupt */
#define MOUNT_MTAB_ERROR       16   /* Problems with /etc/mtab */
#define MOUNT_FAILURE          32   /* Mount failure */
#define MOUNT_PARTIAL_SUCCESS  64   /* Some mount succeeded */

/* Mount options structure */
typedef struct {
    char *device;
    char *mountpoint;
    int read_only;
    int read_write;
    int verbose;
    int show_help;
    int show_version;
    char *options;
} mount_options_t;

/* Filesystem type */
typedef enum {
    FS_TYPE_HFS,
    FS_TYPE_HFSPLUS
} fs_type_t;

/* Function prototypes */
int mount_parse_options(int argc, char **argv, mount_options_t *opts);
int mount_hfs_volume(const char *device, const char *mountpoint, 
                     const mount_options_t *opts);
int mount_hfsplus_volume(const char *device, const char *mountpoint,
                         const mount_options_t *opts);
fs_type_t detect_program_type(const char *progname);

#endif /* MOUNT_COMMON_H */
