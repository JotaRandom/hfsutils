/*
 * fsck_common.c - Common functions shared between fsck.hfs and fsck.hfs+
 * Copyright (C) 2025 Pablo Lezaeta
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

/*
 * NAME:    show_license()
 * DESCRIPTION: Display license information
 */
void fsck_show_license(const char *program_name)
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
 * NAME:    parse_command_line()
 * DESCRIPTION: Parse command-line arguments (common for both HFS and HFS+)
 *              Supports all original hfsck options plus standard fsck options
 */
int fsck_parse_command_line(int argc, char *argv[], fsck_options_t *opts)
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
    
    /* Initialize default options - repair is enabled by default like original hfsck */
    opts->repair = 1;
    
    while ((c = getopt_long(argc, argv, "afnprvyVh", long_options, NULL)) != -1) {
        switch (c) {
            case 'a':
            case 'p':
                /* Auto repair mode - repair without prompting (same as -y) */
                opts->auto_repair = 1;
                opts->repair = 1;
                opts->yes_to_all = 1;  /* Auto mode implies yes to all */
                break;
                
            case 'f':
                /* Force checking even if filesystem appears clean */
                opts->force = 1;
                break;
                
            case 'n':
                /* No-write mode - check only, don't repair */
                opts->read_only = 1;
                opts->repair = 0;
                break;
                
            case 'r':
                /* Interactive repair mode (opposite of auto) */
                opts->repair = 1;
                opts->auto_repair = 0;
                opts->yes_to_all = 0;
                break;
                
            case 'v':
                /* Verbose mode - display detailed information */
                opts->verbose = 1;
                break;
                
            case 'y':
                /* Yes mode - assume 'yes' to all questions (same as -a) */
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
int fsck_validate_options(fsck_options_t *opts)
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
void fsck_cleanup_options(fsck_options_t *opts)
{
    if (opts->device_path) {
        free(opts->device_path);
        opts->device_path = NULL;
    }
}