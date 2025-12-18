/*
 * mount_main.c - Main entry point for mount.hfs/mount.hfs+
 * Copyright (C) 2025
 *
 * Standard Unix mount utility for HFS and HFS+ filesystems
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <getopt.h>
#include "mount_common.h"

static void show_usage(const char *progname, fs_type_t fs_type) {
    const char *fs_name = (fs_type == FS_TYPE_HFS) ? "HFS" : "HFS+";
    
    printf("Usage: %s [options] device mountpoint\n", progname);
    printf("\n");
    printf("Mount %s filesystem\n", fs_name);
    printf("\n");
    printf("Options:\n");
    printf("  -o options       Mount options (ro, rw, sync, async, etc.)\n");
    printf("  -r               Mount read-only (shorthand for -o ro)\n");
    printf("  -w               Mount read-write (shorthand for -o rw)\n");
    printf("  -v               Verbose output\n");
    printf("  -h, --help       Display this help message\n");
    printf("  -V, --version    Display version information\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s /dev/sdb1 /mnt/hfs\n", progname);
    printf("  %s -r /dev/sdb1 /mnt/hfs\n", progname);
    printf("  %s -o ro,sync /dev/sdb1 /mnt/hfs\n", progname);
    printf("\n");
}

static void show_version(void) {
    printf("mount.hfs version 3.2.6\n");
    printf("HFS/HFS+ filesystem mount utility\n");
}

fs_type_t detect_program_type(const char *progname) {
    char *base = basename((char *)progname);
    
    if (strstr(base, "hfs+") || strstr(base, "hfsplus")) {
        return FS_TYPE_HFSPLUS;
    }
    
    return FS_TYPE_HFS;
}

int mount_parse_options(int argc, char **argv, mount_options_t *opts) {
    int c;
    struct option long_options[] = {
        {"help",    no_argument, 0, 'h'},
        {"version", no_argument, 0, 'V'},
        {0, 0, 0, 0}
    };
    
    /* Initialize options */
    memset(opts, 0, sizeof(mount_options_t));
    opts->read_write = 1;  /* Default to rw */
    
    while ((c = getopt_long(argc, argv, "o:rwvhV", long_options, NULL)) != -1) {
        switch (c) {
            case 'o':
                opts->options = strdup(optarg);
                /* Parse common options */
                if (strstr(optarg, "ro")) {
                    opts->read_only = 1;
                    opts->read_write = 0;
                }
                if (strstr(optarg, "rw")) {
                    opts->read_write = 1;
                    opts->read_only = 0;
                }
                break;
                
            case 'r':
                opts->read_only = 1;
                opts->read_write = 0;
                break;
                
            case 'w':
                opts->read_write = 1;
                opts->read_only = 0;
                break;
                
            case 'v':
                opts->verbose = 1;
                break;
                
            case 'h':
                opts->show_help = 1;
                return 0;
                
            case 'V':
                opts->show_version = 1;
                return 0;
                
            case '?':
                return -1;
                
            default:
                fprintf(stderr, "Unknown option\n");
                return -1;
        }
    }
    
    /* Get device and mountpoint */
    if (optind + 2 > argc) {
        fprintf(stderr, "Error: device and mountpoint required\n");
        return -1;
    }
    
    opts->device = argv[optind];
    opts->mountpoint = argv[optind + 1];
    
    return 0;
}

int main(int argc, char **argv) {
    mount_options_t opts;
    fs_type_t fs_type;
    int ret;
    
    /* Detect filesystem type from program name */
    fs_type = detect_program_type(argv[0]);
    
    /* Parse command-line options */
    if (mount_parse_options(argc, argv, &opts) < 0) {
        show_usage(argv[0], fs_type);
        return MOUNT_USAGE_ERROR;
    }
    
    /* Handle --help */
    if (opts.show_help) {
        show_usage(argv[0], fs_type);
        return MOUNT_OK;
    }
    
    /* Handle --version */
    if (opts.show_version) {
        show_version();
        return MOUNT_OK;
    }
    
    /* Perform mount */
    if (fs_type == FS_TYPE_HFS) {
        ret = mount_hfs_volume(opts.device, opts.mountpoint, &opts);
    } else {
        ret = mount_hfsplus_volume(opts.device, opts.mountpoint, &opts);
    }
    
    /* Cleanup */
    if (opts.options) {
        free(opts.options);
    }
    
    return ret;
}
