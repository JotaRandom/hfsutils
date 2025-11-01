#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <endian.h>
#include <time.h>

#include "common/hfs_detect.h"
#include "hfsutil/hfsplus_format.h"

/* Convert UTF-8 to HFS+ Unicode */
int hfsplus_utf8_to_unicode(const char *utf8, struct hfsplus_unistr *unicode) {
    if (!utf8 || !unicode) {
        return -1;
    }
    
    size_t len = strlen(utf8);
    if (len > 255) {
        len = 255;  /* Truncate if too long */
    }
    
    unicode->length = htobe16((uint16_t)len);
    
    /* Simple ASCII to Unicode conversion */
    /* TODO: Implement proper UTF-8 to UTF-16 conversion */
    for (size_t i = 0; i < len; i++) {
        unicode->unicode[i] = htobe16((uint16_t)(unsigned char)utf8[i]);
    }
    
    return 0;
}

/* Get optimal block size for volume */
uint32_t hfsplus_get_optimal_block_size(uint64_t volume_size) {
    /* Choose block size based on volume size */
    if (volume_size < (64ULL * 1024 * 1024)) {         /* < 64MB */
        return 512;
    } else if (volume_size < (256ULL * 1024 * 1024)) { /* < 256MB */
        return 1024;
    } else if (volume_size < (1ULL * 1024 * 1024 * 1024)) { /* < 1GB */
        return 2048;
    } else {                                            /* >= 1GB */
        return 4096;
    }
}

/* Validate format options */
int hfsplus_validate_options(const hfsplus_format_opts_t *opts) {
    if (!opts || !opts->device_path) {
        fprintf(stderr, "Error: Invalid options or device path\n");
        return -1;
    }
    
    if (opts->block_size != 0) {
        if (opts->block_size < HFSPLUS_MIN_BLOCK_SIZE || 
            opts->block_size > HFSPLUS_MAX_BLOCK_SIZE) {
            fprintf(stderr, "Error: Block size must be between %d and %d bytes\n",
                    HFSPLUS_MIN_BLOCK_SIZE, HFSPLUS_MAX_BLOCK_SIZE);
            return -1;
        }
        
        /* Check if block size is power of 2 */
        if ((opts->block_size & (opts->block_size - 1)) != 0) {
            fprintf(stderr, "Error: Block size must be a power of 2\n");
            return -1;
        }
    }
    
    if (opts->volume_name && strlen(opts->volume_name) > 255) {
        fprintf(stderr, "Error: Volume name too long (max 255 characters)\n");
        return -1;
    }
    
    return 0;
}

/* Calculate volume sizes */
int hfsplus_calculate_sizes(const hfsplus_format_opts_t *opts,
                           uint32_t *total_blocks, uint32_t *block_size) {
    struct stat st;
    uint64_t volume_size;
    
    if (stat(opts->device_path, &st) < 0) {
        perror("stat");
        return -1;
    }
    
    if (S_ISBLK(st.st_mode)) {
        /* Block device - get size */
        int fd = open(opts->device_path, O_RDONLY);
        if (fd < 0) {
            perror("open");
            return -1;
        }
        
        if (lseek(fd, 0, SEEK_END) < 0) {
            perror("lseek");
            close(fd);
            return -1;
        }
        
        volume_size = lseek(fd, 0, SEEK_CUR);
        close(fd);
    } else {
        /* Regular file */
        volume_size = st.st_size;
    }
    
    if (opts->total_size > 0 && opts->total_size < volume_size) {
        volume_size = opts->total_size;
    }
    
    /* Determine block size */
    if (opts->block_size > 0) {
        *block_size = opts->block_size;
    } else {
        *block_size = hfsplus_get_optimal_block_size(volume_size);
    }
    
    *total_blocks = (uint32_t)(volume_size / *block_size);
    
    if (opts->verbose) {
        printf("Volume size: %llu bytes (%u blocks of %u bytes)\n",
               (unsigned long long)volume_size, *total_blocks, *block_size);
    }
    
    return 0;
}

/* Set volume dates */
void hfsplus_set_dates(struct hfsplus_vh *vh) {
    time_t now = hfs_get_safe_time();
    uint32_t hfs_time = (uint32_t)(now + HFS_EPOCH_OFFSET);
    
    vh->createDate = htobe32(hfs_time);
    vh->modifyDate = htobe32(hfs_time);
    vh->backupDate = 0;  /* Never backed up */
    vh->checkedDate = htobe32(hfs_time);
}

/* Initialize B-tree header */
void hfsplus_init_btree_header(struct hfsplus_btree_header_rec *header,
                              uint16_t node_size, uint8_t btree_type) {
    memset(header, 0, sizeof(*header));
    
    header->tree_depth = 0;         /* Empty tree */
    header->root_node = 0;          /* No root node yet */
    header->leaf_records = 0;       /* No records yet */
    header->first_leaf_node = 0;    /* No leaf nodes yet */
    header->last_leaf_node = 0;     /* No leaf nodes yet */
    header->node_size = htobe16(node_size);
    header->max_key_len = htobe16(255 * 2 + 6);  /* Max Unicode key */
    header->total_nodes = 0;        /* Will be set later */
    header->free_nodes = 0;         /* Will be set later */
    header->clump_size = htobe32(node_size * 4);  /* 4 nodes per clump */
    header->btree_type = btree_type;
    header->key_compare_type = 0xCF; /* Case-folding, Unicode */
    header->attributes = htobe32(0x80); /* Variable index keys */
}

/* Create volume header */
int hfsplus_create_volume_header(int fd, const hfsplus_format_opts_t *opts,
                                struct hfsplus_vh *vh) {
    uint32_t total_blocks, block_size;
    
    if (hfsplus_calculate_sizes(opts, &total_blocks, &block_size) < 0) {
        return -1;
    }
    
    memset(vh, 0, sizeof(*vh));
    
    /* Basic volume information */
    vh->signature = htobe16(HFSPLUS_SIGNATURE);
    vh->version = htobe16(HFSPLUS_VERSION);
    vh->attributes = htobe32(HFSPLUS_VOL_UNMNT);
    vh->lastMountedVersion = htobe32(0x482B4C78);  /* 'H+Lx' */
    vh->journalInfoBlock = 0;  /* No journal initially */
    
    /* Set dates */
    hfsplus_set_dates(vh);
    
    /* Volume statistics */
    vh->fileCount = 0;
    vh->folderCount = htobe32(1);  /* Root folder */
    
    /* Block information */
    vh->blockSize = htobe32(block_size);
    vh->totalBlocks = htobe32(total_blocks);
    vh->freeBlocks = htobe32(total_blocks - 10);  /* Reserve some blocks */
    
    /* Allocation information */
    vh->nextAllocation = htobe32(10);  /* Start after system files */
    vh->rsrcClumpSize = htobe32(block_size * 4);
    vh->dataClumpSize = htobe32(block_size * 4);
    vh->nextCatalogID = htobe32(HFSPLUS_FIRST_USER_ID);
    
    /* Write count and encoding */
    vh->writeCount = htobe32(1);
    vh->encodingsBitmap = htobe64(0x1);  /* UTF-8 encoding */
    
    /* Initialize Finder info */
    memset(vh->finderInfo, 0, sizeof(vh->finderInfo));
    
    if (opts->verbose) {
        printf("Created HFS+ volume header:\n");
        printf("  Signature: 0x%04X\n", be16toh(vh->signature));
        printf("  Version: %u\n", be16toh(vh->version));
        printf("  Block size: %u bytes\n", be32toh(vh->blockSize));
        printf("  Total blocks: %u\n", be32toh(vh->totalBlocks));
        printf("  Free blocks: %u\n", be32toh(vh->freeBlocks));
    }
    
    return 0;
}

/* Create allocation file */
int hfsplus_create_allocation_file(int fd, const hfsplus_format_opts_t *opts,
                                  struct hfsplus_vh *vh) {
    uint32_t total_blocks = be32toh(vh->totalBlocks);
    uint32_t block_size = be32toh(vh->blockSize);
    uint32_t bitmap_blocks = (total_blocks + (block_size * 8) - 1) / (block_size * 8);
    
    /* Initialize allocation file fork data */
    vh->allocationFile.logicalSize = htobe64(bitmap_blocks * block_size);
    vh->allocationFile.clumpSize = htobe32(block_size);
    vh->allocationFile.totalBlocks = htobe32(bitmap_blocks);
    
    /* Set up initial extent */
    vh->allocationFile.extents[0].startBlock = htobe32(1);  /* Block 1 */
    vh->allocationFile.extents[0].blockCount = htobe32(bitmap_blocks);
    
    /* Clear remaining extents */
    for (int i = 1; i < 8; i++) {
        vh->allocationFile.extents[i].startBlock = 0;
        vh->allocationFile.extents[i].blockCount = 0;
    }
    
    if (opts->verbose) {
        printf("Allocation file: %u blocks starting at block 1\n", bitmap_blocks);
    }
    
    return 0;
}

/* Create extents overflow file */
int hfsplus_create_extents_file(int fd, const hfsplus_format_opts_t *opts,
                               struct hfsplus_vh *vh) {
    uint32_t block_size = be32toh(vh->blockSize);
    uint32_t extents_blocks = 4;  /* Start with 4 blocks */
    
    /* Initialize extents file fork data */
    vh->extentsFile.logicalSize = htobe64(extents_blocks * block_size);
    vh->extentsFile.clumpSize = htobe32(block_size * 4);
    vh->extentsFile.totalBlocks = htobe32(extents_blocks);
    
    /* Set up initial extent */
    uint32_t alloc_blocks = be32toh(vh->allocationFile.totalBlocks);
    vh->extentsFile.extents[0].startBlock = htobe32(1 + alloc_blocks);
    vh->extentsFile.extents[0].blockCount = htobe32(extents_blocks);
    
    /* Clear remaining extents */
    for (int i = 1; i < 8; i++) {
        vh->extentsFile.extents[i].startBlock = 0;
        vh->extentsFile.extents[i].blockCount = 0;
    }
    
    if (opts->verbose) {
        printf("Extents file: %u blocks starting at block %u\n", 
               extents_blocks, be32toh(vh->extentsFile.extents[0].startBlock));
    }
    
    return 0;
}

/* Create catalog file */
int hfsplus_create_catalog_file(int fd, const hfsplus_format_opts_t *opts,
                               struct hfsplus_vh *vh) {
    uint32_t block_size = be32toh(vh->blockSize);
    uint32_t catalog_blocks = 8;  /* Start with 8 blocks */
    
    /* Initialize catalog file fork data */
    vh->catalogFile.logicalSize = htobe64(catalog_blocks * block_size);
    vh->catalogFile.clumpSize = htobe32(block_size * 8);
    vh->catalogFile.totalBlocks = htobe32(catalog_blocks);
    
    /* Set up initial extent */
    uint32_t start_block = be32toh(vh->extentsFile.extents[0].startBlock) +
                          be32toh(vh->extentsFile.extents[0].blockCount);
    vh->catalogFile.extents[0].startBlock = htobe32(start_block);
    vh->catalogFile.extents[0].blockCount = htobe32(catalog_blocks);
    
    /* Clear remaining extents */
    for (int i = 1; i < 8; i++) {
        vh->catalogFile.extents[i].startBlock = 0;
        vh->catalogFile.extents[i].blockCount = 0;
    }
    
    if (opts->verbose) {
        printf("Catalog file: %u blocks starting at block %u\n", 
               catalog_blocks, start_block);
    }
    
    return 0;
}

/* Create attributes file */
int hfsplus_create_attributes_file(int fd, const hfsplus_format_opts_t *opts,
                                  struct hfsplus_vh *vh) {
    /* Attributes file is optional and initially empty */
    memset(&vh->attributesFile, 0, sizeof(vh->attributesFile));
    
    if (opts->verbose) {
        printf("Attributes file: empty (will be created on demand)\n");
    }
    
    return 0;
}

/* Write volume header to disk */
int hfsplus_write_volume_header(int fd, const struct hfsplus_vh *vh) {
    uint32_t block_size = be32toh(vh->blockSize);
    uint32_t total_blocks = be32toh(vh->totalBlocks);
    
    /* Write primary volume header at block 0 */
    if (lseek(fd, 1024, SEEK_SET) < 0) {  /* HFS+ header at offset 1024 */
        perror("lseek");
        return -1;
    }
    
    if (write(fd, vh, sizeof(*vh)) != sizeof(*vh)) {
        perror("write volume header");
        return -1;
    }
    
    /* Write backup volume header at last block */
    off_t backup_offset = (off_t)(total_blocks - 1) * block_size + 1024;
    if (lseek(fd, backup_offset, SEEK_SET) < 0) {
        perror("lseek backup");
        return -1;
    }
    
    if (write(fd, vh, sizeof(*vh)) != sizeof(*vh)) {
        perror("write backup volume header");
        return -1;
    }
    
    return 0;
}

/* Main HFS+ format function */
int hfsplus_format_volume(const hfsplus_format_opts_t *opts) {
    int fd = -1;
    struct hfsplus_vh vh;
    int result = -1;
    
    if (hfsplus_validate_options(opts) < 0) {
        return -1;
    }
    
    if (opts->verbose) {
        printf("Formatting '%s' as HFS+ volume", opts->device_path);
        if (opts->volume_name) {
            printf(" '%s'", opts->volume_name);
        }
        printf("\n");
    }
    
    /* Check if device exists and is not mounted */
    if (!opts->force) {
        int test_fd = open(opts->device_path, O_RDONLY);
        if (test_fd >= 0) {
            hfs_volume_info_t vol_info;
            if (hfs_read_volume_info(test_fd, &vol_info) == 0) {
                fprintf(stderr, "Warning: Device appears to contain a filesystem\n");
                fprintf(stderr, "Use -f to force formatting\n");
                close(test_fd);
                return -1;
            }
            close(test_fd);
        }
    }
    
    /* Open device */
    fd = open(opts->device_path, O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }
    
    /* Create volume header */
    if (hfsplus_create_volume_header(fd, opts, &vh) < 0) {
        goto cleanup;
    }
    
    /* Create system files */
    if (hfsplus_create_allocation_file(fd, opts, &vh) < 0) {
        goto cleanup;
    }
    
    if (hfsplus_create_extents_file(fd, opts, &vh) < 0) {
        goto cleanup;
    }
    
    if (hfsplus_create_catalog_file(fd, opts, &vh) < 0) {
        goto cleanup;
    }
    
    if (hfsplus_create_attributes_file(fd, opts, &vh) < 0) {
        goto cleanup;
    }
    
    /* Write volume header to disk */
    if (hfsplus_write_volume_header(fd, &vh) < 0) {
        goto cleanup;
    }
    
    /* TODO: Initialize allocation bitmap */
    /* TODO: Initialize B-tree files with proper headers */
    /* TODO: Create root directory entry */
    
    if (opts->verbose) {
        printf("HFS+ volume formatting completed successfully\n");
    }
    
    result = 0;
    
cleanup:
    if (fd >= 0) {
        close(fd);
    }
    
    return result;
}