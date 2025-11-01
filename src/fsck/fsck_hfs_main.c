/*
 * fsck_hfs_main.c - Main entry point for fsck.hfs standalone utility
 * Copyright (C) 2025 Pablo Lezaeta
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Based on hfsck from hfsutils by Robert Leslie
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <sys/stat.h>

#include "../embedded/fsck/fsck_hfs.h"
#include "fsck_common.h"

/* Program information */
static const char *program_name = "fsck.hfs";
static program_type_t program_type = PROGRAM_FSCK_HFS;

/* Command-line options are defined in fsck_common.h */

/* Default options */
static fsck_options_t default_options = {
    .device_path = NULL,
    .partition_number = -1,
    .repair = 0,
    .verbose = 0,
    .auto_repair = 0,
    .force = 0,
    .yes_to_all = 0,
    .read_only = 0,
    .show_version = 0,
    .show_help = 0,
    .show_license = 0
};

/*
 * NAME:    show_license()
 * DESCRIPTION: Display license information
 */
static void show_license(void)
{
    printf("%s - Check and repair HFS/HFS+ filesystems\n", program_name);
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
    const char *fs_type_str = (program_type == PROGRAM_FSCK_HFSPLUS) ? "HFS+" : "HFS/HFS+";
    
    printf("Usage: %s [options] device [partition-no]\n", program_name);
    printf("\n");
    printf("Check and repair %s filesystems.\n", fs_type_str);
    printf("\n");
    printf("Options:\n");
    printf("  -a, --auto        Automatically repair filesystem (preen mode)\n");
    printf("  -f, --force       Force checking even if filesystem seems clean\n");
    printf("  -n, --no-write    Check read-only, make no changes to filesystem\n");
    printf("  -p                Same as -a (for compatibility)\n");
    printf("  -r                Interactive repair (ask before fixing)\n");
    printf("  -v, --verbose     Verbose output\n");
    printf("  -y, --yes         Assume 'yes' to all questions\n");
    printf("  -V, --version     Display version information\n");
    printf("  -h, --help        Display this help message\n");
    printf("      --license     Display license information\n");
    printf("\n");
    printf("Arguments:\n");
    printf("  device            Block device or file to check\n");
    printf("  partition-no      Partition number (optional, 0 for whole device)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s /dev/sdb1                    # Check partition\n", program_name);
    printf("  %s -v /dev/sdb1                 # Check with verbose output\n", program_name);
    printf("  %s -a /dev/sdb1                 # Auto-repair if needed\n", program_name);
    printf("  %s -n /dev/sdb1                 # Read-only check\n", program_name);
    printf("  %s -f /dev/sdb 1                # Force check partition 1\n", program_name);
    printf("\n");
    printf("Program Name Detection:\n");
    printf("  fsck.hfs         Check HFS and HFS+ filesystems\n");
    printf("  fsck.hfs+        Same as fsck.hfs\n");
    printf("  fsck.hfsplus     Same as fsck.hfs\n");
    printf("\n");
    printf("Exit codes:\n");
    printf("  0   No errors found\n");
    printf("  1   Errors found and corrected\n");
    printf("  2   System should be rebooted\n");
    printf("  4   Errors found but not corrected\n");
    printf("  8   Operational error\n");
    printf("  16  Usage or syntax error\n");
    printf("  32  Checking cancelled by user\n");
    printf("  128 Shared library error\n");
    printf("\n");
    
    exit(exit_code);
}

/*
 * NAME:    parse_command_line()
 * DESCRIPTION: Parse command-line arguments
 */
static int parse_command_line(int argc, char *argv[], fsck_options_t *opts)
{
    int c;
    static struct option long_options[] = {
        {"auto",    no_argument,       0, 'a'},
        {"force",   no_argument,       0, 'f'},
        {"no-write", no_argument,      0, 'n'},
        {"verbose", no_argument,       0, 'v'},
        {"yes",     no_argument,       0, 'y'},
        {"version", no_argument,       0, 'V'},
        {"help",    no_argument,       0, 'h'},
        {"license", no_argument,       0, 1000},
        {0, 0, 0, 0}
    };
    
    while ((c = getopt_long(argc, argv, "afnprvyVh", long_options, NULL)) != -1) {
        switch (c) {
            case 'a':
            case 'p':
                opts->auto_repair = 1;
                opts->repair = 1;
                break;
                
            case 'f':
                opts->force = 1;
                break;
                
            case 'n':
                opts->read_only = 1;
                opts->repair = 0;
                break;
                
            case 'r':
                opts->repair = 1;
                opts->auto_repair = 0;
                break;
                
            case 'v':
                opts->verbose = 1;
                break;
                
            case 'y':
                opts->yes_to_all = 1;
                opts->repair = 1;
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
    } else {
        /* Default to partition 0 (whole device) if not specified */
        opts->partition_number = 0;
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
static int validate_options(fsck_options_t *opts)
{
    /* Conflicting options */
    if (opts->read_only && opts->repair) {
        error_print("cannot specify both read-only (-n) and repair options");
        return -1;
    }
    
    /* Auto-repair implies yes to all */
    if (opts->auto_repair) {
        opts->yes_to_all = 1;
    }
    
    return 0;
}

/*
 * NAME:    cleanup_options()
 * DESCRIPTION: Free allocated memory in options
 */
static void cleanup_options(fsck_options_t *opts)
{
    if (opts->device_path) {
        free(opts->device_path);
        opts->device_path = NULL;
    }
}

/*
 * NAME:    main()
 * DESCRIPTION: Main entry point for fsck.hfs
 */
int main(int argc, char *argv[])
{
    fsck_options_t opts = default_options;
    int result = FSCK_OK;
    int check_options = 0;
    
    /* Detect program type from program name */
    program_type = common_detect_program_type(argv[0]);
    program_name = (program_type == PROGRAM_FSCK_HFSPLUS) ? "fsck.hfsplus" : "fsck.hfs";
    
    /* Initialize common utilities */
    if (common_init(program_name, 0) != 0) {
        fprintf(stderr, "%s: failed to initialize\n", program_name);
        return FSCK_LIBRARY_ERROR;
    }
    
    /* Parse command-line arguments */
    if (parse_command_line(argc, argv, &opts) != 0) {
        cleanup_options(&opts);
        common_cleanup();
        return FSCK_USAGE_ERROR;
    }
    
    /* Handle special options */
    if (opts.show_version) {
        common_print_version(program_name);
        cleanup_options(&opts);
        common_cleanup();
        return FSCK_OK;
    }
    
    if (opts.show_help) {
        usage(FSCK_OK);
        /* usage() calls exit(), so this is never reached */
    }
    
    if (opts.show_license) {
        show_license();
        cleanup_options(&opts);
        common_cleanup();
        return FSCK_OK;
    }
    
    /* Enable verbose mode if requested */
    if (opts.verbose) {
        error_set_verbose(1);
    }
    
    /* Validate options */
    if (validate_options(&opts) != 0) {
        cleanup_options(&opts);
        common_cleanup();
        return FSCK_USAGE_ERROR;
    }
    
    /* Set up check options */
    if (opts.repair) {
        check_options |= HFSCK_REPAIR;
    }
    if (opts.verbose) {
        check_options |= HFSCK_VERBOSE;
    }
    if (opts.yes_to_all) {
        check_options |= HFSCK_YES;
    }
    
    /* Check if root privileges are required for repair */
    if (opts.repair) {
        common_check_root_required(opts.device_path, 1);  /* Write access required */
    } else {
        common_check_root_required(opts.device_path, 0);  /* Read-only access */
    }
    
    /* Perform the filesystem check */
    error_verbose("checking %s filesystem on %s", 
                 (program_type == PROGRAM_FSCK_HFSPLUS) ? "HFS+" : "HFS/HFS+",
                 opts.device_path);
    
    result = hfs_check_volume(opts.device_path, opts.partition_number, check_options);
    
    /* Report results */
    switch (result) {
        case FSCK_OK:
            error_verbose("filesystem check completed - no errors found");
            break;
        case FSCK_CORRECTED:
            error_verbose("filesystem check completed - errors found and corrected");
            break;
        case FSCK_UNCORRECTED:
            error_print("filesystem check completed - errors found but not corrected");
            break;
        default:
            error_print("filesystem check failed");
            break;
    }
    
    /* Cleanup and exit */
    cleanup_options(&opts);
    common_cleanup();
    
    return result;
}