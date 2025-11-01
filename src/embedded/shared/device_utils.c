/*
 * device_utils.c - Device detection and partitioning utilities
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
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>

#include "device_utils.h"
#include "suid.h"

/*
 * NAME:    device_validate()
 * DESCRIPTION: Validate device path and check accessibility
 */
int device_validate(const char *path, int flags)
{
    struct stat st;
    int fd;
    
    if (!path) {
        errno = EINVAL;
        return -1;
    }
    
    /* Check if path exists */
    if (stat(path, &st) == -1) {
        return -1;
    }
    
    /* Check if it's a block device or regular file */
    if (!S_ISBLK(st.st_mode) && !S_ISREG(st.st_mode)) {
        errno = ENOTBLK;
        return -1;
    }
    
    /* Try to open with requested flags */
    suid_enable();
    fd = open(path, flags);
    suid_disable();
    
    if (fd == -1) {
        return -1;
    }
    
    close(fd);
    return 0;
}

/*
 * NAME:    device_get_size()
 * DESCRIPTION: Get device size in bytes
 */
off_t device_get_size(const char *path)
{
    struct stat st;
    int fd;
    off_t size = -1;
    
    if (!path) {
        errno = EINVAL;
        return -1;
    }
    
    if (stat(path, &st) == -1) {
        return -1;
    }
    
    if (S_ISREG(st.st_mode)) {
        return st.st_size;
    }
    
    if (S_ISBLK(st.st_mode)) {
        suid_enable();
        fd = open(path, O_RDONLY);
        suid_disable();
        
        if (fd == -1) {
            return -1;
        }
        
        /* Try to seek to end to get size */
        size = lseek(fd, 0, SEEK_END);
        close(fd);
        
        if (size == -1) {
            /* If seek fails, try ioctl methods on Linux */
#ifdef __linux__
            #include <sys/ioctl.h>
            #include <linux/fs.h>
            
            suid_enable();
            fd = open(path, O_RDONLY);
            suid_disable();
            
            if (fd != -1) {
                unsigned long long bytes;
                if (ioctl(fd, BLKGETSIZE64, &bytes) == 0) {
                    size = (off_t)bytes;
                }
                close(fd);
            }
#endif
        }
    }
    
    return size;
}

/*
 * NAME:    device_is_mounted()
 * DESCRIPTION: Check if device is currently mounted
 */
int device_is_mounted(const char *path)
{
    FILE *mounts;
    char line[1024];
    char *device, *mountpoint __attribute__((unused)) = NULL;
    int mounted = 0;
    
    if (!path) {
        errno = EINVAL;
        return -1;
    }
    
    mounts = fopen("/proc/mounts", "r");
    if (!mounts) {
        /* Try /etc/mtab as fallback */
        mounts = fopen("/etc/mtab", "r");
        if (!mounts) {
            return -1;
        }
    }
    
    while (fgets(line, sizeof(line), mounts)) {
        device = strtok(line, " \t");
        mountpoint = strtok(NULL, " \t");
        
        if (device && strcmp(device, path) == 0) {
            mounted = 1;
            break;
        }
    }
    
    fclose(mounts);
    return mounted;
}

/*
 * NAME:    partition_detect_type()
 * DESCRIPTION: Detect partition table type
 */
partition_type_t partition_detect_type(const char *path)
{
    int fd;
    unsigned char buf[512];
    partition_type_t type = PARTITION_UNKNOWN;
    
    if (!path) {
        errno = EINVAL;
        return PARTITION_UNKNOWN;
    }
    
    suid_enable();
    fd = open(path, O_RDONLY);
    suid_disable();
    
    if (fd == -1) {
        return PARTITION_UNKNOWN;
    }
    
    /* Read first sector */
    if (read(fd, buf, sizeof(buf)) != sizeof(buf)) {
        close(fd);
        return PARTITION_UNKNOWN;
    }
    
    /* Check for Apple Partition Map */
    if (buf[0] == 0x45 && buf[1] == 0x52) {  /* "ER" */
        type = PARTITION_APPLE;
    }
    /* Check for MBR */
    else if (buf[510] == 0x55 && buf[511] == 0xAA) {
        type = PARTITION_MBR;
    }
    /* Check for GPT */
    else {
        /* GPT signature is at offset 512 */
        if (lseek(fd, 512, SEEK_SET) == 512) {
            if (read(fd, buf, 8) == 8) {
                if (memcmp(buf, "EFI PART", 8) == 0) {
                    type = PARTITION_GPT;
                }
            }
        }
    }
    
    close(fd);
    return type;
}

/*
 * NAME:    partition_count()
 * DESCRIPTION: Count partitions on device
 */
int partition_count(const char *path)
{
    partition_type_t type;
    int count = 0;
    
    if (!path) {
        errno = EINVAL;
        return -1;
    }
    
    type = partition_detect_type(path);
    
    switch (type) {
        case PARTITION_APPLE:
            count = partition_count_apple(path);
            break;
        case PARTITION_MBR:
            count = partition_count_mbr(path);
            break;
        case PARTITION_GPT:
            count = partition_count_gpt(path);
            break;
        case PARTITION_UNKNOWN:
        default:
            count = 0;
            break;
    }
    
    return count;
}

/*
 * NAME:    partition_count_apple()
 * DESCRIPTION: Count Apple Partition Map entries
 */
int partition_count_apple(const char *path)
{
    int fd, count = 0;
    unsigned char buf[512];
    uint32_t map_entries;
    
    suid_enable();
    fd = open(path, O_RDONLY);
    suid_disable();
    
    if (fd == -1) {
        return -1;
    }
    
    /* Skip driver descriptor record, read partition map */
    if (lseek(fd, 512, SEEK_SET) != 512) {
        close(fd);
        return -1;
    }
    
    if (read(fd, buf, sizeof(buf)) != sizeof(buf)) {
        close(fd);
        return -1;
    }
    
    /* Check partition map signature */
    if (buf[0] != 0x50 || buf[1] != 0x4D) {  /* "PM" */
        close(fd);
        return 0;
    }
    
    /* Get number of partition map entries */
    map_entries = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
    count = (int)map_entries;
    
    close(fd);
    return count;
}

/*
 * NAME:    partition_count_mbr()
 * DESCRIPTION: Count MBR partition entries
 */
int partition_count_mbr(const char *path)
{
    int fd, count = 0;
    unsigned char buf[512];
    int i;
    
    suid_enable();
    fd = open(path, O_RDONLY);
    suid_disable();
    
    if (fd == -1) {
        return -1;
    }
    
    if (read(fd, buf, sizeof(buf)) != sizeof(buf)) {
        close(fd);
        return -1;
    }
    
    /* Check MBR signature */
    if (buf[510] != 0x55 || buf[511] != 0xAA) {
        close(fd);
        return 0;
    }
    
    /* Count non-empty partition entries */
    for (i = 0; i < 4; i++) {
        int offset = 446 + (i * 16);
        if (buf[offset + 4] != 0) {  /* Partition type */
            count++;
        }
    }
    
    close(fd);
    return count;
}

/*
 * NAME:    partition_count_gpt()
 * DESCRIPTION: Count GPT partition entries
 */
int partition_count_gpt(const char *path)
{
    int fd, count = 0;
    unsigned char buf[512];
    uint32_t num_entries;
    
    suid_enable();
    fd = open(path, O_RDONLY);
    suid_disable();
    
    if (fd == -1) {
        return -1;
    }
    
    /* Read GPT header at sector 1 */
    if (lseek(fd, 512, SEEK_SET) != 512) {
        close(fd);
        return -1;
    }
    
    if (read(fd, buf, sizeof(buf)) != sizeof(buf)) {
        close(fd);
        return -1;
    }
    
    /* Check GPT signature */
    if (memcmp(buf, "EFI PART", 8) != 0) {
        close(fd);
        return 0;
    }
    
    /* Get number of partition entries */
    num_entries = buf[80] | (buf[81] << 8) | (buf[82] << 16) | (buf[83] << 24);
    count = (int)num_entries;
    
    close(fd);
    return count;
}