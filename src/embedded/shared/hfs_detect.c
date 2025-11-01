/*
 * hfs_detect.c - HFS/HFS+ filesystem detection and utilities
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
#include <time.h>
#include <sys/stat.h>

#include "../../include/common/hfs_detect.h"

/*
 * NAME:    hfs_detect_fs_type()
 * DESCRIPTION: Detect filesystem type by reading signature
 */
hfs_fs_type_t hfs_detect_fs_type(int fd)
{
    uint16_t signature;
    
    if (lseek(fd, HFS_SUPERBLOCK_OFFSET, SEEK_SET) == -1) {
        return FS_TYPE_UNKNOWN;
    }
    
    if (read(fd, &signature, sizeof(signature)) != sizeof(signature)) {
        return FS_TYPE_UNKNOWN;
    }
    
    signature = HFS_BE16(signature);
    
    switch (signature) {
        case HFS_SIGNATURE:
            return FS_TYPE_HFS;
        case HFSPLUS_SIGNATURE:
            return FS_TYPE_HFSPLUS;
        case HFSX_SIGNATURE:
            return FS_TYPE_HFSX;
        default:
            return FS_TYPE_UNKNOWN;
    }
}

/*
 * NAME:    hfs_read_volume_info()
 * DESCRIPTION: Read volume information for HFS or HFS+
 */
int hfs_read_volume_info(int fd, hfs_volume_info_t *vol_info)
{
    if (!vol_info) {
        errno = EINVAL;
        return -1;
    }
    
    memset(vol_info, 0, sizeof(*vol_info));
    vol_info->fd = fd;
    vol_info->fs_type = hfs_detect_fs_type(fd);
    
    if (vol_info->fs_type == FS_TYPE_UNKNOWN) {
        errno = EINVAL;
        return -1;
    }
    
    /* Seek to superblock */
    if (lseek(fd, HFS_SUPERBLOCK_OFFSET, SEEK_SET) == -1) {
        return -1;
    }
    
    if (vol_info->fs_type == FS_TYPE_HFS) {
        /* Read HFS MDB */
        if (read(fd, &vol_info->sb.hfs, sizeof(vol_info->sb.hfs)) != sizeof(vol_info->sb.hfs)) {
            return -1;
        }
        
        /* Convert endianness and extract common fields */
        vol_info->block_size = HFS_BE32(vol_info->sb.hfs.drAlBlkSiz);
        vol_info->total_blocks = HFS_BE16(vol_info->sb.hfs.drNmAlBlks);
        vol_info->free_blocks = HFS_BE16(vol_info->sb.hfs.drFreeBks);
        vol_info->create_date = HFS_BE32(vol_info->sb.hfs.drCrDate) - HFS_EPOCH_OFFSET;
        vol_info->modify_date = HFS_BE32(vol_info->sb.hfs.drLsMod) - HFS_EPOCH_OFFSET;
        
        /* Copy volume name (HFS uses Pascal string format) */
        int name_len = vol_info->sb.hfs.drVN[0];
        if (name_len > 27) name_len = 27;
        memcpy(vol_info->volume_name, &vol_info->sb.hfs.drVN[1], name_len);
        vol_info->volume_name[name_len] = '\0';
        
    } else if (vol_info->fs_type == FS_TYPE_HFSPLUS || vol_info->fs_type == FS_TYPE_HFSX) {
        /* Read HFS+ Volume Header */
        if (read(fd, &vol_info->sb.hfsplus, sizeof(vol_info->sb.hfsplus)) != sizeof(vol_info->sb.hfsplus)) {
            return -1;
        }
        
        /* Convert endianness and extract common fields */
        vol_info->block_size = HFS_BE32(vol_info->sb.hfsplus.blockSize);
        vol_info->total_blocks = HFS_BE32(vol_info->sb.hfsplus.totalBlocks);
        vol_info->free_blocks = HFS_BE32(vol_info->sb.hfsplus.freeBlocks);
        vol_info->create_date = HFS_BE32(vol_info->sb.hfsplus.createDate) - HFS_EPOCH_OFFSET;
        vol_info->modify_date = HFS_BE32(vol_info->sb.hfsplus.modifyDate) - HFS_EPOCH_OFFSET;
        
        /* For HFS+, volume name is stored in the catalog file root directory */
        strcpy(vol_info->volume_name, "Untitled");  /* Default name */
    }
    
    return 0;
}

/*
 * NAME:    hfs_validate_dates()
 * DESCRIPTION: Validate HFS date field
 */
int hfs_validate_dates(time_t date, const char *field_name)
{
    time_t hfs_date = date + HFS_EPOCH_OFFSET;
    
    if (hfs_date > HFS_MAX_TIME) {
        fprintf(stderr, "Warning: %s exceeds HFS date limit (Feb 6, 2040)\n", field_name);
        return -1;
    }
    
    return 0;
}

/*
 * NAME:    hfs_get_safe_time()
 * DESCRIPTION: Get current time, adjusted if necessary for HFS limits
 */
time_t hfs_get_safe_time(void)
{
    time_t now = time(NULL);
    time_t hfs_time = now + HFS_EPOCH_OFFSET;
    
    if (hfs_time > HFS_MAX_TIME) {
        /* Use January 1, 2030 as safe fallback */
        return HFS_MAX_TIME - 315360000 - HFS_EPOCH_OFFSET;
    }
    
    return now;
}

/*
 * NAME:    hfs_log_date_adjustment()
 * DESCRIPTION: Log date adjustments to hfsutils.log
 */
void hfs_log_date_adjustment(const char *path, time_t original, time_t adjusted)
{
    int log_fd = open("hfsutils.log", O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (log_fd < 0) return;
    
    char buf[512];
    snprintf(buf, sizeof(buf), 
             "Date adjustment: %s - Original: %ld, Adjusted: %ld\n",
             path ? path : "unknown", original, adjusted);
    
    if (write(log_fd, buf, strlen(buf)) == -1) {
        /* Ignore write errors for logging */
    }
    close(log_fd);
}/*

 * NAME:    hfs_detect_filesystem_type()
 * DESCRIPTION: Detect filesystem type on a device with partition support
 */
hfs_fs_type_t hfs_detect_filesystem_type(const char *device_path, int partition_number)
{
    int fd;
    hfs_fs_type_t fs_type;
    off_t offset = 0;
    
    fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        return FS_TYPE_UNKNOWN;
    }
    
    /* If partition number is specified, calculate offset */
    if (partition_number > 0) {
        /* This is a simplified partition handling - in a real implementation,
         * we would read the partition table to find the correct offset */
        offset = partition_number * 512 * 1024; /* Simplified offset calculation */
        if (lseek(fd, offset, SEEK_SET) == -1) {
            close(fd);
            return FS_TYPE_UNKNOWN;
        }
    }
    
    fs_type = hfs_detect_fs_type(fd);
    close(fd);
    
    return fs_type;
}

/*
 * NAME:    hfs_get_fs_type_name()
 * DESCRIPTION: Get string name for filesystem type
 */
const char *hfs_get_fs_type_name(hfs_fs_type_t fs_type)
{
    switch (fs_type) {
        case FS_TYPE_HFS:
            return "HFS";
        case FS_TYPE_HFSPLUS:
            return "HFS+";
        case FS_TYPE_HFSX:
            return "HFSX";
        default:
            return "Unknown";
    }
}