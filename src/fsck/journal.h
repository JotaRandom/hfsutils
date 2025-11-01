/*
 * journal.h - Enhanced HFS+ journaling support for fsck.hfs
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
#include <fcntl.h>
#include <unistd.h>
#include <endian.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/* HFS+ Journaling Constants */
#define JOURNAL_MAGIC          0x4A4E4C78
#define JOURNAL_ENDIAN         0x12345678

/* Journal Flags */
#define JOURNAL_ON_OTHER_DEVICE (1 << 0)
#define JOURNAL_NEED_INIT       (1 << 1)

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
    uint8_t reserved[88];
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

/* Forward declaration - actual definition is in fsck_hfs.h */
struct HFSPlus_VolumeHeader;

/* Function prototypes - these are already declared in fsck_hfs.h */

/* HFS+ specific checking functions */
int hfsplus_check_volume(const char *device_path, int partition_number, int check_options);

/* Global variables for compatibility */
extern int options;

/* Compatibility macros */
#define VERBOSE (options & HFSCK_VERBOSE)
#define REPAIR  (options & HFSCK_REPAIR)
#define YES     (options & HFSCK_YES)

#endif /* JOURNAL_H */