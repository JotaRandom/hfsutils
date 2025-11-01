/*
 * fsck_hfs.h - Header file for fsck.hfs embedded dependencies
 * Copyright (C) 1996-1998 Robert Leslie
 */

#ifndef FSCK_HFS_H
#define FSCK_HFS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <endian.h>

/* Include only the essential libhfs header */
#include "../shared/libhfs.h"

/* Include common utilities */
#include "../shared/hfs_detect.h"
#include "../shared/hfs_utils.h"
#include "../shared/version.h"
#include "../shared/suid.h"
#include "../shared/common_utils.h"
#include "../shared/device_utils.h"
#include "../shared/error_utils.h"

/* Redefine ERROR macro to not use goto */
#undef ERROR
#define ERROR(code, str) do { hfs_error = (str); errno = (code); } while (0)

/* Additional constants for fsck */
#define HFS_OPT_NOCACHE   0x0100

/* HFS catalog record types */
#define cdrDirRec    1
#define cdrFilRec    2
#define cdrThdRec    3
#define cdrFThdRec   4

/* HFS node types */
#define ndIndxNode   0
#define ndHdrNode    1
#define ndMapNode    2
#define ndLeafNode   255

/* HFS constants */
#define HFS_MAX_CATKEY_LEN  37
#define fsRtParID           1

/* hfsck option flags */
#define HFSCK_REPAIR      0x0001
#define HFSCK_VERBOSE     0x0100
#define HFSCK_YES         0x0200

/* Standard fsck exit codes */
#define FSCK_OK                 0   /* No errors found */
#define FSCK_CORRECTED          1   /* Errors found and corrected */
#define FSCK_REBOOT_REQUIRED    2   /* System should be rebooted */
#define FSCK_UNCORRECTED        4   /* Errors found but not corrected */
#define FSCK_OPERATIONAL_ERROR  8   /* Operational error */
#define FSCK_USAGE_ERROR       16   /* Usage or syntax error */
#define FSCK_CANCELLED         32   /* fsck canceled by user request */
#define FSCK_LIBRARY_ERROR    128   /* Shared library error */

/* HFS+ Volume attributes */
#define HFSPLUS_VOL_JOURNALED   0x00002000

/* Filesystem type constants */
#define FS_TYPE_UNKNOWN    0
#define FS_TYPE_HFS        1
#define FS_TYPE_HFSPLUS    2

/* HFS+ Volume Header structure (simplified) */
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
};

/* Volume management functions */
void v_init(hfsvol *vol, int flags);
int v_open(hfsvol *vol, const char *path, int mode);
int v_geometry(hfsvol *vol, int pnum);
int v_close(hfsvol *vol);
int getvol(hfsvol **vol);

/* Low-level I/O functions */
int l_getmdb(hfsvol *vol, MDB *mdb, int backup);

/* B-tree functions */
int bt_readhdr(btree *bt);
int bt_getnode(node *n);

/* File functions */
void f_selectfork(hfsfile *f, int fork);

/* Data conversion functions */
unsigned long d_ltime(unsigned long mtime);
unsigned long d_mtime(time_t unix_time);

/* HFS checking functions */
int hfs_check_volume(const char *path, int pnum, int check_options);

/* HFS+ checking functions */
int hfsplus_check_volume(const char *device_path, int partition_number, int check_options);

/* Low-level block I/O functions */
int l_getblock(void *priv, unsigned long block_num, void *buffer);
int l_putblock(void *priv, unsigned long block_num, const void *buffer);

/* B-tree node functions */
int bt_putnode(node *n);

/* User interaction */
int ask(const char *question, ...);

/* Utility functions for enhanced checking */
char *mctime(unsigned long secs);
char *extstr(ExtDescriptor *ext);
char *extrecstr(ExtDataRec *rec);
void outhex(unsigned char *data, unsigned int len);

/* Journal functions */
int journal_is_valid(int fd, struct HFSPlus_VolumeHeader *vh);
int journal_replay(int fd, struct HFSPlus_VolumeHeader *vh, int repair_mode);
int journal_disable(int fd, struct HFSPlus_VolumeHeader *vh);

/* Global variables */
extern const char *argv0, *bargv0;
extern int options;

/* Error handling */
extern const char *hfs_error;

/* Additional type definitions */
typedef unsigned char block[HFS_BLOCKSZ];
typedef unsigned char byte;

/* Note: CatKeyRec and CatDataRec are already defined in apple.h */

/* HFS record access macros are defined in libhfs.h */

#endif /* FSCK_HFS_H */