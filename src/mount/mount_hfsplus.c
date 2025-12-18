/*
 * mount_hfsplus.c - HFS+ filesystem mount implementation
 * Copyright (C) 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "mount_common.h"

/* Verify device is HFS+ filesystem */
static int verify_hfsplus_filesystem(const char *device) {
    int fd;
    unsigned char sig[2];
    
    fd = open(device, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "mount.hfs+: cannot open %s: %s\n", device, strerror(errno));
        return -1;
    }
    
    /* Read signature at offset 1024 (Volume Header) */
    if (lseek(fd, 1024, SEEK_SET) < 0) {
        close(fd);
        return -1;
    }
    
    if (read(fd, sig, 2) != 2) {
        close(fd);
        return -1;
    }
    
    close(fd);
    
    /* Check HFS+ signature 0x482B ('H+') or 0x4858 ('HX') */
    if (!((sig[0] == 0x48 && sig[1] == 0x2B) || 
          (sig[0] == 0x48 && sig[1] == 0x58))) {
        fprintf(stderr, "mount.hfs+: %s is not a valid HFS+ filesystem\n", device);
        return -1;
    }
    
    return 0;
}

/* Verify mountpoint exists and is a directory */
static int verify_mountpoint(const char *mountpoint) {
    struct stat st;
    
    if (stat(mountpoint, &st) < 0) {
        fprintf(stderr, "mount.hfs+: mount point %s does not exist\n", mountpoint);
        return -1;
    }
    
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "mount.hfs+: %s is not a directory\n", mountpoint);
        return -1;
    }
    
    return 0;
}

int mount_hfsplus_volume(const char *device, const char *mountpoint,
                         const mount_options_t *opts) {
    unsigned long flags = 0;
    
    if (opts->verbose) {
        printf("mount.hfs+: mounting %s on %s\n", device, mountpoint);
    }
    
    /* Verify filesystem */
    if (verify_hfsplus_filesystem(device) < 0) {
        return MOUNT_FAILURE;
    }
    
    /* Verify mountpoint */
    if (verify_mountpoint(mountpoint) < 0) {
        return MOUNT_FAILURE;
    }
    
    /* Set mount flags */
    if (opts->read_only) {
        flags |= MS_RDONLY;
        if (opts->verbose) {
            printf("mount.hfs+: mounting read-only\n");
        }
    }
    
    /* Attempt mount */
    if (mount(device, mountpoint, "hfsplus", flags, NULL) < 0) {
        /* If mount failed, provide helpful error message */
        if (errno == EACCES || errno == EPERM) {
            fprintf(stderr, "mount.hfs+: permission denied\n");
            return MOUNT_USAGE_ERROR;
        } else if (errno == EBUSY) {
            fprintf(stderr, "mount.hfs+: %s is already mounted or %s is busy\n",
                    device, mountpoint);
            return MOUNT_FAILURE;
        } else if (errno == ENODEV) {
            fprintf(stderr, "mount.hfs+: filesystem type 'hfsplus' not supported by kernel\n");
            fprintf(stderr, "mount.hfs+: try loading the hfsplus kernel module: modprobe hfsplus\n");
            return MOUNT_FAILURE;
        } else {
            fprintf(stderr, "mount.hfs+: cannot mount %s on %s: %s\n",
                    device, mountpoint, strerror(errno));
            return MOUNT_FAILURE;
        }
    }
    
    if (opts->verbose) {
        printf("mount.hfs+: successfully mounted %s on %s\n", device, mountpoint);
    }
    
    return MOUNT_OK;
}
