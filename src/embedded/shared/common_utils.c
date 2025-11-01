/*
 * common_utils.c - Common utilities for embedded HFS tools
 * Copyright (C) 2025 Pablo Lezaeta
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include "common_utils.h"
#include "version.h"
#include "error_utils.h"
#include "suid.h"

/*
 * NAME:    common_init()
 * DESCRIPTION: Initialize common utilities
 */
int common_init(const char *program_name, int verbose)
{
    /* Initialize SUID handling */
    suid_init();
    
    /* Set up error reporting */
    error_set_program_name(program_name);
    error_set_verbose(verbose);
    
    /* Initialize error logging */
    if (error_init_log("hfsutils.log") == -1) {
        error_warning("could not initialize error logging");
    }
    
    return 0;
}

/*
 * NAME:    common_cleanup()
 * DESCRIPTION: Cleanup common utilities
 */
void common_cleanup(void)
{
    error_cleanup_log();
}

/*
 * NAME:    common_detect_program_type()
 * DESCRIPTION: Detect program type from program name
 */
program_type_t common_detect_program_type(const char *program_name)
{
    char *name_copy, *base_name;
    program_type_t type = PROGRAM_UNKNOWN;
    
    if (!program_name) {
        return PROGRAM_UNKNOWN;
    }
    
    name_copy = strdup(program_name);
    if (!name_copy) {
        return PROGRAM_UNKNOWN;
    }
    
    base_name = basename(name_copy);
    
    /* Check for mkfs variants */
    if (strstr(base_name, "mkfs")) {
        if (strstr(base_name, "hfs+") || strstr(base_name, "hfsplus")) {
            type = PROGRAM_MKFS_HFSPLUS;
        } else if (strstr(base_name, "hfs")) {
            type = PROGRAM_MKFS_HFS;
        }
    }
    /* Check for fsck variants */
    else if (strstr(base_name, "fsck")) {
        if (strstr(base_name, "hfs+") || strstr(base_name, "hfsplus")) {
            type = PROGRAM_FSCK_HFSPLUS;
        } else if (strstr(base_name, "hfs")) {
            type = PROGRAM_FSCK_HFS;
        }
    }
    /* Check for mount variants */
    else if (strstr(base_name, "mount")) {
        if (strstr(base_name, "hfs+") || strstr(base_name, "hfsplus")) {
            type = PROGRAM_MOUNT_HFSPLUS;
        } else if (strstr(base_name, "hfs")) {
            type = PROGRAM_MOUNT_HFS;
        }
    }
    
    free(name_copy);
    return type;
}

/*
 * NAME:    common_get_fs_type_from_program()
 * DESCRIPTION: Get filesystem type from program type
 */
hfs_fs_type_t common_get_fs_type_from_program(program_type_t prog_type)
{
    switch (prog_type) {
        case PROGRAM_MKFS_HFS:
        case PROGRAM_FSCK_HFS:
        case PROGRAM_MOUNT_HFS:
            return FS_TYPE_HFS;
            
        case PROGRAM_MKFS_HFSPLUS:
        case PROGRAM_FSCK_HFSPLUS:
        case PROGRAM_MOUNT_HFSPLUS:
            return FS_TYPE_HFSPLUS;
            
        default:
            return FS_TYPE_UNKNOWN;
    }
}

/*
 * NAME:    common_validate_fs_type()
 * DESCRIPTION: Validate filesystem type matches program expectations
 */
int common_validate_fs_type(program_type_t prog_type, hfs_fs_type_t detected_type)
{
    hfs_fs_type_t expected_type = common_get_fs_type_from_program(prog_type);
    
    if (expected_type == FS_TYPE_UNKNOWN) {
        return 0;  /* No specific requirement */
    }
    
    /* Allow HFS+ tools to work with HFSX */
    if (expected_type == FS_TYPE_HFSPLUS && detected_type == FS_TYPE_HFSX) {
        return 0;
    }
    
    return (expected_type == detected_type) ? 0 : -1;
}

/*
 * NAME:    common_print_version()
 * DESCRIPTION: Print version information
 */
void common_print_version(const char *program_name)
{
    printf("%s (%s)\n", program_name, hfsutils_version);
    printf("%s", hfsutils_copyright);
    printf("\n");
    printf("This is free software; see the source for copying conditions.\n");
    printf("There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A\n");
    printf("PARTICULAR PURPOSE.\n");
}

/*
 * NAME:    common_print_license()
 * DESCRIPTION: Print license information
 */
void common_print_license(void)
{
    printf("%s", hfsutils_license);
}

/*
 * NAME:    common_parse_partition_number()
 * DESCRIPTION: Parse partition number from string
 */
int common_parse_partition_number(const char *str, int *partition)
{
    char *endptr;
    long val;
    
    if (!str || !partition) {
        return -1;
    }
    
    val = strtol(str, &endptr, 10);
    
    if (*endptr != '\0' || val < 0 || val > 255) {
        return -1;
    }
    
    *partition = (int)val;
    return 0;
}

/*
 * NAME:    common_resolve_device_path()
 * DESCRIPTION: Resolve device path (handle symlinks, etc.)
 */
char *common_resolve_device_path(const char *path)
{
    char *resolved_path;
    
    if (!path) {
        return NULL;
    }
    
    resolved_path = realpath(path, NULL);
    if (!resolved_path) {
        /* If realpath fails, just duplicate the original path */
        resolved_path = strdup(path);
    }
    
    return resolved_path;
}

/*
 * NAME:    common_check_root_required()
 * DESCRIPTION: Check if root privileges are required for operation
 */
int common_check_root_required(const char *device_path, int write_access)
{
    /* For block devices, root is typically required */
    /* For regular files, check file permissions */
    
    if (write_access) {
        /* Write operations typically require elevated privileges */
        if (geteuid() != 0) {
            error_warning("write operations may require root privileges");
            return 1;
        }
    }
    
    return 0;
}