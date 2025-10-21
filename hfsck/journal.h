/*
 * journal.h - HFS+ journaling support for hfsck
 * Copyright (C) 2025 Pablo Lezaeta
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef JOURNAL_H
#define JOURNAL_H

#include <stdint.h>
#include <sys/types.h>

/* HFS+ Journaling Constants */
#define HFSPLUS_SIGNATURE       0x482B
#define JOURNAL_MAGIC          0x4A4E4C78
#define JOURNAL_ENDIAN         0x12345678
#define HFS_MAX_TIME           0xFFFFFFFF
#define HFS_EPOCH_OFFSET       2082844800

/* Journal Flags */
#define JOURNAL_ON_OTHER_DEVICE (1 << 0)
#define JOURNAL_NEED_INIT       (1 << 1)

/* Volume Header Attributes */
#define HFSPLUS_VOL_JOURNALED   (1 << 8)
#define HFSPLUS_VOL_DIRTY       (1 << 15)

/* HFS+ Journal Info Block */
struct HFSPlus_JournalInfoBlock {
    uint32_t flags;
    uint32_t deviceSignature[8];
    uint64_t offset;
    uint64_t size;
    uint8_t reserved[432];
} __attribute__((packed));

/* HFS+ Journal Header */
struct HFSPlus_JournalHeader {
    uint32_t magic;
    uint32_t endian;
    uint64_t start;
    uint64_t end;
    uint64_t size;
    uint32_t blhdrSize;
    uint32_t checksum;
    uint32_t jhdrSize;
} __attribute__((packed));

/* HFS+ Block List Header */
struct HFSPlus_BlockListHeader {
    uint16_t bsize;
    uint16_t numBlocks;
    uint32_t checksum;
    uint32_t reserved[8];
} __attribute__((packed));

/* HFS+ Block Info */
struct HFSPlus_BlockInfo {
    uint64_t bnum;
    uint32_t bsize;
    uint64_t next;
} __attribute__((packed));

/* HFS+ Volume Header (simplified for journaling) */
struct HFSPlus_VolumeHeader {
    uint16_t signature;
    uint16_t version;
    uint32_t attributes;
    uint32_t lastMountedVersion;
    uint32_t journalInfoBlock;
    uint32_t createDate;
    uint32_t modifyDate;
    uint32_t backupDate;
    uint32_t checkedDate;
    uint32_t fileCount;
    uint32_t folderCount;
    uint32_t blockSize;
    uint32_t totalBlocks;
    uint32_t freeBlocks;
    uint32_t nextAllocation;
    uint32_t rsrcClumpSize;
    uint32_t dataClumpSize;
    uint32_t nextCatalogID;
    uint32_t writeCount;
    uint64_t encodingsBitmap;
    uint8_t finderInfo[32];
    /* ForkData structures would follow here */
} __attribute__((packed));

/* Function prototypes */
int journal_is_valid(int fd, const struct HFSPlus_VolumeHeader *vh);
int journal_replay(int fd, const struct HFSPlus_VolumeHeader *vh, int repair);
int journal_disable(int fd, struct HFSPlus_VolumeHeader *vh);
uint32_t journal_calculate_checksum(const void *data, size_t size);
void journal_log_error(const char *device, const char *message);

#endif /* JOURNAL_H */