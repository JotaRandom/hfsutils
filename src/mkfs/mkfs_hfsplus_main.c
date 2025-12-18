/*
 * mkfs_hfsplus_main.c - Main entry point for mkfs.hfs+ standalone utility
 * Copyright (C) 2025 Pablo Lezaeta
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Based on hformat from hfsutils by Robert Leslie
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <sys/stat.h>

#include "../embedded/mkfs/mkfs_hfs.h"
#include "mkfs_common.h"

/* Program information */
static const char *program_name = "mkfs.hfs+";
static program_type_t program_type = PROGRAM_MKFS_HFSPLUS;

/* Command-line options defined in mkfs_hfs.h */

/* Default options */
static mkfs_options_t default_options = {
    .device_path = NULL,
    .volume_name = NULL,
    .filesystem_type = FS_TYPE_HFSPLUS,  /* Default to HFS+ */
    .partition_number = -1,
    .force = 0,
    .verbose = 0,
    .show_version = 0,
    .show_help = 0,
    .show_license = 0,
    .block_size = 0,  /* Auto-calculate */
    .total_size = 0,  /* Use full device */
    .enable_journaling = 0  /* Disabled by default with warning */
};



/*
 * NAME:    usage()
 * DESCRIPTION: Display usage information
 */
static void usage(int exit_code)
{
    printf("Usage: %s [options] device [partition-no]\n", program_name);
    printf("\n");
    printf("Create HFS+ filesystems on devices or files.\n");
    printf("\n");
    printf("Options:\n");
    printf("  -f, --force          Force creation, overwrite existing filesystem\n");
    printf("  -j, --journal        Enable HFS+ journaling (Linux kernel driver does NOT support)\n");
    printf("  -L, --label NAME     Set volume label/name (also accepts -l)\n");
    printf("  -s, --size SIZE      Specify filesystem size in bytes (supports K, M, G suffixes)\n");
    printf("  -v, --verbose        Display detailed formatting information\n");
    printf("  -V, --version        Display version information\n");
    printf("  -h, --help           Display this help message\n");
    printf("      --license        Display license information\n");
    printf("\n");
    printf("Arguments:\n");
    printf("  device               Block device or file to format\n");
    printf("  partition-no         Partition number (optional, 0 for whole device)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s /dev/sdb1                    # Format partition as HFS+\n", program_name);
    printf("  %s -l \"My Volume\" /dev/sdb1     # Format with custom label\n", program_name);
    printf("  %s -s 1073741824 disk.img       # Create 1GB filesystem\n", program_name);
    printf("  %s -f /dev/sdb 1                # Force format partition 1\n", program_name);
    printf("  %s -f /dev/sdb 0                # Format entire disk (erases partition table)\n", program_name);
    printf("  %s -v /dev/fd0                  # Format floppy with verbose output\n", program_name);
    printf("\n");
    printf("HFS+ Features:\n");
    printf("  - Supports volumes larger than 2GB\n");
    printf("  - Unicode filenames (up to 255 characters)\n");
    printf("  - Better performance than HFS\n");
    printf("  - Case-insensitive but case-preserving\n");
    printf("  - Extended attributes support\n");
    printf("\n");
    printf("Exit codes:\n");
    printf("  0   Success\n");
    printf("  1   Error (any kind)\n");
    printf("\n");
    printf("Note: Exit codes follow Unix standard (0=success, 1=error).\n");
    printf("      Use -v for detailed error information.\n");
    printf("\n");
    
    exit(exit_code);
}



/*
 * NAME:    main()
 * DESCRIPTION: Main entry point for mkfs.hfs+
 */
int main(int argc, char *argv[])
{
    mkfs_options_t opts = default_options;
    int result = EXIT_SUCCESS;
    
    /* Initialize common utilities */
    if (common_init(program_name, 0) != 0) {
        fprintf(stderr, "%s: failed to initialize\n", program_name);
        return EXIT_SYSTEM_ERROR;
    }
    
    /* Parse command-line arguments */
    if (mkfs_parse_command_line(argc, argv, &opts, 1) != 0) {
        mkfs_cleanup_options(&opts);
        common_cleanup();
        return EXIT_USAGE_ERROR;
    }
    
    /* Handle special options */
    if (opts.show_version) {
        common_print_version(program_name);
        mkfs_cleanup_options(&opts);
        common_cleanup();
        return EXIT_SUCCESS;
    }
    
    if (opts.show_help) {
        usage(EXIT_SUCCESS);
        /* usage() calls exit(), so this is never reached */
    }
    
    if (opts.show_license) {
        mkfs_show_license(program_name);
        mkfs_cleanup_options(&opts);
        common_cleanup();
        return EXIT_SUCCESS;
    }
    
    /* Enable verbose mode if requested */
    if (opts.verbose) {
        error_set_verbose(1);
    }
    
    /* Validate options */
    if (mkfs_validate_options(&opts, 1) != 0) {
        mkfs_cleanup_options(&opts);
        common_cleanup();
        return EXIT_USAGE_ERROR;
    }
    
    /* Check if root privileges are required */
    common_check_root_required(opts.device_path, 1);  /* Write access required */
    
    /* Perform the formatting operation */
    error_verbose("formatting %s as HFS+ filesystem", opts.device_path);
    
    result = mkfs_hfsplus_format(opts.device_path, &opts);
    
    if (result == 0) {
        error_verbose("HFS+ formatting completed successfully");
    } else {
        error_print("HFS+ formatting failed");
        result = error_get_exit_code(errno);
    }
    
    /* Cleanup and exit */
    mkfs_cleanup_options(&opts);
    common_cleanup();
    
    /* Normalize exit code to Unix standard (0=success, 1=any error) */
    /* Preserve detailed code in verbose mode for debugging */
    if (result != 0) {
        if (opts.verbose) {
            fprintf(stderr, "Internal exit code: %d\n", result);
        }
        return 1;  /* Unix standard: any error = exit code 1 */
    }
    return 0;
}