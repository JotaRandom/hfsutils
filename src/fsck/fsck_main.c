/*
 * fsck_main.c - Main entry point for fsck.hfs standalone utility
 * Copyright (C) 2025 Pablo Lezaeta
 * Based on hfsck by Robert Leslie
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
#include <errno.h>
#include <sys/stat.h>

#include "../embedded/fsck/fsck_hfs.h"
#include "fsck_common.h"
#include "journal.h"

/* Global variables */
const char *argv0, *bargv0;
extern int options;

/*
 * NAME:    show_usage()
 * DESCRIPTION: Display comprehensive usage information for fsck.hfs
 */
static void show_usage(const char *program_name)
{
    printf("Usage: %s [options] device-path [partition-no]\n", program_name);
    printf("\n");
    printf("Check and repair HFS filesystems.\n");
    printf("\n");
    printf("Options:\n");
    printf("  -v, --verbose     Display detailed information during check\n");
    printf("  -n, --no-write    Check filesystem but make no changes (read-only)\n");
    printf("  -a, --auto        Automatically repair filesystem without prompting\n");
    printf("  -f, --force       Force checking even if filesystem appears clean\n");
    printf("  -y, --yes         Assume 'yes' to all questions (same as -a)\n");
    printf("  -p                Automatically repair filesystem (same as -a)\n");
    printf("  -r                Interactively repair filesystem\n");
    printf("  -V, --version     Display version information and exit\n");
    printf("  -h, --help        Display this help message and exit\n");
    printf("      --license     Display license information and exit\n");
    printf("\n");
    printf("Exit codes:\n");
    printf("  0   No errors found\n");
    printf("  1   Errors found and corrected\n");
    printf("  2   System should be rebooted\n");
    printf("  4   Errors found but not corrected\n");
    printf("  8   Operational error\n");
    printf("  16  Usage or syntax error\n");
    printf("  32  fsck canceled by user request\n");
    printf("  128 Shared library error\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s /dev/sdb1              Check HFS filesystem\n", program_name);
    printf("  %s -v /dev/sdb1           Check with verbose output\n", program_name);
    printf("  %s -n /dev/sdb1           Check without making changes\n", program_name);
    printf("  %s -a /dev/sdb1           Check and auto-repair\n", program_name);
    printf("\n");
    printf("Note: This program automatically detects the filesystem type.\n");
    printf("      HFS+ filesystems are automatically delegated to fsck.hfs+.\n");
    printf("\n");
}

/*
 * NAME:    show_version()
 * DESCRIPTION: Display version information
 */
static void show_version(const char *program_name)
{
    printf("%s (hfsutils) 4.1.0B\n", program_name);
    printf("Copyright (C) 2025 Pablo Lezaeta\n");
    printf("Based on hfsutils by Robert Leslie\n");
    printf("This is free software; see the source for copying conditions.\n");
}

/*
 * NAME:    main()
 * DESCRIPTION: Main entry point for fsck.hfs
 */
int main(int argc, char *argv[])
{
    fsck_options_t opts = {0};
    program_type_t prog_type;
    hfs_fs_type_t fs_type;
    int result;
    
    /* Initialize program name */
    argv0 = bargv0 = argv[0];
    
    /* Detect program type from name */
    prog_type = common_detect_program_type(argv[0]);
    
    /* Ensure we're running as an HFS program */
    if (prog_type != PROGRAM_FSCK_HFS) {
        error_print("internal error: program type detection failed");
        return FSCK_OPERATIONAL_ERROR;
    }
    
    /* Parse command-line arguments */
    if (fsck_parse_command_line(argc, argv, &opts) != 0) {
        show_usage(argv[0]);
        return FSCK_USAGE_ERROR;
    }
    
    /* Handle special options */
    if (opts.show_help) {
        show_usage(argv[0]);
        fsck_cleanup_options(&opts);
        return FSCK_OK;
    }
    
    if (opts.show_version) {
        show_version(argv[0]);
        fsck_cleanup_options(&opts);
        return FSCK_OK;
    }
    
    if (opts.show_license) {
        fsck_show_license(argv[0]);
        fsck_cleanup_options(&opts);
        return FSCK_OK;
    }
    
    /* Validate options */
    if (fsck_validate_options(&opts) != 0) {
        show_usage(argv[0]);
        fsck_cleanup_options(&opts);
        return FSCK_USAGE_ERROR;
    }
    
    /* Initialize common utilities */
    if (common_init(argv[0], opts.verbose) != 0) {
        error_print("failed to initialize utilities");
        fsck_cleanup_options(&opts);
        return FSCK_OPERATIONAL_ERROR;
    }
    
    /* Set global options for compatibility with original hfsck */
    options = 0;
    if (opts.repair) options |= HFSCK_REPAIR;
    if (opts.verbose) options |= HFSCK_VERBOSE;
    if (opts.yes_to_all) options |= HFSCK_YES;
    
    /* Detect filesystem type */
    fs_type = hfs_detect_filesystem_type(opts.device_path, opts.partition_number);
    if (fs_type == FS_TYPE_UNKNOWN) {
        error_print("unable to detect filesystem type on %s", opts.device_path);
        fsck_cleanup_options(&opts);
        common_cleanup();
        return FSCK_OPERATIONAL_ERROR;
    }
    
    /* Handle filesystem type - delegate to appropriate checker */
    if (fs_type != FS_TYPE_HFS) {
        const char *detected_name = hfs_get_fs_type_name(fs_type);
        
        /* If HFS+ or HFSX detected, automatically delegate to fsck.hfs+ */
        if (fs_type == FS_TYPE_HFSPLUS || fs_type == FS_TYPE_HFSX) {
            if (opts.verbose) {
                printf("Detected %s filesystem, delegating to fsck.hfs+...\n", detected_name);
            }
            
            /* Build new argv for fsck.hfs+ */
            char **new_argv = malloc((argc + 1) * sizeof(char *));
            if (!new_argv) {
                error_print("failed to allocate memory for delegation");
                fsck_cleanup_options(&opts);
                common_cleanup();
                return FSCK_OPERATIONAL_ERROR;
            }
            
            /* Replace program name with fsck.hfs+ */
            new_argv[0] = "fsck.hfs+";
            for (int i = 1; i < argc; i++) {
                new_argv[i] = argv[i];
            }
            new_argv[argc] = NULL;
            
            /* Clean up before exec */
            fsck_cleanup_options(&opts);
            common_cleanup();
            
            /* Execute fsck.hfs+ */
            execvp("fsck.hfs+", new_argv);
            
            /* If execvp returns, it failed */
            /* Check if failure was due to missing fsck.hfs+ */
            if (errno == ENOENT) {
                fprintf(stderr, "Error: fsck.hfs+ not found in PATH\n");
                fprintf(stderr, "Cannot check %s filesystem without fsck.hfs+\n", detected_name);
                fprintf(stderr, "\n");
                fprintf(stderr, "Options:\n");
                fprintf(stderr, "  1. Install fsck.hfs+ (build with: make fsck.hfs+)\n");
                fprintf(stderr, "  2. Use a system that has fsck.hfs+ installed\n");
                fprintf(stderr, "  3. Mount the volume read-only without checking\n");
            } else {
                error_print_errno("failed to execute fsck.hfs+");
            }
            free(new_argv);
            return FSCK_OPERATIONAL_ERROR;
        }
        
        /* Unknown or unsupported filesystem type */
        error_print("unsupported filesystem type: %s", detected_name);
        error_print("This program only handles HFS and HFS+ filesystems.");
        fsck_cleanup_options(&opts);
        common_cleanup();
        return FSCK_OPERATIONAL_ERROR;
    }
    
    /* Use standard HFS checking */
    result = hfs_check_volume(opts.device_path, opts.partition_number, options);
    
    /* Cleanup */
    fsck_cleanup_options(&opts);
    common_cleanup();
    
    return result;
}