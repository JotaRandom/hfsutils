/*
 * mkfs_common.c - Common functions shared between mkfs.hfs and mkfs.hfs+
 * Copyright (C) 2025 Pablo Lezaeta
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <sys/stat.h>

#include "../embedded/mkfs/mkfs_hfs.h"

/* Forward declarations */
long long parse_size_common(const char *size_str);
int validate_volume_name_hfs(const char *name);
int validate_volume_name_hfsplus(const char *name);

/*
 * NAME:    show_license()
 * DESCRIPTION: Display license information
 */
void mkfs_show_license(const char *program_name)
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
 * NAME:    parse_size()
 * DESCRIPTION: Parse size string (supports K, M, G suffixes)
 */
long long mkfs_parse_size(const char *size_str, int is_hfsplus)
{
    if (!size_str) {
        return -1;
    }
    
    char *endptr;
    long long size = strtoll(size_str, &endptr, 10);
    
    if (size <= 0) {
        return -1;
    }
    
    /* Handle size suffixes */
    if (*endptr != '\0') {
        switch (*endptr) {
            case 'k':
            case 'K':
                size *= 1024;
                break;
            case 'm':
            case 'M':
                size *= 1024 * 1024;
                break;
            case 'g':
            case 'G':
                size *= 1024 * 1024 * 1024;
                break;
            default:
                return -1;  /* Invalid suffix */
        }
        
        /* Check for extra characters after suffix */
        if (*(endptr + 1) != '\0') {
            return -1;
        }
    }
    
    /* Minimum size check */
    long long min_size = is_hfsplus ? (10 * 1024 * 1024) : (800 * 1024);
    if (size < min_size) {
        error_print("%s filesystem size must be at least %s", 
                   is_hfsplus ? "HFS+" : "HFS",
                   is_hfsplus ? "10MB" : "800KB");
        return -1;
    }
    
    return size;
}

/*
 * NAME:    validate_volume_name()
 * DESCRIPTION: Validate volume name according to HFS/HFS+ rules
 */
int mkfs_validate_volume_name(const char *name, int is_hfsplus)
{
    if (!name) {
        return 0;
    }
    
    size_t len = strlen(name);
    int max_len = is_hfsplus ? 255 : 27;
    
    if (len == 0 || len > max_len) {
        error_print("volume name must be 1-%d characters long", max_len);
        return -1;
    }
    
    /* Check for invalid characters (colon is not allowed) */
    for (size_t i = 0; i < len; i++) {
        if (name[i] == ':' || name[i] == '\0') {
            error_print("volume name cannot contain ':' or null characters");
            return -1;
        }
    }
    
    return 0;
}

/*
 * NAME:    parse_command_line()
 * DESCRIPTION: Parse command-line arguments (common for both HFS and HFS+)
 */
int mkfs_parse_command_line(int argc, char *argv[], mkfs_options_t *opts, int is_hfsplus)
{
    int c;
    static struct option long_options[] = {
        {"force",   no_argument,       0, 'f'},
        {"label",   required_argument, 0, 'L'},  /* Primary: -L per Unix convention */
        {"size",    required_argument, 0, 's'},
        {"verbose", no_argument,       0, 'v'},
        {"version", no_argument,       0, 'V'},
        {"help",    no_argument,       0, 'h'},
        {"license", no_argument,       0, 1000},
        {0, 0, 0, 0}
    };
    
    /* HFS+ supports -s option, HFS does not */
    /* Configure getopt_long options based on filesystem type */
    const char *optstring = is_hfsplus ? "fj:l:L:s:vVh" : "fl:L:vVh";
    
    while ((c = getopt_long(argc, argv, optstring, long_options, NULL)) != -1) {
        switch (c) {
            case 'f':
                opts->force = 1;
                break;
                
            case 'l':  /* Legacy/alternative */
            case 'L':  /* Unix standard */
                opts->volume_name = strdup(optarg);
                if (!opts->volume_name) {
                    error_print_errno("failed to allocate memory for volume name");
                    return -1;
                }
                if (is_hfsplus) {
                    if (validate_volume_name_hfsplus(opts->volume_name) != 0) {
                        return -1;
                    }
                } else {
                    if (validate_volume_name_hfs(opts->volume_name) != 0) {
                        return -1;
                    }
                }
                break;
                
            case 'j':
                /* Journaling option only for HFS+ */
                if (!is_hfsplus) {
                    error_print("-j option is only supported for HFS+");
                    return -1;
                }
                opts->enable_journaling = 1;
                
                /* WARNING: Linux HFS+ driver does NOT support journaling */
                fprintf(stderr, "\\n");
                fprintf(stderr, "WARNING: HFS+ journaling enabled\\n");
                fprintf(stderr, "=========================================\\n");
                fprintf(stderr, "The Linux HFS+ kernel driver does NOT support journaling.\\n");
                fprintf(stderr, "Journaled volumes will:  \\n");
                fprintf(stderr, "  - Mount as NO_JOURNAL on Linux\\n");
                fprintf(stderr, "  - Work correctly on macOS/Darwin\\n");
                fprintf(stderr, "  - Require fsck on Linux if unclean unmount\\n");
                fprintf(stderr, "\\n");
                fprintf(stderr, "For Linux-only use, journaling is NOT recommended.\\n");
                fprintf(stderr, "=========================================\\n");
                fprintf(stderr, "\\n");
                break;
                
            case 's':
                /* Size option only for HFS+ */
                if (!is_hfsplus) {
                    error_print("-s option is only supported for HFS+");
                    return -1;
                }
                opts->total_size = mkfs_parse_size(optarg, is_hfsplus);
                if (opts->total_size < 0) {
                    error_print("invalid size specification: %s", optarg);
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
int mkfs_validate_options(mkfs_options_t *opts, int is_hfsplus)
{
    /* Auto-detect filesystem type from program name if not specified */
    if (opts->filesystem_type == FS_TYPE_UNKNOWN) {
        opts->filesystem_type = is_hfsplus ? FS_TYPE_HFSPLUS : FS_TYPE_HFS;
    }
    
    /* Set default volume name if not specified */
    if (!opts->volume_name) {
        opts->volume_name = strdup("Untitled");
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
void mkfs_cleanup_options(mkfs_options_t *opts)
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
*
 * NAME:    parse_size_common()
 * DESCRIPTION: Parse size string with K/M/G suffixes (common for both programs)
 */
long long parse_size_common(const char *size_str)
{
    if (!size_str) {
        return -1;
    }
    
    char *endptr;
    long long size = strtoll(size_str, &endptr, 10);
    
    if (size <= 0) {
        return -1;
    }
    
    /* Handle size suffixes */
    if (*endptr != '\0') {
        switch (*endptr) {
            case 'k':
            case 'K':
                size *= 1024;
                break;
            case 'm':
            case 'M':
                size *= 1024 * 1024;
                break;
            case 'g':
            case 'G':
                size *= 1024 * 1024 * 1024;
                break;
            default:
                return -1;  /* Invalid suffix */
        }
        
        /* Check for extra characters after suffix */
        if (*(endptr + 1) != '\0') {
            return -1;
        }
    }
    
    return size;
}

/*
 * NAME:    validate_volume_name_hfs()
 * DESCRIPTION: Validate volume name according to HFS rules
 */
int validate_volume_name_hfs(const char *name)
{
    if (!name) {
        return 0;
    }
    
    size_t len = strlen(name);
    
    /* HFS volume names are limited to 27 characters */
    if (len == 0 || len > 27) {
        error_print("HFS volume name must be 1-27 characters long");
        return -1;
    }
    
    /* Check for invalid characters (colon is not allowed) */
    for (size_t i = 0; i < len; i++) {
        if (name[i] == ':' || name[i] == '\0') {
            error_print("volume name cannot contain ':' or null characters");
            return -1;
        }
    }
    
    return 0;
}

/*
 * NAME:    validate_volume_name_hfsplus()
 * DESCRIPTION: Validate volume name according to HFS+ rules
 */
int validate_volume_name_hfsplus(const char *name)
{
    if (!name) {
        return 0;
    }
    
    size_t len = strlen(name);
    
    /* HFS+ volume names are limited to 255 characters */
    if (len == 0 || len > 255) {
        error_print("HFS+ volume name must be 1-255 characters long");
        return -1;
    }
    
    /* Check for invalid characters (colon is not allowed) */
    for (size_t i = 0; i < len; i++) {
        if (name[i] == ':' || name[i] == '\0') {
            error_print("volume name cannot contain ':' or null characters");
            return -1;
        }
    }
    
    return 0;
}

/*
 * NAME:    show_license_common()
 * DESCRIPTION: Display license information (common for both programs)
 */
void show_license_common(const char *program_name)
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
 * NAME:    validate_options_common()
 * DESCRIPTION: Validate parsed options (common for both programs)
 */
int validate_options_common(mkfs_options_t *opts)
{
    /* Set default volume name if not specified */
    if (!opts->volume_name) {
        opts->volume_name = strdup("Untitled");
        if (!opts->volume_name) {
            error_print_errno("failed to allocate memory for default volume name");
            return -1;
        }
    }
    
    return 0;
}

/*
 * NAME:    cleanup_options_common()
 * DESCRIPTION: Free allocated memory in options (common for both programs)
 */
void cleanup_options_common(mkfs_options_t *opts)
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