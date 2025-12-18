/*
 * mkfs_hfs_main.c - Main entry point for mkfs.hfs standalone utility
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
static const char *program_name = "mkfs.hfs";
static program_type_t program_type = PROGRAM_MKFS_HFS;

/* Command-line options defined in mkfs_hfs.h */

/* Default options */
static mkfs_options_t default_options = {
    .device_path = NULL,
    .volume_name = NULL,
    .filesystem_type = FS_TYPE_UNKNOWN,  /* Auto-detect from program name */
    .partition_number = -1,
    .force = 0,
    .verbose = 0,
    .show_version = 0,
    .show_help = 0,
    .show_license = 0,
    .block_size = 0,  /* Auto-calculate */
    .total_size = 0   /* Use full device */
};

/*
 * NAME:    show_license()
 * DESCRIPTION: Display license information
 */
static void show_license(void)
{
    printf("%s - Create HFS/HFS+ filesystems\n", program_name);
    printf("Copyright (C) 2025 Pablo Lezaeta\n");
    printf("Based on hfsutils by Robert Leslie\n");
    printf("\n");
    printf("This program is free software; you can redistribute it and/or modify\n");
    printf("it under the terms of the GNU General Public License as published by\n");
    printf("the Free Software Foundation; either version 2 of the License, or\n");
    printf("(at your option) any later version.\n");
    printf("\n");
    printf("This program is distributed in the hope that it will be useful,\n");
    printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
    printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
    printf("GNU General Public License for more details.\n");
    printf("\n");
    printf("You should have received a copy of the GNU General Public License\n");
    printf("along with this program; if not, write to the Free Software\n");
    printf("Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA\n");
    printf("\n");
}

/*
 * NAME:    usage()
 * DESCRIPTION: Display usage information
 */
static void usage(int exit_code)
{
    const char *fs_type_str = (program_type == PROGRAM_MKFS_HFSPLUS) ? "HFS+" : "HFS";
    
    printf("Usage: %s [options] device [partition-no]\n", program_name);
    printf("\n");
    printf("Create %s filesystems on devices or files.\n", fs_type_str);
    printf("\n");
    printf("Options:\n");
    printf("  -f, --force          Force creation, overwrite existing filesystem\n");
    printf("  -l, --label NAME     Set volume label/name (max 27 characters for HFS)\n");
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
    printf("  %s /dev/sdb1                    # Format partition as %s\n", program_name, fs_type_str);
    printf("  %s -l \"My Volume\" /dev/sdb1     # Format with custom label\n", program_name);
    printf("  %s -f /dev/sdb 1                # Force format partition 1\n", program_name);
    printf("  %s -f /dev/sdb 0                # Format entire disk (erases partition table)\n", program_name);
    printf("  %s -v /dev/fd0                  # Format floppy with verbose output\n", program_name);
    printf("\n");
    printf("Notes:\n");
    printf("  - mkfs.hfs creates HFS filesystems only\n");
    printf("  - For HFS+ filesystems, use mkfs.hfs+ or mkfs.hfsplus\n");
    printf("  - Minimum HFS volume size is 800KB\n");
    printf("  - Maximum HFS volume size is 2GB\n");
    printf("\n");
    printf("Exit codes:\n");
    printf("  0   Success\n");
    printf("  1   General error\n");
    printf("  2   Usage error\n");
    printf("  4   Operational error\n");
    printf("  8   System error\n");
    printf("\n");
    
    exit(exit_code);
}

/* Functions moved to mkfs_common.c */

/*
 * NAME:    parse_command_line()
 * DESCRIPTION: Parse command-line arguments with full hformat compatibility
 */
static int parse_command_line(int argc, char *argv[], mkfs_options_t *opts)
{
    int c;
    static struct option long_options[] = {
        {"force",   no_argument,       0, 'f'},
        {"label",   required_argument, 0, 'l'},
        {"verbose", no_argument,       0, 'v'},
        {"version", no_argument,       0, 'V'},
        {"help",    no_argument,       0, 'h'},
        {"license", no_argument,       0, 1000},
        {0, 0, 0, 0}
    };
    
    while ((c = getopt_long(argc, argv, "fl:vVh", long_options, NULL)) != -1) {
        switch (c) {
            case 'f':
                opts->force = 1;
                break;
                
            case 'l':
                opts->volume_name = strdup(optarg);
                if (!opts->volume_name) {
                    error_print_errno("failed to allocate memory for volume name");
                    return -1;
                }
                if (validate_volume_name_hfs(opts->volume_name) != 0) {
                    return -1;
                }
                break;
                

                
            case 'v':
                opts->verbose = 1;
                break;
                
            case 'V':
                opts->show_version = 1;
                return 0;
                
            case 'h':
                opts->show_help = 1;
                return 0;
                
            case 1000:  /* --license */
                opts->show_license = 1;
                return 0;
                
            case '?':
                /* getopt_long already printed an error message */
                return -1;
                
            default:
                error_print("unknown option");
                return -1;
        }
    }
    
    /* Parse positional arguments */
    if (optind >= argc) {
        error_print("missing device argument");
        return -1;
    }
    
    opts->device_path = strdup(argv[optind]);
    if (!opts->device_path) {
        error_print_errno("failed to allocate memory for device path");
        return -1;
    }
    
    /* Optional partition number */
    if (optind + 1 < argc) {
        if (common_parse_partition_number(argv[optind + 1], &opts->partition_number) != 0) {
            error_print("invalid partition number '%s'", argv[optind + 1]);
            return -1;
        }
    }
    
    /* Check for extra arguments */
    if (optind + 2 < argc) {
        error_print("too many arguments");
        return -1;
    }
    
    return 0;
}

/*
 * NAME:    validate_options()
 * DESCRIPTION: Validate parsed options
 */
static int validate_options(mkfs_options_t *opts)
{
    /* Auto-detect filesystem type from program name if not specified */
    if (opts->filesystem_type == FS_TYPE_UNKNOWN) {
        opts->filesystem_type = common_get_fs_type_from_program(program_type);
        if (opts->filesystem_type == FS_TYPE_UNKNOWN) {
            opts->filesystem_type = FS_TYPE_HFS;  /* Default to HFS */
        }
    }
    
    /* Validate filesystem type matches program expectations */
    if (common_validate_fs_type(program_type, opts->filesystem_type) != 0) {
        const char *expected = (program_type == PROGRAM_MKFS_HFSPLUS) ? "HFS+" : "HFS";
        const char *actual = (opts->filesystem_type == FS_TYPE_HFSPLUS) ? "HFS+" : "HFS";
        error_print("filesystem type mismatch: %s expects %s, got %s", 
                   program_name, expected, actual);
        return -1;
    }
    
    /* Set default volume name if not specified */
    if (!opts->volume_name) {
        const char *default_name = (opts->filesystem_type == FS_TYPE_HFSPLUS) ? "Untitled" : "Untitled";
        opts->volume_name = strdup(default_name);
        if (!opts->volume_name) {
            error_print_errno("failed to allocate memory for default volume name");
            return -1;
        }
    }
    
    return 0;
}

/*
 * NAME:    cleanup_options()
 * DESCRIPTION: Free allocated memory in options
 */
static void cleanup_options(mkfs_options_t *opts)
{
    if (opts->device_path) {
        free(opts->device_path);
        opts->device_path = NULL;
    }
    
    if (opts->volume_name) {
        free(opts->volume_name);
        opts->volume_name = NULL;
    }
}

/*
 * NAME:    main()
 * DESCRIPTION: Main entry point for mkfs.hfs
 */
int main(int argc, char *argv[])
{
    mkfs_options_t opts = default_options;
    int result = EXIT_SUCCESS;
    
    /* Detect program type from program name */
    program_type = common_detect_program_type(argv[0]);
    program_name = (program_type == PROGRAM_MKFS_HFSPLUS) ? "mkfs.hfsplus" : "mkfs.hfs";
    
    /* Initialize common utilities */
    if (common_init(program_name, 0) != 0) {
        fprintf(stderr, "%s: failed to initialize\n", program_name);
        return EXIT_SYSTEM_ERROR;
    }
    
    /* Parse command-line arguments */
    if (parse_command_line(argc, argv, &opts) != 0) {
        cleanup_options(&opts);
        common_cleanup();
        return EXIT_USAGE_ERROR;
    }
    
    /* Handle special options */
    if (opts.show_version) {
        common_print_version(program_name);
        cleanup_options(&opts);
        common_cleanup();
        return EXIT_SUCCESS;
    }
    
    if (opts.show_help) {
        usage(EXIT_SUCCESS);
        /* usage() calls exit(), so this is never reached */
    }
    
    if (opts.show_license) {
        show_license();
        cleanup_options(&opts);
        common_cleanup();
        return EXIT_SUCCESS;
    }
    
    /* Enable verbose mode if requested */
    if (opts.verbose) {
        error_set_verbose(1);
    }
    
    /* Validate options */
    if (validate_options(&opts) != 0) {
        cleanup_options(&opts);
        common_cleanup();
        return EXIT_USAGE_ERROR;
    }
    
    /* Check if root privileges are required */
    common_check_root_required(opts.device_path, 1);  /* Write access required */
    
    /* Perform the formatting operation */
    error_verbose("formatting %s as %s filesystem", 
                 opts.device_path, 
                 (opts.filesystem_type == FS_TYPE_HFSPLUS) ? "HFS+" : "HFS");
    
    if (opts.filesystem_type == FS_TYPE_HFSPLUS) {
        result = mkfs_hfsplus_format(opts.device_path, &opts);
    } else {
        result = mkfs_hfs_format(opts.device_path, &opts);
    }
    
    if (result == 0) {
        error_verbose("formatting completed successfully");
    } else {
        error_print("formatting failed");
        result = error_get_exit_code(errno);
    }
    
    /* Cleanup and exit */
    cleanup_options(&opts);
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