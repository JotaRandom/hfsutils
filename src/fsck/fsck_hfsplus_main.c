/*
 * fsck_hfsplus_main.c - Main entry point for fsck.hfs+ standalone utility
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
 * DESCRIPTION: Display comprehensive usage information for fsck.hfs+
 */
static void show_usage(const char *program_name)
{
    printf("Usage: %s [options] device-path [partition-no]\n", program_name);
    printf("\n");
    printf("Check and repair HFS+ filesystems with journaling support.\n");
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
    printf("HFS+ Journaling:\n");
    printf("  This fsck supports HFS+ journaling with automatic journal replay\n");
    printf("  for crash recovery. Corrupted journals are detected and can be\n");
    printf("  automatically disabled during repair operations.\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s /dev/sdb1              Check HFS+ filesystem\n", program_name);
    printf("  %s -v /dev/sdb1           Check with verbose output\n", program_name);
    printf("  %s -n /dev/sdb1           Check without making changes\n", program_name);
    printf("  %s -a /dev/sdb1           Check and auto-repair\n", program_name);
    printf("\n");
    printf("Note: This program only works with HFS+ filesystems.\n");
    printf("      For HFS filesystems, use fsck.hfs instead.\n");
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
 * DESCRIPTION: Main entry point for fsck.hfs+
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
    
    /* Ensure we're running as an HFS+ program */
    if (prog_type != PROGRAM_FSCK_HFSPLUS) {
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
    
    /* Validate filesystem type matches program type - HFS+ only */
    if (fs_type != FS_TYPE_HFSPLUS && fs_type != FS_TYPE_HFSX) {
        const char *detected_name = hfs_get_fs_type_name(fs_type);
        error_print("filesystem type mismatch: detected %s filesystem", detected_name);
        error_print("This program only works with HFS+ filesystems.");
        if (fs_type == FS_TYPE_HFS) {
            error_print("For HFS filesystems, use fsck.hfs instead.");
        }
        fsck_cleanup_options(&opts);
        common_cleanup();
        return FSCK_OPERATIONAL_ERROR;
    }
    
    /* Use enhanced HFS+ checking with journal support */
    result = hfsplus_check_volume(opts.device_path, opts.partition_number, options);
    
    /* Cleanup */
    fsck_cleanup_options(&opts);
    common_cleanup();
    
    return result;
}