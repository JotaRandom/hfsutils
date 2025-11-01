/*
 * hfs_detect.h - HFS/HFS+ filesystem detection and structures
 * Copyright (C) 2025 Pablo Lezaeta
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef HFS_DETECT_H
#define HFS_DETECT_H

#include <stdint.h>
#include <time.h>
#include <endian.h>

/* HFS/HFS+ Common Constants */
#define HFS_BLOCK_SIZE          512
#define HFSPLUS_BLOCK_SIZE      4096
#define HFS_SUPERBLOCK_OFFSET   1024
#define HFS_EPOCH_OFFSET        2082844800  /* 1904 to 1970 */
#define HFS_MAX_TIME            0xFFFFFFFF  /* Feb 6, 2040 */

/* HFS Signatures */
#define HFS_SIGNATURE           0x4244      /* "BD" */
#define HFSPLUS_SIGNATURE       0x482B      /* "H+" */
#define HFSX_SIGNATURE          0x4858      /* "HX" */

/* Filesystem Types */
typedef enum {
    FS_TYPE_UNKNOWN = 0,
    FS_TYPE_HFS,
    FS_TYPE_HFSPLUS,
    FS_TYPE_HFSX
} hfs_fs_type_t;

/* HFS Master Directory Block */
struct hfs_mdb {
    uint16_t drSigWord;         /* Signature word */
    uint32_t drCrDate;          /* Creation date */
    uint32_t drLsMod;           /* Last modification date */
    uint16_t drAtrb;            /* Volume attributes */
    uint16_t drNmFls;           /* Number of files in root directory */
    uint16_t drVBMSt;           /* Volume bitmap start block */
    uint16_t drAlBlSt;          /* Allocation block start */
    uint32_t drAlBlkSiz;        /* Allocation block size */
    uint32_t drClpSiz;          /* Default clump size */
    uint16_t drNmAlBlks;        /* Number of allocation blocks */
    uint32_t drNxtCNID;         /* Next catalog node ID */
    uint16_t drFreeBks;         /* Number of free allocation blocks */
    char drVN[28];              /* Volume name (27 chars + null) */
    uint32_t drVolBkUp;         /* Volume backup date */
    uint16_t drVSeqNum;         /* Volume sequence number */
    uint32_t drWrCnt;           /* Volume write count */
    uint32_t drXTClpSiz;        /* Extents overflow file clump size */
    uint32_t drCTClpSiz;        /* Catalog file clump size */
    uint16_t drNmRtDirs;        /* Number of directories in root */
    uint32_t drFilCnt;          /* Number of files in volume */
    uint32_t drDirCnt;          /* Number of directories in volume */
    uint32_t drFndrInfo[8];     /* Finder info */
    uint16_t drEmbedSigWord;    /* Embedded volume signature */
    uint32_t drEmbedExtent[2];  /* Embedded volume extent */
    uint32_t drXTFlSize;        /* Extents overflow file size */
    uint16_t drXTExtRec[3][2];  /* Extents overflow file extents */
    uint32_t drCTFlSize;        /* Catalog file size */
    uint16_t drCTExtRec[3][2];  /* Catalog file extents */
} __attribute__((packed));

/* HFS+ Extent Descriptor */
struct hfsplus_extent {
    uint32_t startBlock;        /* First allocation block */
    uint32_t blockCount;        /* Number of allocation blocks */
} __attribute__((packed));

/* HFS+ Fork Data */
struct hfsplus_fork_data {
    uint64_t logicalSize;       /* Logical size of fork */
    uint32_t clumpSize;         /* Fork clump size */
    uint32_t totalBlocks;       /* Total blocks used by fork */
    struct hfsplus_extent extents[8];  /* Initial extents */
} __attribute__((packed));

/* HFS+ Volume Header */
struct hfsplus_vh {
    uint16_t signature;         /* Volume signature */
    uint16_t version;           /* Volume version */
    uint32_t attributes;        /* Volume attributes */
    uint32_t lastMountedVersion; /* Last mounted version */
    uint32_t journalInfoBlock;  /* Journal info block */
    uint32_t createDate;        /* Volume creation date */
    uint32_t modifyDate;        /* Volume modification date */
    uint32_t backupDate;        /* Volume backup date */
    uint32_t checkedDate;       /* Volume checked date */
    uint32_t fileCount;         /* Number of files */
    uint32_t folderCount;       /* Number of folders */
    uint32_t blockSize;         /* Allocation block size */
    uint32_t totalBlocks;       /* Total allocation blocks */
    uint32_t freeBlocks;        /* Free allocation blocks */
    uint32_t nextAllocation;    /* Next allocation block */
    uint32_t rsrcClumpSize;     /* Resource fork clump size */
    uint32_t dataClumpSize;     /* Data fork clump size */
    uint32_t nextCatalogID;     /* Next catalog node ID */
    uint32_t writeCount;        /* Volume write count */
    uint64_t encodingsBitmap;   /* Encodings used bitmap */
    uint8_t finderInfo[32];     /* Finder info */
    struct hfsplus_fork_data allocationFile;    /* Allocation file */
    struct hfsplus_fork_data extentsFile;       /* Extents overflow file */
    struct hfsplus_fork_data catalogFile;       /* Catalog file */
    struct hfsplus_fork_data attributesFile;    /* Attributes file */
    struct hfsplus_fork_data startupFile;       /* Startup file */
} __attribute__((packed));

/* Volume Information Structure */
typedef struct {
    hfs_fs_type_t fs_type;
    int fd;
    char *device_path;
    
    union {
        struct hfs_mdb hfs;
        struct hfsplus_vh hfsplus;
    } sb;
    
    /* Common fields */
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t free_blocks;
    time_t create_date;
    time_t modify_date;
    char volume_name[256];
} hfs_volume_info_t;

/* Function Prototypes */
hfs_fs_type_t hfs_detect_fs_type(int fd);
hfs_fs_type_t hfs_detect_filesystem_type(const char *device_path, int partition_number);
const char *hfs_get_fs_type_name(hfs_fs_type_t fs_type);
int hfs_read_volume_info(int fd, hfs_volume_info_t *vol_info);
int hfs_validate_dates(time_t date, const char *field_name);
time_t hfs_get_safe_time(void);
void hfs_log_date_adjustment(const char *path, time_t original, time_t adjusted);

/* Endianness conversion macros */
#define HFS_BE16(x) be16toh(x)
#define HFS_BE32(x) be32toh(x)
#define HFS_BE64(x) be64toh(x)
#define HFS_TO_BE16(x) htobe16(x)
#define HFS_TO_BE32(x) htobe32(x)
#define HFS_TO_BE64(x) htobe64(x)

#endif /* HFS_DETECT_H */