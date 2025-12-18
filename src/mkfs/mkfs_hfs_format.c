/*
 * mkfs_hfs_format.c - HFS/HFS+ formatting functions for mkfs.hfs
 * Copyright (C) 2025 Pablo Lezaeta
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Based on hformat from hfsutils by Robert Leslie
 * Uses HFS specification from Inside Macintosh: Files (1992)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

#include "../embedded/mkfs/mkfs_hfs.h"

/* Volume parameters structure for HFS */
typedef struct {
    off_t device_size;
    uint32_t sector_size;
    uint32_t total_sectors;
    uint32_t allocation_block_size;
    uint16_t total_allocation_blocks;
    uint16_t free_allocation_blocks;
    uint32_t catalog_file_size;
    uint32_t extents_file_size;
    time_t creation_date;
} volume_params_t;

/* Volume parameters structure for HFS+ */
typedef struct {
    off_t device_size;
    uint32_t sector_size;
    uint32_t total_sectors;
    uint32_t block_size;           /* HFS+ uses larger blocks */
    uint32_t total_blocks;         /* 32-bit for HFS+ */
    uint32_t free_blocks;          /* 32-bit for HFS+ */
    uint32_t allocation_file_size;
    uint32_t extents_file_size;
    uint32_t catalog_file_size;
    uint32_t attributes_file_size;
    uint32_t startup_file_size;
    time_t creation_date;
    int enable_journaling;         /* Future: journaling support */
    int case_sensitive;            /* Future: case sensitivity */
} hfsplus_volume_params_t;

/* Forward declarations - HFS */
static int validate_device(const char *device_path, int force);
static int calculate_volume_parameters(const char *device_path, mkfs_options_t *opts, 
                                     volume_params_t *params);
static int format_hfs_volume(const char *device_path, int partno, 
                            const mkfs_options_t *opts, const volume_params_t *params);
static int verify_hfs_volume(const char *device_path, int partno, const mkfs_options_t *opts);
static int write_boot_blocks(int fd, const volume_params_t *params);
static int write_master_directory_block(int fd, const volume_params_t *params, 
                                       const mkfs_options_t *opts);
static int write_volume_bitmap(int fd, const volume_params_t *params);
static int initialize_catalog_file(int fd, const volume_params_t *params);
static int initialize_extents_file(int fd, const volume_params_t *params);
static int validate_volume_name(const char *vname);

/* Forward declarations - HFS+ */
static int calculate_hfsplus_volume_parameters(const char *device_path, mkfs_options_t *opts,
                                             hfsplus_volume_params_t *params);
static int format_hfsplus_volume(const char *device_path, int partno,
                                const mkfs_options_t *opts, const hfsplus_volume_params_t *params);
static int verify_hfsplus_volume(const char *device_path, int partno, const mkfs_options_t *opts);
static int write_hfsplus_boot_blocks(int fd, const hfsplus_volume_params_t *params);
static int write_hfsplus_volume_header(int fd, const hfsplus_volume_params_t *params,
                                     const mkfs_options_t *opts);
static int write_hfsplus_allocation_bitmap(int fd, const hfsplus_volume_params_t *params);
static int initialize_hfsplus_catalog_file(int fd, const hfsplus_volume_params_t *params);
static int initialize_hfsplus_extents_file(int fd, const hfsplus_volume_params_t *params);

/*
 * NAME:    mkfs_hfs_format()
 * DESCRIPTION: Format device as HFS filesystem using libhfs logic
 */
int mkfs_hfs_format(const char *device_path, const mkfs_options_t *opts)
{
    int result = -1;
    volume_params_t params;
    char *resolved_path = NULL;
    int nparts;
    
    error_verbose("starting HFS formatting of %s", device_path);
    
    /* Resolve device path */
    resolved_path = common_resolve_device_path(device_path);
    if (!resolved_path) {
        error_print_errno("failed to resolve device path %s", device_path);
        return -1;
    }
    
    /* Validate device and options */
    if (validate_device(resolved_path, opts->force) != 0) {
        goto cleanup;
    }
    
    /* Check partition information */
    suid_enable();
    nparts = hfs_nparts(resolved_path);
    suid_disable();
    
    if (nparts >= 0) {
        error_verbose("%s: contains %d HFS partition%s", 
                     resolved_path, nparts, nparts == 1 ? "" : "s");
    }
    
    /* Validate partition number */
    int partno = opts->partition_number;
    if (partno == -1) {
        if (nparts > 1) {
            error_print("must specify partition number (%d available)", nparts);
            goto cleanup;
        } else if (nparts == -1) {
            partno = 0;  /* Whole device */
        } else {
            partno = 1;  /* Single partition */
        }
    }
    
    /* Check for partition conflicts */
    if (nparts != -1 && partno == 0) {
        if (opts->force) {
            error_warning("erasing partition information");
        } else {
            error_print("medium is partitioned; select partition > 0 or use -f");
            goto cleanup;
        }
    }
    
    /* Calculate volume parameters */
    if (calculate_volume_parameters(resolved_path, (mkfs_options_t *)opts, &params) != 0) {
        error_print("failed to calculate volume parameters");
        goto cleanup;
    }
    
    error_verbose("volume parameters calculated:");
    error_verbose("  device size: %lld bytes", (long long)params.device_size);
    error_verbose("  allocation block size: %u bytes", params.allocation_block_size);
    error_verbose("  total allocation blocks: %u", params.total_allocation_blocks);
    error_verbose("  free allocation blocks: %u", params.free_allocation_blocks);
    
    /* Perform the actual formatting using integrated libhfs logic */
    if (format_hfs_volume(resolved_path, partno, opts, &params) != 0) {
        error_print("HFS formatting failed");
        goto cleanup;
    }
    
    /* Verify the created filesystem */
    if (verify_hfs_volume(resolved_path, partno, opts) != 0) {
        error_warning("filesystem verification failed, but volume may still be usable");
    }
    
    error_verbose("HFS formatting completed successfully");
    printf("HFS volume '%s' created successfully\n", opts->volume_name);
    printf("Filesystem type: HFS\n");
    printf("Volume size: %lld bytes (%u allocation blocks)\n", 
           (long long)params.device_size, params.total_allocation_blocks);
    printf("Allocation block size: %u bytes\n", params.allocation_block_size);
    
    result = 0;
    
cleanup:
    if (resolved_path) {
        free(resolved_path);
    }
    
    return result;
}

/*
 * NAME:    mkfs_hfsplus_format()
 * DESCRIPTION: Format device as HFS+ filesystem
 */
int mkfs_hfsplus_format(const char *device_path, const mkfs_options_t *opts)
{
    int result = -1;
    hfsplus_volume_params_t params;
    char *resolved_path = NULL;
    int nparts;
    
    error_verbose("starting HFS+ formatting of %s", device_path);
    
    /* Resolve device path */
    resolved_path = common_resolve_device_path(device_path);
    if (!resolved_path) {
        error_print_errno("failed to resolve device path %s", device_path);
        return -1;
    }
    
    /* Validate device and options */
    if (validate_device(resolved_path, opts->force) != 0) {
        goto cleanup;
    }
    
    /* Check partition information */
    suid_enable();
    nparts = hfs_nparts(resolved_path);
    suid_disable();
    
    if (nparts >= 0) {
        error_verbose("%s: contains %d HFS partition%s", 
                     resolved_path, nparts, nparts == 1 ? "" : "s");
    }
    
    /* Validate partition number */
    int partno = opts->partition_number;
    if (partno == -1) {
        if (nparts > 1) {
            error_print("must specify partition number (%d available)", nparts);
            goto cleanup;
        } else if (nparts == -1) {
            partno = 0;  /* Whole device */
        } else {
            partno = 1;  /* Single partition */
        }
    }
    
    /* Check for partition conflicts */
    if (nparts != -1 && partno == 0) {
        if (opts->force) {
            error_warning("erasing partition information");
        } else {
            error_print("medium is partitioned; select partition > 0 or use -f");
            goto cleanup;
        }
    }
    
    /* Calculate HFS+ volume parameters */
    if (calculate_hfsplus_volume_parameters(resolved_path, (mkfs_options_t *)opts, &params) != 0) {
        error_print("failed to calculate HFS+ volume parameters");
        goto cleanup;
    }
    
    error_verbose("HFS+ volume parameters calculated:");
    error_verbose("  device size: %lld bytes", (long long)params.device_size);
    error_verbose("  block size: %u bytes", params.block_size);
    error_verbose("  total blocks: %u", params.total_blocks);
    error_verbose("  free blocks: %u", params.free_blocks);
    
    /* Perform the actual HFS+ formatting */
    if (format_hfsplus_volume(resolved_path, partno, opts, &params) != 0) {
        error_print("HFS+ formatting failed");
        goto cleanup;
    }
    
    /* Verify the created filesystem */
    if (verify_hfsplus_volume(resolved_path, partno, opts) != 0) {
        error_warning("filesystem verification failed, but volume may still be usable");
    }
    
    error_verbose("HFS+ formatting completed successfully");
    printf("HFS+ volume '%s' created successfully\n", opts->volume_name);
    printf("Filesystem type: HFS+\n");
    printf("Volume size: %lld bytes (%u blocks)\n", 
           (long long)params.device_size, params.total_blocks);
    printf("Block size: %u bytes\n", params.block_size);
    printf("Features: Basic HFS+ (no journaling)\n");
    
    result = 0;
    
cleanup:
    if (resolved_path) {
        free(resolved_path);
    }
    
    return result;
}

/*
 * NAME:    validate_device()
 * DESCRIPTION: Validate device for formatting
 */
static int validate_device(const char *device_path, int force)
{
    struct stat st;
    
    /* Check if device exists and is accessible */
    if (device_validate(device_path, O_RDWR) != 0) {
        error_print_errno("cannot access device %s", device_path);
        return -1;
    }
    
    /* Get device information */
    if (stat(device_path, &st) != 0) {
        error_print_errno("cannot stat device %s", device_path);
        return -1;
    }
    
    /* Check if device is mounted */
    if (device_is_mounted(device_path) > 0) {
        error_print("device %s is currently mounted", device_path);
        return -1;
    }
    
    /* Check if device already contains a filesystem (unless force is used) */
    if (!force) {
        int fd;
        unsigned char buf[512];
        
        suid_enable();
        fd = open(device_path, O_RDONLY);
        suid_disable();
        
        if (fd != -1) {
            if (read(fd, buf, sizeof(buf)) == sizeof(buf)) {
                /* Check for existing filesystem signatures */
                if (buf[0] != 0 || buf[1] != 0) {
                    error_print("device %s appears to contain data, use -f to force formatting", device_path);
                    close(fd);
                    return -1;
                }
            }
            close(fd);
        }
    }
    
    return 0;
}

/*
 * NAME:    calculate_volume_parameters()
 * DESCRIPTION: Calculate volume parameters based on device size
 */
static int calculate_volume_parameters(const char *device_path, mkfs_options_t *opts, 
                                     volume_params_t *params)
{
    memset(params, 0, sizeof(*params));
    
    /* Get device size */
    params->device_size = device_get_size(device_path);
    if (params->device_size <= 0) {
        error_print("cannot determine device size");
        return -1;
    }
    
    /* Set sector size (always 512 for HFS) */
    params->sector_size = 512;
    params->total_sectors = params->device_size / params->sector_size;
    
    /* Calculate allocation block size according to HFS specification */
    /* AB size = max(512, (volume_size / 65536) * 512) for efficiency */
    params->allocation_block_size = 512;
    if (params->device_size > 32 * 1024 * 1024) {  /* > 32MB */
        params->allocation_block_size = ((params->device_size / 65536) / 512) * 512;
        if (params->allocation_block_size < 512) {
            params->allocation_block_size = 512;
        }
        /* Ensure it's a multiple of 512 */
        params->allocation_block_size = (params->allocation_block_size + 511) & ~511;
    }
    
    /* Calculate total allocation blocks (max 65535 for HFS) */
    params->total_allocation_blocks = params->device_size / params->allocation_block_size;
    if (params->total_allocation_blocks > 65535) {
        params->total_allocation_blocks = 65535;
        /* Recalculate allocation block size */
        params->allocation_block_size = params->device_size / 65535;
        params->allocation_block_size = (params->allocation_block_size + 511) & ~511;
    }
    
    /* Calculate file sizes (catalog and extents files) */
    /* Catalog file: minimum 4 allocation blocks, scale with volume size */
    params->catalog_file_size = 4 * params->allocation_block_size;
    if (params->total_allocation_blocks > 1000) {
        params->catalog_file_size = (params->total_allocation_blocks / 250) * params->allocation_block_size;
    }
    
    /* Extents file: minimum 1 allocation block */
    params->extents_file_size = params->allocation_block_size;
    
    /* Calculate free allocation blocks (total minus system files) */
    /* System uses: boot blocks (2), MDB (1), alternate MDB (1), bitmap, catalog, extents */
    uint32_t bitmap_blocks = ((params->total_allocation_blocks + 7) / 8 + params->allocation_block_size - 1) / params->allocation_block_size;
    uint32_t catalog_blocks = (params->catalog_file_size + params->allocation_block_size - 1) / params->allocation_block_size;
    uint32_t extents_blocks = (params->extents_file_size + params->allocation_block_size - 1) / params->allocation_block_size;
    
    params->free_allocation_blocks = params->total_allocation_blocks - bitmap_blocks - catalog_blocks - extents_blocks;
    
    /* Set creation date */
    params->creation_date = hfs_get_safe_time();
    
    return 0;
}

/*
 * NAME:    write_boot_blocks()
 * DESCRIPTION: Write HFS boot blocks (blocks 0-1)
 */
static int write_boot_blocks(int fd, const volume_params_t *params)
{
    unsigned char boot_block[1024];
    
    /* Clear boot blocks */
    memset(boot_block, 0, sizeof(boot_block));
    
    /* Set boot block signature ($4C4B = 'LK') */
    boot_block[0] = 0x4C;
    boot_block[1] = 0x4B;
    
    /* Set basic boot block header fields */
    /* bbEntry: BRA.S instruction (0x6000 + offset) - not used for HFS */
    boot_block[2] = 0x60;
    boot_block[3] = 0x00;
    
    /* bbVersion: new format, no execution */
    boot_block[6] = 0x80;  /* High byte: bit 7 = new format */
    boot_block[7] = 0x15;  /* Low byte: >= $15 for heap use */
    
    /* Write to device */
    if (lseek(fd, 0, SEEK_SET) == -1) {
        error_print_errno("failed to seek to boot blocks");
        return -1;
    }
    
    if (write(fd, boot_block, sizeof(boot_block)) != sizeof(boot_block)) {
        error_print_errno("failed to write boot blocks");
        return -1;
    }
    
    return 0;
}

/*
 * NAME:    write_master_directory_block()
 * DESCRIPTION: Write HFS Master Directory Block according to specification
 */
static int write_master_directory_block(int fd, const volume_params_t *params, 
                                       const mkfs_options_t *opts)
{
    unsigned char mdb_block[512];
    uint32_t hfs_date;
    size_t name_len;
    uint32_t catalog_blocks, extents_blocks;
    uint32_t bitmap_blocks;
    
    /* Clear MDB block */
    memset(mdb_block, 0, sizeof(mdb_block));
    
    /* Convert Unix time to HFS time (seconds since 1904) */
    hfs_date = params->creation_date + HFS_EPOCH_OFFSET;
    
    /* Calculate system file allocation blocks */
    bitmap_blocks = ((params->total_allocation_blocks + 7) / 8 + params->allocation_block_size - 1) / params->allocation_block_size;
    catalog_blocks = (params->catalog_file_size + params->allocation_block_size - 1) / params->allocation_block_size;
    extents_blocks = (params->extents_file_size + params->allocation_block_size - 1) / params->allocation_block_size;
    
    /* Set MDB fields according to HFS specification (Inside Macintosh: Files) */
    
    /* drSigWord: HFS signature ($4244 = 'BD') */
    mdb_block[0] = 0x42;
    mdb_block[1] = 0x44;
    
    /* drCrDate: Creation date */
    mdb_block[2] = (hfs_date >> 24) & 0xFF;
    mdb_block[3] = (hfs_date >> 16) & 0xFF;
    mdb_block[4] = (hfs_date >> 8) & 0xFF;
    mdb_block[5] = hfs_date & 0xFF;
    
    /* drLsMod: Last modification date (same as creation) */
    mdb_block[6] = (hfs_date >> 24) & 0xFF;
    mdb_block[7] = (hfs_date >> 16) & 0xFF;
    mdb_block[8] = (hfs_date >> 8) & 0xFF;
    mdb_block[9] = hfs_date & 0xFF;
    
    /* drAtrb: Volume attributes (bit 8=unmounted cleanly) */
    mdb_block[10] = 0x01;  /* Set unmounted bit */
    mdb_block[11] = 0x00;
    
    /* drNmFls: Number of files in root directory (0 initially) */
    mdb_block[12] = 0x00;
    mdb_block[13] = 0x00;
    
    /* drVBMSt: Volume bitmap start block (always 3) */
    mdb_block[14] = 0x00;
    mdb_block[15] = 0x03;
    
    /* drAllocPtr: Next allocation search pointer (start after system files) */
    uint16_t alloc_ptr = bitmap_blocks + catalog_blocks + extents_blocks;
    mdb_block[16] = (alloc_ptr >> 8) & 0xFF;
    mdb_block[17] = alloc_ptr & 0xFF;
    
    /* drNmAlBlks: Number of allocation blocks */
    mdb_block[18] = (params->total_allocation_blocks >> 8) & 0xFF;
    mdb_block[19] = params->total_allocation_blocks & 0xFF;
    
    /* drAlBlkSiz: Allocation block size */
    mdb_block[20] = (params->allocation_block_size >> 24) & 0xFF;
    mdb_block[21] = (params->allocation_block_size >> 16) & 0xFF;
    mdb_block[22] = (params->allocation_block_size >> 8) & 0xFF;
    mdb_block[23] = params->allocation_block_size & 0xFF;
    
    /* drClpSiz: Default clump size (4 * allocation block size) */
    uint32_t clump_size = params->allocation_block_size * 4;
    mdb_block[24] = (clump_size >> 24) & 0xFF;
    mdb_block[25] = (clump_size >> 16) & 0xFF;
    mdb_block[26] = (clump_size >> 8) & 0xFF;
    mdb_block[27] = clump_size & 0xFF;
    
    /* drAlBlSt: First allocation block (after bitmap) */
    uint16_t first_alloc_block = 3 + bitmap_blocks;
    mdb_block[28] = (first_alloc_block >> 8) & 0xFF;
    mdb_block[29] = first_alloc_block & 0xFF;
    
    /* drNxtCNID: Next catalog node ID (start at 16 per spec) */
    mdb_block[30] = 0x00;
    mdb_block[31] = 0x00;
    mdb_block[32] = 0x00;
    mdb_block[33] = 0x10;
    
    /* drFreeBks: Number of free allocation blocks */
    mdb_block[34] = (params->free_allocation_blocks >> 8) & 0xFF;
    mdb_block[35] = params->free_allocation_blocks & 0xFF;
    
    /* drVN: Volume name (Pascal string format - length byte + chars) */
    name_len = strlen(opts->volume_name);
    if (name_len > 27) name_len = 27;
    mdb_block[36] = name_len;
    memcpy(&mdb_block[37], opts->volume_name, name_len);
    /* Pad remaining bytes with zeros (already done by memset) */
    
    /* drVolBkUp: Volume backup date (0 = never backed up) */
    mdb_block[64] = 0x00;
    mdb_block[65] = 0x00;
    mdb_block[66] = 0x00;
    mdb_block[67] = 0x00;
    
    /* drVSeqNum: Volume sequence number (0) */
    mdb_block[68] = 0x00;
    mdb_block[69] = 0x00;
    
    /* drWrCnt: Write count (0) */
    mdb_block[70] = 0x00;
    mdb_block[71] = 0x00;
    mdb_block[72] = 0x00;
    mdb_block[73] = 0x00;
    
    /* drXTClpSiz: Extents overflow file clump size */
    uint32_t xt_clump = params->extents_file_size;
    mdb_block[74] = (xt_clump >> 24) & 0xFF;
    mdb_block[75] = (xt_clump >> 16) & 0xFF;
    mdb_block[76] = (xt_clump >> 8) & 0xFF;
    mdb_block[77] = xt_clump & 0xFF;
    
    /* drCTClpSiz: Catalog file clump size */
    uint32_t ct_clump = params->catalog_file_size;
    mdb_block[78] = (ct_clump >> 24) & 0xFF;
    mdb_block[79] = (ct_clump >> 16) & 0xFF;
    mdb_block[80] = (ct_clump >> 8) & 0xFF;
    mdb_block[81] = ct_clump & 0xFF;
    
    /* drNmRtDirs: Number of directories in root (0 initially) */
    mdb_block[82] = 0x00;
    mdb_block[83] = 0x00;
    
    /* drFilCnt: Total number of files (0 initially) */
    mdb_block[84] = 0x00;
    mdb_block[85] = 0x00;
    mdb_block[86] = 0x00;
    mdb_block[87] = 0x00;
    
    /* drDirCnt: Total number of directories (1 for root) */
    mdb_block[88] = 0x00;
    mdb_block[89] = 0x00;
    mdb_block[90] = 0x00;
    mdb_block[91] = 0x01;
    
    /* drFndrInfo: Finder info (8 longs, all zeros) */
    /* Already zeroed by memset */
    
    /* drXTFlSize: Extents overflow file size in bytes */
    mdb_block[130] = (params->extents_file_size >> 24) & 0xFF;
    mdb_block[131] = (params->extents_file_size >> 16) & 0xFF;
    mdb_block[132] = (params->extents_file_size >> 8) & 0xFF;
    mdb_block[133] = params->extents_file_size & 0xFF;
    
    /* drXTExtRec: Extents overflow file first extent record */
    uint16_t xt_start_block = bitmap_blocks;
    mdb_block[134] = (xt_start_block >> 8) & 0xFF;
    mdb_block[135] = xt_start_block & 0xFF;
    mdb_block[136] = (extents_blocks >> 8) & 0xFF;
    mdb_block[137] = extents_blocks & 0xFF;
    /* Remaining extent entries are zero */
    
    /* drCTFlSize: Catalog file size in bytes */
    mdb_block[146] = (params->catalog_file_size >> 24) & 0xFF;
    mdb_block[147] = (params->catalog_file_size >> 16) & 0xFF;
    mdb_block[148] = (params->catalog_file_size >> 8) & 0xFF;
    mdb_block[149] = params->catalog_file_size & 0xFF;
    
    /* drCTExtRec: Catalog file first extent record */
    uint16_t ct_start_block = bitmap_blocks + extents_blocks;
    mdb_block[150] = (ct_start_block >> 8) & 0xFF;
    mdb_block[151] = ct_start_block & 0xFF;
    mdb_block[152] = (catalog_blocks >> 8) & 0xFF;
    mdb_block[153] = catalog_blocks & 0xFF;
    /* Remaining extent entries are zero */
    
    /* Write MDB to device */
    if (write(fd, mdb_block, sizeof(mdb_block)) != sizeof(mdb_block)) {
        error_print_errno("failed to write master directory block");
        return -1;
    }
    
    return 0;
}

/*
 * NAME:    write_volume_bitmap()
 * DESCRIPTION: Write HFS volume bitmap with correct allocation block marking
 */
static int write_volume_bitmap(int fd, const volume_params_t *params)
{
    size_t bitmap_size = (params->total_allocation_blocks + 7) / 8;
    size_t bitmap_sectors = (bitmap_size + params->sector_size - 1) / params->sector_size;
    unsigned char *bitmap;
    uint32_t bitmap_blocks, catalog_blocks, extents_blocks;
    uint32_t used_blocks, i;
    
    /* Calculate system file sizes in allocation blocks */
    bitmap_blocks = (bitmap_size + params->allocation_block_size - 1) / params->allocation_block_size;
    catalog_blocks = (params->catalog_file_size + params->allocation_block_size - 1) / params->allocation_block_size;
    extents_blocks = (params->extents_file_size + params->allocation_block_size - 1) / params->allocation_block_size;
    
    /* Total used blocks = bitmap + extents + catalog */
    used_blocks = bitmap_blocks + extents_blocks + catalog_blocks;
    
    error_verbose("bitmap allocation: bitmap=%u, extents=%u, catalog=%u blocks", 
                 bitmap_blocks, extents_blocks, catalog_blocks);
    
    /* Allocate bitmap buffer */
    bitmap = calloc(bitmap_sectors, params->sector_size);
    if (!bitmap) {
        error_print_errno("failed to allocate memory for volume bitmap");
        return -1;
    }
    
    /* Mark system allocation blocks as used */
    /* HFS allocation blocks start after the bitmap area */
    /* Block layout: boot(0-1), MDB(2), bitmap(3+), then allocation blocks */
    
    for (i = 0; i < used_blocks && i < params->total_allocation_blocks; i++) {
        /* Set bit i in the bitmap (bit 0 = allocation block 0) */
        bitmap[i / 8] |= (0x80 >> (i % 8));
    }
    
    error_verbose("marked %u allocation blocks as used in bitmap", used_blocks);
    
    /* Seek to bitmap location (block 3) */
    if (lseek(fd, 3 * params->sector_size, SEEK_SET) == -1) {
        error_print_errno("failed to seek to volume bitmap");
        free(bitmap);
        return -1;
    }
    
    /* Write bitmap */
    if (write(fd, bitmap, bitmap_sectors * params->sector_size) != (ssize_t)(bitmap_sectors * params->sector_size)) {
        error_print_errno("failed to write volume bitmap");
        free(bitmap);
        return -1;
    }
    
    free(bitmap);
    return 0;
}

/*
 * NAME:    initialize_catalog_file()
 * DESCRIPTION: Initialize empty HFS catalog B*-tree
 */
static int initialize_catalog_file(int fd, const volume_params_t *params)
{
    /* TODO: Implement catalog B*-tree initialization */
    /* For now, just write zeros */
    unsigned char *catalog_data = calloc(1, params->catalog_file_size);
    if (!catalog_data) {
        error_print_errno("failed to allocate memory for catalog file");
        return -1;
    }
    
    /* Calculate catalog file location (after bitmap) */
    size_t bitmap_size = (params->total_allocation_blocks + 7) / 8;
    size_t bitmap_sectors = (bitmap_size + params->sector_size - 1) / params->sector_size;
    off_t catalog_offset = (3 + bitmap_sectors) * params->sector_size;
    
    if (lseek(fd, catalog_offset, SEEK_SET) == -1) {
        error_print_errno("failed to seek to catalog file location");
        free(catalog_data);
        return -1;
    }
    
    if (write(fd, catalog_data, params->catalog_file_size) != (ssize_t)params->catalog_file_size) {
        error_print_errno("failed to write catalog file");
        free(catalog_data);
        return -1;
    }
    
    free(catalog_data);
    return 0;
}

/*
 * NAME:    initialize_extents_file()
 * DESCRIPTION: Initialize empty HFS extents overflow B*-tree
 */
static int initialize_extents_file(int fd, const volume_params_t *params)
{
    /* TODO: Implement extents B*-tree initialization */
    /* For now, just write zeros */
    unsigned char *extents_data = calloc(1, params->extents_file_size);
    if (!extents_data) {
        error_print_errno("failed to allocate memory for extents file");
        return -1;
    }
    
    /* Calculate extents file location (after catalog file) */
    size_t bitmap_size = (params->total_allocation_blocks + 7) / 8;
    size_t bitmap_sectors = (bitmap_size + params->sector_size - 1) / params->sector_size;
    off_t extents_offset = (3 + bitmap_sectors) * params->sector_size + params->catalog_file_size;
    
    if (lseek(fd, extents_offset, SEEK_SET) == -1) {
        error_print_errno("failed to seek to extents file location");
        free(extents_data);
        return -1;
    }
    
    if (write(fd, extents_data, params->extents_file_size) != (ssize_t)params->extents_file_size) {
        error_print_errno("failed to write extents file");
        free(extents_data);
        return -1;
    }
    
    free(extents_data);
    return 0;
}

/*
 * NAME:    format_hfs_volume()
 * DESCRIPTION: Perform the actual HFS formatting using libhfs-style logic
 */
static int format_hfs_volume(const char *device_path, int partno, 
                            const mkfs_options_t *opts, const volume_params_t *params)
{
    int fd = -1;
    int result = -1;
    
    /* Validate volume name */
    if (validate_volume_name(opts->volume_name) != 0) {
        return -1;
    }
    
    /* Open device for writing */
    suid_enable();
    fd = open(device_path, O_RDWR);
    suid_disable();
    
    if (fd == -1) {
        error_print_errno("cannot open device %s", device_path);
        return -1;
    }
    
    error_verbose("device opened successfully");
    
    /* Write HFS structures in order following HFS specification */
    
    /* 1. Write boot blocks (blocks 0-1) */
    error_verbose("writing boot blocks");
    if (write_boot_blocks(fd, params) != 0) {
        error_print("failed to write boot blocks");
        goto cleanup;
    }
    
    /* 2. Write Master Directory Block (block 2) */
    error_verbose("writing master directory block");
    if (write_master_directory_block(fd, params, opts) != 0) {
        error_print("failed to write master directory block");
        goto cleanup;
    }
    
    /* 3. Write volume bitmap (starting at block 3) */
    error_verbose("writing volume bitmap");
    if (write_volume_bitmap(fd, params) != 0) {
        error_print("failed to write volume bitmap");
        goto cleanup;
    }
    
    /* 4. Initialize catalog file B*-tree */
    error_verbose("initializing catalog file");
    if (initialize_catalog_file(fd, params) != 0) {
        error_print("failed to initialize catalog file");
        goto cleanup;
    }
    
    /* 5. Initialize extents overflow file B*-tree */
    error_verbose("initializing extents overflow file");
    if (initialize_extents_file(fd, params) != 0) {
        error_print("failed to initialize extents overflow file");
        goto cleanup;
    }
    
    /* 6. Write alternate Master Directory Block (1024 bytes before end, per TN1150) */
    error_verbose("writing alternate master directory block");
    if (lseek(fd, params->device_size - 1024, SEEK_SET) == -1) {
        error_print_errno("failed to seek to alternate MDB location");
        goto cleanup;
    }
    if (write_master_directory_block(fd, params, opts) != 0) {
        error_print("failed to write alternate master directory block");
        goto cleanup;
    }
    
    /* Sync to ensure all data is written */
    if (fsync(fd) != 0) {
        error_print_errno("failed to sync filesystem data");
        goto cleanup;
    }
    
    result = 0;
    
cleanup:
    if (fd != -1) {
        close(fd);
    }
    
    return result;
}

/*
 * NAME:    verify_hfs_volume()
 * DESCRIPTION: Verify the created HFS volume
 */
static int verify_hfs_volume(const char *device_path, int partno, const mkfs_options_t *opts)
{
    hfs_volume_info_t vol_info;
    int fd;
    
    /* Open device for reading */
    suid_enable();
    fd = open(device_path, O_RDONLY);
    suid_disable();
    
    if (fd == -1) {
        error_print_errno("cannot open device for verification");
        return -1;
    }
    
    /* Read volume information */
    if (hfs_read_volume_info(fd, &vol_info) != 0) {
        error_print("failed to read volume information for verification");
        close(fd);
        return -1;
    }
    
    /* Verify filesystem type */
    if (vol_info.fs_type != FS_TYPE_HFS) {
        error_print("verification failed: incorrect filesystem type");
        close(fd);
        return -1;
    }
    
    /* Verify volume name */
    if (strcmp(vol_info.volume_name, opts->volume_name) != 0) {
        error_warning("volume name mismatch: expected '%s', got '%s'", 
                     opts->volume_name, vol_info.volume_name);
    }
    
    error_verbose("volume verification successful");
    error_verbose("  filesystem type: HFS");
    error_verbose("  volume name: %s", vol_info.volume_name);
    error_verbose("  block size: %u", vol_info.block_size);
    error_verbose("  total blocks: %u", vol_info.total_blocks);
    error_verbose("  free blocks: %u", vol_info.free_blocks);
    
    close(fd);
    return 0;
}

/*
 * NAME:    validate_volume_name()
 * DESCRIPTION: Validate HFS volume name according to specification
 */
static int validate_volume_name(const char *vname)
{
    size_t len;
    
    if (!vname) {
        error_print("volume name cannot be null");
        return -1;
    }
    
    len = strlen(vname);
    
    /* HFS volume names are limited to 27 characters */
    if (len == 0 || len > 27) {
        error_print("volume name must be 1-27 characters long");
        return -1;
    }
    
    /* Check for invalid characters */
    for (size_t i = 0; i < len; i++) {
        if (vname[i] == ':' || vname[i] == '\0') {
            error_print("volume name cannot contain ':' or null characters");
            return -1;
        }
        /* Check for non-printable characters */
        if (vname[i] < 32 || vname[i] > 126) {
            error_print("volume name contains invalid character at position %zu", i);
            return -1;
        }
    }
    
    return 0;
}
/*

 * NAME:    calculate_hfsplus_volume_parameters()
 * DESCRIPTION: Calculate HFS+ volume parameters based on device size
 */
static int calculate_hfsplus_volume_parameters(const char *device_path, mkfs_options_t *opts,
                                             hfsplus_volume_params_t *params)
{
    memset(params, 0, sizeof(*params));
    
    /* Get device size */
    params->device_size = device_get_size(device_path);
    if (params->device_size <= 0) {
        error_print("cannot determine device size");
        return -1;
    }
    
    /* Set sector size (always 512) */
    params->sector_size = 512;
    params->total_sectors = params->device_size / params->sector_size;
    
    /* HFS+ uses larger block sizes for efficiency */
    /* Default block size is 4096 bytes for volumes > 1GB, 512 for smaller */
    if (params->device_size > 1024 * 1024 * 1024) {  /* > 1GB */
        params->block_size = 4096;
    } else {
        params->block_size = 512;
    }
    
    /* Calculate total blocks */
    params->total_blocks = params->device_size / params->block_size;
    
    /* HFS+ can handle much larger volumes than HFS */
    if (params->total_blocks > 0xFFFFFFFF) {
        error_print("volume too large for HFS+");
        return -1;
    }
    
    /* Calculate system file sizes (larger than HFS) */
    /* Allocation file: 1 bit per block, rounded up to block size */
    params->allocation_file_size = ((params->total_blocks + 7) / 8 + params->block_size - 1) & ~(params->block_size - 1);
    
    /* Catalog file: larger initial size for HFS+ */
    params->catalog_file_size = params->block_size * 4;  /* 4 blocks minimum */
    if (params->total_blocks > 10000) {
        params->catalog_file_size = params->block_size * (params->total_blocks / 2500);
    }
    
    /* Extents overflow file: 1 block minimum */
    params->extents_file_size = params->block_size;
    
    /* Attributes file: 1 block (for extended attributes) */
    params->attributes_file_size = params->block_size;
    
    /* Startup file: 0 (not used) */
    params->startup_file_size = 0;
    
    /* Calculate free blocks */
    uint32_t system_blocks = 
        (params->allocation_file_size + params->block_size - 1) / params->block_size +
        (params->catalog_file_size + params->block_size - 1) / params->block_size +
        (params->extents_file_size + params->block_size - 1) / params->block_size +
        (params->attributes_file_size + params->block_size - 1) / params->block_size +
        3;  /* Boot blocks + volume header + alternate volume header */
    
    params->free_blocks = params->total_blocks - system_blocks;
    
    /* Set creation date */
    params->creation_date = hfs_get_safe_time();
    
    /* Default options */
    params->enable_journaling = opts->enable_journaling;  /* Use option from command line */
    params->case_sensitive = 0;     /* Case-insensitive by default */
    
    /* Journaling requires additional space */
    if (params->enable_journaling) {
        /* Reserve space for journal (typically 8-32 MB) */
        if (opts->verbose) {
            printf("Journaling enabled - allocating journal blocks\n");
        }
    }
    
    return 0;
}

/*
 * NAME:    format_hfsplus_volume()
 * DESCRIPTION: Perform the actual HFS+ formatting
 */
static int format_hfsplus_volume(const char *device_path, int partno,
                                const mkfs_options_t *opts, const hfsplus_volume_params_t *params)
{
    int fd = -1;
    int result = -1;
    
    /* Validate volume name */
    if (validate_volume_name(opts->volume_name) != 0) {
        return -1;
    }
    
    /* Open device for writing */
    suid_enable();
    fd = open(device_path, O_RDWR);
    suid_disable();
    
    if (fd == -1) {
        error_print_errno("cannot open device %s", device_path);
        return -1;
    }
    
    error_verbose("device opened successfully");
    
    /* Write HFS+ structures in order */
    
    /* 1. Write boot blocks (blocks 0-1) */
    error_verbose("writing HFS+ boot blocks");
    if (write_hfsplus_boot_blocks(fd, params) != 0) {
        error_print("failed to write boot blocks");
        goto cleanup;
    }
    
    /* 2. Write Volume Header (block 2) */
    error_verbose("writing HFS+ volume header");
    if (write_hfsplus_volume_header(fd, params, opts) != 0) {
        error_print("failed to write volume header");
        goto cleanup;
    }
    
    /* 3. Write allocation bitmap */
    error_verbose("writing allocation bitmap");
    if (write_hfsplus_allocation_bitmap(fd, params) != 0) {
        error_print("failed to write allocation bitmap");
        goto cleanup;
    }
    
    /* 4. Initialize catalog file B*-tree */
    error_verbose("initializing catalog file");
    if (initialize_hfsplus_catalog_file(fd, params) != 0) {
        error_print("failed to initialize catalog file");
        goto cleanup;
    }
    
    /* 5. Initialize extents overflow file B*-tree */
    error_verbose("initializing extents overflow file");
    if (initialize_hfsplus_extents_file(fd, params) != 0) {
        error_print("failed to initialize extents overflow file");
        goto cleanup;
    }
    
    /* 6. Write alternate Volume Header (1024 bytes before end, per TN1150) */
    error_verbose("writing alternate volume header");
    if (lseek(fd, params->device_size - 1024, SEEK_SET) == -1) {
        error_print_errno("failed to seek to alternate volume header location");
        goto cleanup;
    }
    if (write_hfsplus_volume_header(fd, params, opts) != 0) {
        error_print("failed to write alternate volume header");
        goto cleanup;
    }
    
    /* Sync to ensure all data is written */
    if (fsync(fd) != 0) {
        error_print_errno("failed to sync filesystem data");
        goto cleanup;
    }
    
    result = 0;
    
cleanup:
    if (fd != -1) {
        close(fd);
    }
    
    return result;
}

/*
 * NAME:    write_hfsplus_boot_blocks()
 * DESCRIPTION: Write HFS+ boot blocks (same as HFS)
 */
static int write_hfsplus_boot_blocks(int fd, const hfsplus_volume_params_t *params)
{
    unsigned char boot_block[1024];
    
    /* Clear boot blocks */
    memset(boot_block, 0, sizeof(boot_block));
    
    /* Set boot block signature ($4C4B = 'LK') */
    boot_block[0] = 0x4C;
    boot_block[1] = 0x4B;
    
    /* Set basic boot block header fields */
    /* bbEntry: BRA.S instruction (0x6000 + offset) - not used for HFS+ */
    boot_block[2] = 0x60;
    boot_block[3] = 0x00;
    
    /* bbVersion: new format, no execution */
    boot_block[6] = 0x80;  /* High byte: bit 7 = new format */
    boot_block[7] = 0x15;  /* Low byte: >= $15 for heap use */
    
    /* Write to device */
    if (lseek(fd, 0, SEEK_SET) == -1) {
        error_print_errno("failed to seek to boot blocks");
        return -1;
    }
    
    if (write(fd, boot_block, sizeof(boot_block)) != sizeof(boot_block)) {
        error_print_errno("failed to write boot blocks");
        return -1;
    }
    
    return 0;
}

/*
 * NAME:    write_hfsplus_volume_header()
 * DESCRIPTION: Write HFS+ Volume Header
 */
static int write_hfsplus_volume_header(int fd, const hfsplus_volume_params_t *params,
                                     const mkfs_options_t *opts)
{
    unsigned char vh_block[512];
    uint32_t hfs_date;
    
    /* Clear volume header block */
    memset(vh_block, 0, sizeof(vh_block));
    
    /* Convert Unix time to HFS time (seconds since 1904) */
    hfs_date = params->creation_date + HFS_EPOCH_OFFSET;
    
    /* Set Volume Header fields according to HFS+ specification */
    
    /* signature: HFS+ signature ($482B = 'H+') */
    vh_block[0] = 0x48;
    vh_block[1] = 0x2B;
    
    /* version: always 4 for HFS+ */
    vh_block[2] = 0x00;
    vh_block[3] = 0x04;
    
    /* attributes: volume attributes (TN1150 kHFSVolumeUnmountedBit=0x0100 at bit 8) */
    uint32_t attributes = 0x00000100;  /* bit 8: volume unmounted cleanly */
    if (params->enable_journaling) {
        attributes |= 0x00002000;  /* kHFSVolumeJournaledMask */
    }
    vh_block[4] = (attributes >> 24) & 0xFF;
    vh_block[5] = (attributes >> 16) & 0xFF;
    vh_block[6] = (attributes >> 8) & 0xFF;
    vh_block[7] = attributes & 0xFF;
    
    /* lastMountedVersion: Mac OS X */
    vh_block[8] = 0x31;   /* '1' */
    vh_block[9] = 0x30;   /* '0' */
    vh_block[10] = 0x2E;  /* '.' */
    vh_block[11] = 0x30;  /* '0' */
    
    /* createDate: Creation date */
    vh_block[12] = (hfs_date >> 24) & 0xFF;
    vh_block[13] = (hfs_date >> 16) & 0xFF;
    vh_block[14] = (hfs_date >> 8) & 0xFF;
    vh_block[15] = hfs_date & 0xFF;
    
    /* modifyDate: Last modification date (same as creation) */
    vh_block[16] = (hfs_date >> 24) & 0xFF;
    vh_block[17] = (hfs_date >> 16) & 0xFF;
    vh_block[18] = (hfs_date >> 8) & 0xFF;
    vh_block[19] = hfs_date & 0xFF;
    
    /* backupDate: Last backup date (0 = never backed up) */
    vh_block[20] = 0x00;
    vh_block[21] = 0x00;
    vh_block[22] = 0x00;
    vh_block[23] = 0x00;
    
    /* checkedDate: Last checked date (same as creation) */
    vh_block[24] = (hfs_date >> 24) & 0xFF;
    vh_block[25] = (hfs_date >> 16) & 0xFF;
    vh_block[26] = (hfs_date >> 8) & 0xFF;
    vh_block[27] = hfs_date & 0xFF;
    
    /* fileCount: Number of files (0 initially) */
    vh_block[28] = 0x00;
    vh_block[29] = 0x00;
    vh_block[30] = 0x00;
    vh_block[31] = 0x00;
    
    /* folderCount: Number of folders (1 for root) */
    vh_block[32] = 0x00;
    vh_block[33] = 0x00;
    vh_block[34] = 0x00;
    vh_block[35] = 0x01;
    
    /* blockSize: Allocation block size */
    vh_block[40] = (params->block_size >> 24) & 0xFF;
    vh_block[41] = (params->block_size >> 16) & 0xFF;
    vh_block[42] = (params->block_size >> 8) & 0xFF;
    vh_block[43] = params->block_size & 0xFF;
    
    /* totalBlocks: Total allocation blocks */
    vh_block[44] = (params->total_blocks >> 24) & 0xFF;
    vh_block[45] = (params->total_blocks >> 16) & 0xFF;
    vh_block[46] = (params->total_blocks >> 8) & 0xFF;
    vh_block[47] = params->total_blocks & 0xFF;
    
    /* freeBlocks: Free allocation blocks */
    vh_block[48] = (params->free_blocks >> 24) & 0xFF;
    vh_block[49] = (params->free_blocks >> 16) & 0xFF;
    vh_block[50] = (params->free_blocks >> 8) & 0xFF;
    vh_block[51] = params->free_blocks & 0xFF;
    
    /* nextAllocation: Next allocation block (start after system files) */
    uint32_t next_alloc = (params->total_blocks - params->free_blocks);
    vh_block[52] = (next_alloc >> 24) & 0xFF;
    vh_block[53] = (next_alloc >> 16) & 0xFF;
    vh_block[54] = (next_alloc >> 8) & 0xFF;
    vh_block[55] = next_alloc & 0xFF;
    
    /* rsrcClumpSize: Default resource fork clump size (offset +56) */
    uint32_t rsrc_clump = params->block_size * 4;  /* 4 blocks default */
    vh_block[56] = (rsrc_clump >> 24) & 0xFF;
    vh_block[57] = (rsrc_clump >> 16) & 0xFF;
    vh_block[58] = (rsrc_clump >> 8) & 0xFF;
    vh_block[59] = rsrc_clump & 0xFF;
    
    /* dataClumpSize: Default data fork clump size (offset +60) */
    uint32_t data_clump = params->block_size * 4;  /* 4 blocks default */
    vh_block[60] = (data_clump >> 24) & 0xFF;
    vh_block[61] = (data_clump >> 16) & 0xFF;
    vh_block[62] = (data_clump >> 8) & 0xFF;
    vh_block[63] = data_clump & 0xFF;
    
    /* nextCatalogID: Next catalog node ID (start at 16, offset +64, per TN1150) */
    vh_block[64] = 0x00;
    vh_block[65] = 0x00;
    vh_block[66] = 0x00;
    vh_block[67] = 0x10;
    
    /* writeCount: Volume write count (0) */
    vh_block[68] = 0x00;
    vh_block[69] = 0x00;
    vh_block[70] = 0x00;
    vh_block[71] = 0x00;
    
    /* TODO: Add fork data structures for system files */
    /* For now, we'll leave them as zeros which is valid for an empty volume */
    
    /* Write Volume Header to device */
    if (write(fd, vh_block, sizeof(vh_block)) != sizeof(vh_block)) {
        error_print_errno("failed to write volume header");
        return -1;
    }
    
    return 0;
}

/*
 * NAME:    write_hfsplus_allocation_bitmap()
 * DESCRIPTION: Write HFS+ allocation bitmap
 */
static int write_hfsplus_allocation_bitmap(int fd, const hfsplus_volume_params_t *params)
{
    size_t bitmap_size = (params->total_blocks + 7) / 8;
    size_t bitmap_blocks = (bitmap_size + params->block_size - 1) / params->block_size;
    unsigned char *bitmap;
    uint32_t used_blocks, i;
    
    /* Calculate system file sizes in blocks */
    uint32_t allocation_blocks = (params->allocation_file_size + params->block_size - 1) / params->block_size;
    uint32_t catalog_blocks = (params->catalog_file_size + params->block_size - 1) / params->block_size;
    uint32_t extents_blocks = (params->extents_file_size + params->block_size - 1) / params->block_size;
    uint32_t attributes_blocks = (params->attributes_file_size + params->block_size - 1) / params->block_size;
    
    /* Total used blocks = allocation bitmap + catalog + extents + attributes */
    used_blocks = allocation_blocks + catalog_blocks + extents_blocks + attributes_blocks;
    
    error_verbose("HFS+ allocation bitmap: allocation=%u, catalog=%u, extents=%u, attributes=%u blocks", 
                 allocation_blocks, catalog_blocks, extents_blocks, attributes_blocks);
    
    /* Allocate bitmap buffer */
    bitmap = calloc(bitmap_blocks, params->block_size);
    if (!bitmap) {
        error_print_errno("failed to allocate memory for allocation bitmap");
        return -1;
    }
    
    /* Mark system blocks as used */
    for (i = 0; i < used_blocks && i < params->total_blocks; i++) {
        /* Set bit i in the bitmap (bit 0 = allocation block 0) */
        bitmap[i / 8] |= (0x80 >> (i % 8));
    }
    
    error_verbose("marked %u allocation blocks as used in bitmap", used_blocks);
    
    /* Seek to allocation bitmap location (after volume header) */
    if (lseek(fd, 3 * params->sector_size, SEEK_SET) == -1) {
        error_print_errno("failed to seek to allocation bitmap");
        free(bitmap);
        return -1;
    }
    
    /* Write bitmap */
    if (write(fd, bitmap, bitmap_blocks * params->block_size) != (ssize_t)(bitmap_blocks * params->block_size)) {
        error_print_errno("failed to write allocation bitmap");
        free(bitmap);
        return -1;
    }
    
    free(bitmap);
    return 0;
}

/*
 * NAME:    initialize_hfsplus_catalog_file()
 * DESCRIPTION: Initialize HFS+ catalog B-tree with proper structure per TN1150
 */
static int initialize_hfsplus_catalog_file(int fd, const hfsplus_volume_params_t *params)
{
    const uint32_t NODE_SIZE = 4096;  /* Standard HFS+ B-tree node size */
    const uint16_t MAX_KEY_LENGTH = 516;  /* sizeof(HFSPlusCatalogKey) */
    
    unsigned char *node_data;
    uint32_t total_nodes;
    size_t bitmap_size;
    size_t bitmap_blocks;
    off_t catalog_offset;
    
    /* Calculate catalog file location */
    bitmap_size = (params->total_blocks + 7) / 8;
    bitmap_blocks = (bitmap_size + params->block_size - 1) / params->block_size;
    catalog_offset = (3 * params->sector_size) + (bitmap_blocks * params->block_size);
    
    total_nodes = params->catalog_file_size / NODE_SIZE;
    
    /* Allocate buffer for one node */
    node_data = calloc(1, NODE_SIZE);
    if (!node_data) {
        error_print_errno("failed to allocate memory for catalog node");
        return -1;
    }
    
    /* === Node 0: Header Node === */
    
    /* Node Descriptor (14 bytes) */
    /* fLink (next node) */
    node_data[0] = 0x00;
    node_data[1] = 0x00;
    node_data[2] = 0x00;
    node_data[3] = 0x00;
    
    /* bLink (previous node) */
    node_data[4] = 0x00;
    node_data[5] = 0x00;
    node_data[6] = 0x00;
    node_data[7] = 0x00;
    
    /* kind = 1 (header node) */
    node_data[8] = 0x01;
    
    /* height = 0 */
    node_data[9] = 0x00;
    
    /* numRecords = 3 (BTHeaderRec, user data, map record) */
    node_data[10] = 0x00;
    node_data[11] = 0x03;
    
    /* reserved */
    node_data[12] = 0x00;
    node_data[13] = 0x00;
    
    /* BTHeaderRec starts at offset 14 (106 bytes) */
    int offset = 14;
    
    /* treeDepth = 1 (header + one leaf for root folder) */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x01;
    
    /* rootNode = 1 (node 1 contains root folder) */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x01;
    
    /* leafRecords = 1 (only root folder initially) */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x01;
    
    /* firstLeafNode = 1 */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x01;
    
    /* lastLeafNode = 1 */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x01;
    
    /* nodeSize = 4096 */
    node_data[offset++] = 0x10;
    node_data[offset++] = 0x00;
    
    /* maxKeyLength = 516 */
    node_data[offset++] = (MAX_KEY_LENGTH >> 8) & 0xFF;
    node_data[offset++] = MAX_KEY_LENGTH & 0xFF;
    
    /* totalNodes */
    node_data[offset++] = (total_nodes >> 24) & 0xFF;
    node_data[offset++] = (total_nodes >> 16) & 0xFF;
    node_data[offset++] = (total_nodes >> 8) & 0xFF;
    node_data[offset++] = total_nodes & 0xFF;
    
    /* freeNodes = total_nodes - 2 (header + root leaf used) */
    uint32_t free_nodes = total_nodes - 2;
    node_data[offset++] = (free_nodes >> 24) & 0xFF;
    node_data[offset++] = (free_nodes >> 16) & 0xFF;
    node_data[offset++] = (free_nodes >> 8) & 0xFF;
    node_data[offset++] = free_nodes & 0xFF;
    
    /* reserved1 */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    
    /* clumpSize (obsolete, use catalog clump from volume header) */
    uint32_t clump_size = params->catalog_file_size;
    node_data[offset++] = (clump_size >> 24) & 0xFF;
    node_data[offset++] = (clump_size >> 16) & 0xFF;
    node_data[offset++] = (clump_size >> 8) & 0xFF;
    node_data[offset++] = clump_size & 0xFF;
    
    /* btreeType = 0 (Catalog) */
    node_data[offset++] = 0x00;
    
    /* keyCompareType = 0xCF (case-insensitive, TN1150 default) */
    node_data[offset++] = 0xCF;
    
    /* attributes = 0 (no special attributes) */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    
    /* reserved3[16] (64 bytes) - already zeroed by calloc */
    offset += 64;
    
    /* Map record (128 bytes showing nodes 0-1 allocated) */
    /* Set bits 0 and 1 to mark header and root leaf as used */
    int map_offset = NODE_SIZE - 256;  /* Map record typically at end of node */
    node_data[map_offset] = 0xC0;  /* Binary: 11000000 = nodes 0,1 used */
    
    /* Record offsets at end of node (3 records + free space offset) */
    /* Offset table grows backwards from end of node */
    int rec_offset_base = NODE_SIZE - 2;
    
    /* Free space offset (points after map record) */
    uint16_t free_offset = map_offset + 256;
    node_data[rec_offset_base--] = (free_offset >> 8) & 0xFF;
    node_data[rec_offset_base--] = free_offset & 0xFF;
    
    /* Record 2: map record offset */
    uint16_t map_rec_offset = map_offset;
    node_data[rec_offset_base--] = (map_rec_offset >> 8) & 0xFF;
    node_data[rec_offset_base--] = map_rec_offset & 0xFF;
    
    /* Record 1: user data record (empty, 128 bytes reserved) */
    uint16_t user_offset = map_offset - 128;
    node_data[rec_offset_base--] = (user_offset >> 8) & 0xFF;
    node_data[rec_offset_base--] = user_offset & 0xFF;
    
    /* Record 0: BTHeaderRec offset */
    uint16_t header_offset = 14;
    node_data[rec_offset_base--] = (header_offset >> 8) & 0xFF;
    node_data[rec_offset_base--] = header_offset & 0xFF;
    
    /* Write header node */
    if (lseek(fd, catalog_offset, SEEK_SET) == -1) {
        error_print_errno("failed to seek to catalog file location");
        free(node_data);
        return -1;
    }
    
    if (write(fd, node_data, NODE_SIZE) != NODE_SIZE) {
        error_print_errno("failed to write catalog header node");
        free(node_data);
        return -1;
    }
    
    /* === Node 1: Root Folder Leaf Node === */
    memset(node_data, 0, NODE_SIZE);
    
    /* Node Descriptor */
    /* fLink = 0 (no next leaf) */
    node_data[0] = 0x00;
    node_data[1] = 0x00;
    node_data[2] = 0x00;
    node_data[3] = 0x00;
    
    /* bLink = 0 (no previous leaf) */
    node_data[4] = 0x00;
    node_data[5] = 0x00;
    node_data[6] = 0x00;
    node_data[7] = 0x00;
    
    /* kind = -1 (0xFF = leaf node) */
    node_data[8] = 0xFF;
    
    /* height = 1 */
    node_data[9] = 0x01;
    
    /* numRecords = 1 (root folder record) */
    node_data[10] = 0x00;
    node_data[11] = 0x01;
    
    /* reserved */
    node_data[12] = 0x00;
    node_data[13] = 0x00;
    
    /* Root folder record at offset 14 */
    offset = 14;
    
    /* Catalog key: parentID=1 (kHFSRootParentID), name="" (empty for root) */
    /* keyLength = 6 (minimum: 2 bytes parentID field + 2 bytes name length + 2 for keyLength itself) */
    uint16_t key_length = 6;
    node_data[offset++] = (key_length >> 8) & 0xFF;
    node_data[offset++] = key_length & 0xFF;
    
    /* parentID = 1 (kHFSRootParentID) */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x01;
    
    /* nodeName.length = 0 (empty name for root) */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    
    /* Catalog folder record (88 bytes minimum) */
    /* recordType = kHFSPlusFolderRecord (0x0001) */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x01;
    
    /* flags = 0 */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    
    /* valence = 0 (no items in root initially) */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    
    /* folderID = 2 (kHFSRootFolderID) */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x02;
    
    /* createDate, contentModDate, attributeModDate, accessDate (all use current HFS time) */
    uint32_t hfs_time = (uint32_t)(params->creation_date + HFS_EPOCH_OFFSET);
    for (int i = 0; i < 4; i++) {
        node_data[offset++] = (hfs_time >> 24) & 0xFF;
        node_data[offset++] = (hfs_time >> 16) & 0xFF;
        node_data[offset++] = (hfs_time >> 8) & 0xFF;
        node_data[offset++] = hfs_time & 0xFF;
    }
    
    /* backupDate = 0 */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    
    /* permissions (80 bytes, complex structure, zero for now) */
    offset += 80;
    
    /* userInfo + finderInfo (32 bytes total) - zero */
    offset += 32;
    
    /* textEncoding = 0 */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    
    /* reserved = 0 */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    
    /* Record offset table at end */
    rec_offset_base = NODE_SIZE - 2;
    
    /* Free space offset */
    uint16_t leaf_free_offset = offset;
    node_data[rec_offset_base--] = (leaf_free_offset >> 8) & 0xFF;
    node_data[rec_offset_base--] = leaf_free_offset & 0xFF;
    
    /* Record 0 offset (root folder record) */
    uint16_t rec0_offset = 14;
    node_data[rec_offset_base--] = (rec0_offset >> 8) & 0xFF;
    node_data[rec_offset_base--] = rec0_offset & 0xFF;
    
    /* Write root folder leaf node */
    if (write(fd, node_data, NODE_SIZE) != NODE_SIZE) {
        error_print_errno("failed to write catalog root leaf node");
        free(node_data);
        return -1;
    }
    
    /* Zero out remaining catalog nodes */
    memset(node_data, 0, NODE_SIZE);
    for (uint32_t i = 2; i < total_nodes; i++) {
        if (write(fd, node_data, NODE_SIZE) != NODE_SIZE) {
            error_print_errno("failed to write catalog empty nodes");
            free(node_data);
            return -1;
        }
    }
    
    free(node_data);
    return 0;
}

/*
 * NAME:    initialize_hfsplus_extents_file()
 * DESCRIPTION: Initialize HFS+ extents overflow B-tree per TN1150
 */
static int initialize_hfsplus_extents_file(int fd, const hfsplus_volume_params_t *params)
{
    const uint32_t NODE_SIZE = 4096;  /* Standard HFS+ B-tree node size */
    const uint16_t MAX_KEY_LENGTH = 10;  /* sizeof(HFSPlusExtentKey) */
    
    unsigned char *node_data;
    uint32_t total_nodes;
    size_t bitmap_size;
    size_t bitmap_blocks;
    off_t extents_offset;
    
    /* Calculate extents file location (after catalog file) */
    bitmap_size = (params->total_blocks + 7) / 8;
    bitmap_blocks = (bitmap_size + params->block_size - 1) / params->block_size;
    extents_offset = (3 * params->sector_size) + 
                     (bitmap_blocks * params->block_size) + 
                     params->catalog_file_size;
    
    total_nodes = params->extents_file_size / NODE_SIZE;
    
    /* Allocate buffer for one node */
    node_data = calloc(1, NODE_SIZE);
    if (!node_data) {
        error_print_errno("failed to allocate memory for extents node");
        return -1;
    }
    
    /* === Node 0: Header Node === */
    
    /* Node Descriptor (14 bytes) */
    /* fLink (next node) */
    node_data[0] = 0x00;
    node_data[1] = 0x00;
    node_data[2] = 0x00;
    node_data[3] = 0x00;
    
    /* bLink (previous node) */
    node_data[4] = 0x00;
    node_data[5] = 0x00;
    node_data[6] = 0x00;
    node_data[7] = 0x00;
    
    /* kind = 1 (header node) */
    node_data[8] = 0x01;
    
    /* height = 0 */
    node_data[9] = 0x00;
    
    /* numRecords = 3 (BTHeaderRec, user data, map record) */
    node_data[10] = 0x00;
    node_data[11] = 0x03;
    
    /* reserved */
    node_data[12] = 0x00;
    node_data[13] = 0x00;
    
    /* BTHeaderRec starts at offset 14 (106 bytes) */
    int offset = 14;
    
    /* treeDepth = 0 (empty tree - no extents overflow initially) */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    
    /* rootNode = 0 (no root - tree is empty) */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    
    /* leafRecords = 0 (no records initially) */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    
    /* firstLeafNode = 0 (no leaves yet) */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    
    /* lastLeafNode = 0 (no leaves yet) */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    
    /* nodeSize = 4096 */
    node_data[offset++] = 0x10;
    node_data[offset++] = 0x00;
    
    /* maxKeyLength = 10 */
    node_data[offset++] = (MAX_KEY_LENGTH >> 8) & 0xFF;
    node_data[offset++] = MAX_KEY_LENGTH & 0xFF;
    
    /* totalNodes */
    node_data[offset++] = (total_nodes >> 24) & 0xFF;
    node_data[offset++] = (total_nodes >> 16) & 0xFF;
    node_data[offset++] = (total_nodes >> 8) & 0xFF;
    node_data[offset++] = total_nodes & 0xFF;
    
    /* freeNodes = total_nodes - 1 (only header used) */
    uint32_t free_nodes = total_nodes - 1;
    node_data[offset++] = (free_nodes >> 24) & 0xFF;
    node_data[offset++] = (free_nodes >> 16) & 0xFF;
    node_data[offset++] = (free_nodes >> 8) & 0xFF;
    node_data[offset++] = free_nodes & 0xFF;
    
    /* reserved1 */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    
    /* clumpSize (obsolete, use extents clump from volume header) */
    uint32_t clump_size = params->extents_file_size;
    node_data[offset++] = (clump_size >> 24) & 0xFF;
    node_data[offset++] = (clump_size >> 16) & 0xFF;
    node_data[offset++] = (clump_size >> 8) & 0xFF;
    node_data[offset++] = clump_size & 0xFF;
    
    /* btreeType = 255 (0xFF = Extents Overflow) */
    node_data[offset++] = 0xFF;
    
    /* keyCompareType = 0 (simple comparison for extents keys) */
    node_data[offset++] = 0x00;
    
    /* attributes = 0 (no special attributes) */
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    node_data[offset++] = 0x00;
    
    /* reserved3[16] (64 bytes) - already zeroed by calloc */
    offset += 64;
    
    /* Map record (128 bytes showing only node 0 allocated) */
    /* Set bit 0 to mark header as used */
    int map_offset = NODE_SIZE - 256;  /* Map record at end of node */
    node_data[map_offset] = 0x80;  /* Binary: 10000000 = node 0 used */
    
    /* Record offsets at end of node (3 records + free space offset) */
    /* Offset table grows backwards from end of node */
    int rec_offset_base = NODE_SIZE - 2;
    
    /* Free space offset (points after map record) */
    uint16_t free_offset = map_offset + 256;
    node_data[rec_offset_base--] = (free_offset >> 8) & 0xFF;
    node_data[rec_offset_base--] = free_offset & 0xFF;
    
    /* Record 2: map record offset */
    uint16_t map_rec_offset = map_offset;
    node_data[rec_offset_base--] = (map_rec_offset >> 8) & 0xFF;
    node_data[rec_offset_base--] = map_rec_offset & 0xFF;
    
    /* Record 1: user data record (empty, 128 bytes reserved) */
    uint16_t user_offset = map_offset - 128;
    node_data[rec_offset_base--] = (user_offset >> 8) & 0xFF;
    node_data[rec_offset_base--] = user_offset & 0xFF;
    
    /* Record 0: BTHeaderRec offset */
    uint16_t header_offset = 14;
    node_data[rec_offset_base--] = (header_offset >> 8) & 0xFF;
    node_data[rec_offset_base--] = header_offset & 0xFF;
    
    /* Write header node */
    if (lseek(fd, extents_offset, SEEK_SET) == -1) {
        error_print_errno("failed to seek to extents file location");
        free(node_data);
        return -1;
    }
    
    if (write(fd, node_data, NODE_SIZE) != NODE_SIZE) {
        error_print_errno("failed to write extents header node");
        free(node_data);
        return -1;
    }
    
    /* Zero out remaining extents nodes (all free) */
    memset(node_data, 0, NODE_SIZE);
    for (uint32_t i = 1; i < total_nodes; i++) {
        if (write(fd, node_data, NODE_SIZE) != NODE_SIZE) {
            error_print_errno("failed to write extents empty nodes");
            free(node_data);
            return -1;
        }
    }
    
    free(node_data);
    return 0;
}

/*
 * NAME:    verify_hfsplus_volume()
 * DESCRIPTION: Verify the created HFS+ volume
 */
static int verify_hfsplus_volume(const char *device_path, int partno, const mkfs_options_t *opts)
{
    hfs_volume_info_t vol_info;
    int fd;
    
    /* Open device for reading */
    suid_enable();
    fd = open(device_path, O_RDONLY);
    suid_disable();
    
    if (fd == -1) {
        error_print_errno("cannot open device for verification");
        return -1;
    }
    
    /* Read volume information */
    if (hfs_read_volume_info(fd, &vol_info) != 0) {
        error_print("failed to read volume information for verification");
        close(fd);
        return -1;
    }
    
    /* Verify filesystem type */
    if (vol_info.fs_type != FS_TYPE_HFSPLUS) {
        error_print("verification failed: incorrect filesystem type");
        close(fd);
        return -1;
    }
    
    error_verbose("HFS+ volume verification successful");
    error_verbose("  filesystem type: HFS+");
    error_verbose("  block size: %u", vol_info.block_size);
    error_verbose("  total blocks: %u", vol_info.total_blocks);
    error_verbose("  free blocks: %u", vol_info.free_blocks);
    
    close(fd);
    return 0;
}