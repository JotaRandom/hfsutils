/*
 * journal.c - HFS+ journaling support for hfsck
 * Copyright (C) 2025 Pablo Lezaeta
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <fcntl.h>
#include <unistd.h>
#include <endian.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "journal.h"

/*
 * NAME:        journal_calculate_checksum()
 * DESCRIPTION: Calculate checksum for HFS+ journal structures
 *              Uses simple 32-bit word summation as per HFS+ specification
 * PARAMETERS:  data - pointer to data to checksum
 *              size - size of data in bytes (must be multiple of 4)
 * RETURNS:     32-bit checksum value in host byte order
 */
uint32_t journal_calculate_checksum(const void *data, size_t size)
{
    uint32_t sum = 0;
    const uint32_t *ptr = (const uint32_t *)data;
    
    /* Sum all 32-bit words in big-endian format */
    for (size_t i = 0; i < size / 4; i++) {
        sum += be32toh(ptr[i]);
    }
    
    return sum;
}

/*
 * NAME:        journal_log_error()
 * DESCRIPTION: Log error messages to hfsutils.log with timestamp
 *              Provides comprehensive logging for journal operations
 *              and troubleshooting. Log file is created if it doesn't exist.
 * PARAMETERS:  device - device name or NULL for generic journal messages
 *              message - error message to log
 * RETURNS:     void
 * NOTES:       Write errors are silently ignored to prevent logging loops
 */
void journal_log_error(const char *device, const char *message)
{
    int fd = open("hfsutils.log", O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd >= 0) {
        char buf[512];
        time_t now = time(NULL);
        struct tm *tm = localtime(&now);
        
        /* Format: [YYYY-MM-DD HH:MM:SS] device: message */
        snprintf(buf, sizeof(buf), "[%04d-%02d-%02d %02d:%02d:%02d] %s: %s\n",
                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                tm->tm_hour, tm->tm_min, tm->tm_sec,
                device ? device : "journal", message);
        
        if (write(fd, buf, strlen(buf)) == -1) {
            /* Ignore write errors for logging to prevent infinite loops */
        }
        close(fd);
    }
}

/*
 * Check if journal is valid and accessible
 */
int journal_is_valid(int fd, const struct HFSPlus_VolumeHeader *vh)
{
    uint32_t attributes = be32toh(vh->attributes);
    
    /* Check if journaling is enabled */
    if (!(attributes & HFSPLUS_VOL_JOURNALED)) {
        return 0; /* Not journaled, but valid */
    }
    
    uint32_t blockSize = be32toh(vh->blockSize);
    uint32_t jibBlock = be32toh(vh->journalInfoBlock);
    
    /* Read Journal Info Block */
    struct HFSPlus_JournalInfoBlock jib;
    if (lseek(fd, (off_t)jibBlock * blockSize, SEEK_SET) == -1) {
        journal_log_error(NULL, "Failed to seek to Journal Info Block");
        return -1;
    }
    
    if (read(fd, &jib, sizeof(jib)) != sizeof(jib)) {
        journal_log_error(NULL, "Failed to read Journal Info Block");
        return -1;
    }
    
    uint32_t flags = be32toh(jib.flags);
    
    /* Check for external journal (not supported) */
    if (flags & JOURNAL_ON_OTHER_DEVICE) {
        journal_log_error(NULL, "External journal not supported");
        return -1;
    }
    
    /* Check if journal needs initialization */
    if (flags & JOURNAL_NEED_INIT) {
        journal_log_error(NULL, "Journal needs initialization");
        return -1;
    }
    
    /* Read Journal Header */
    struct HFSPlus_JournalHeader jh;
    uint64_t journal_offset = be64toh(jib.offset);
    
    if (lseek(fd, journal_offset, SEEK_SET) == -1) {
        journal_log_error(NULL, "Failed to seek to Journal Header");
        return -1;
    }
    
    if (read(fd, &jh, sizeof(jh)) != sizeof(jh)) {
        journal_log_error(NULL, "Failed to read Journal Header");
        return -1;
    }
    
    /* Validate journal header */
    if (be32toh(jh.magic) != JOURNAL_MAGIC) {
        journal_log_error(NULL, "Invalid journal magic");
        return -1;
    }
    
    if (be32toh(jh.endian) != JOURNAL_ENDIAN) {
        journal_log_error(NULL, "Invalid journal endianness");
        return -1;
    }
    
    return 1; /* Valid journal */
}

/*
 * Replay journal transactions
 */
int journal_replay(int fd, const struct HFSPlus_VolumeHeader *vh, int repair)
{
    uint32_t blockSize = be32toh(vh->blockSize);
    uint32_t jibBlock = be32toh(vh->journalInfoBlock);
    
    /* Read Journal Info Block */
    struct HFSPlus_JournalInfoBlock jib;
    if (lseek(fd, (off_t)jibBlock * blockSize, SEEK_SET) == -1) {
        return -EIO;
    }
    
    if (read(fd, &jib, sizeof(jib)) != sizeof(jib)) {
        return -EIO;
    }
    
    uint32_t flags = be32toh(jib.flags);
    
    /* Check for unsupported configurations */
    if (flags & JOURNAL_ON_OTHER_DEVICE) {
        journal_log_error(NULL, "External journal not supported");
        return -EINVAL;
    }
    
    /* Read Journal Header */
    struct HFSPlus_JournalHeader jh;
    uint64_t journal_offset = be64toh(jib.offset);
    
    if (lseek(fd, journal_offset, SEEK_SET) == -1) {
        journal_log_error(NULL, "Failed to seek to journal");
        return -EIO;
    }
    
    if (read(fd, &jh, sizeof(jh)) != sizeof(jh)) {
        journal_log_error(NULL, "Failed to read journal header");
        return -EIO;
    }
    
    /* Validate journal header */
    if (be32toh(jh.magic) != JOURNAL_MAGIC || 
        be32toh(jh.endian) != JOURNAL_ENDIAN) {
        if (repair) {
            /* Mark journal for reinitialization */
            jib.flags = htobe32(be32toh(jib.flags) | JOURNAL_NEED_INIT);
            if (lseek(fd, (off_t)jibBlock * blockSize, SEEK_SET) != -1) {
                if (write(fd, &jib, sizeof(jib)) == -1) {
                    /* Ignore write errors during repair */
                }
            }
        }
        journal_log_error(NULL, "Corrupt journal header");
        return -EINVAL;
    }
    
    uint64_t start = be64toh(jh.start);
    uint64_t end = be64toh(jh.end);
    uint64_t pos = start;
    
    journal_log_error(NULL, "Replaying journal transactions");
    
    /* Replay transactions */
    while (pos < end) {
        /* Read Block List Header */
        struct HFSPlus_BlockListHeader blh;
        if (lseek(fd, journal_offset + pos, SEEK_SET) == -1) {
            journal_log_error(NULL, "Failed to seek to block list");
            return -EIO;
        }
        
        if (read(fd, &blh, sizeof(blh)) != sizeof(blh)) {
            journal_log_error(NULL, "Failed to read block list header");
            return -EIO;
        }
        
        /* Validate checksum */
        uint32_t calculated_checksum = journal_calculate_checksum(&blh, sizeof(blh) - sizeof(blh.checksum));
        if (calculated_checksum != be32toh(blh.checksum)) {
            if (repair) {
                jib.flags = htobe32(be32toh(jib.flags) | JOURNAL_NEED_INIT);
                if (lseek(fd, (off_t)jibBlock * blockSize, SEEK_SET) != -1) {
                    if (write(fd, &jib, sizeof(jib)) == -1) {
                        /* Ignore write errors during repair */
                    }
                }
            }
            journal_log_error(NULL, "Invalid block list checksum");
            return -EINVAL;
        }
        
        pos += sizeof(blh);
        uint16_t numBlocks = be16toh(blh.numBlocks);
        
        /* Process each block in the transaction */
        for (uint16_t i = 0; i < numBlocks; i++) {
            struct HFSPlus_BlockInfo bi;
            if (read(fd, &bi, sizeof(bi)) != sizeof(bi)) {
                journal_log_error(NULL, "Failed to read block info");
                return -EIO;
            }
            
            uint32_t bsize = be32toh(bi.bsize);
            uint64_t bnum = be64toh(bi.bnum);
            
            /* Allocate buffer for block data */
            uint8_t *data = malloc(bsize);
            if (!data) {
                journal_log_error(NULL, "Memory allocation failed");
                return -ENOMEM;
            }
            
            /* Read block data from journal */
            if (read(fd, data, bsize) != bsize) {
                free(data);
                journal_log_error(NULL, "Failed to read block data");
                return -EIO;
            }
            
            /* Write block data to its location in the volume */
            if (repair) {
                off_t block_offset = (off_t)bnum * blockSize;
                if (lseek(fd, block_offset, SEEK_SET) == -1) {
                    free(data);
                    journal_log_error(NULL, "Failed to seek to volume block");
                    return -EIO;
                }
                
                if (write(fd, data, bsize) != bsize) {
                    free(data);
                    journal_log_error(NULL, "Failed to write volume block");
                    return -EIO;
                }
            }
            
            free(data);
            pos = be64toh(bi.next);
            
            /* Seek to next block info */
            if (i < numBlocks - 1) {
                if (lseek(fd, journal_offset + pos, SEEK_SET) == -1) {
                    journal_log_error(NULL, "Failed to seek to next block");
                    return -EIO;
                }
            }
        }
    }
    
    /* Update journal header to mark transactions as replayed */
    if (repair) {
        jh.start = htobe64(end);
        if (lseek(fd, journal_offset, SEEK_SET) == -1) {
            journal_log_error(NULL, "Failed to seek to journal header");
            return -EIO;
        }
        
        if (write(fd, &jh, sizeof(jh)) != sizeof(jh)) {
            journal_log_error(NULL, "Failed to update journal header");
            return -EIO;
        }
    }
    
    journal_log_error(NULL, "Journal replay completed successfully");
    return 0;
}

/*
 * Disable journaling on a volume
 */
int journal_disable(int fd, struct HFSPlus_VolumeHeader *vh)
{
    uint32_t attributes = be32toh(vh->attributes);
    
    /* Clear journaling bit */
    attributes &= ~HFSPLUS_VOL_JOURNALED;
    vh->attributes = htobe32(attributes);
    
    /* Write updated volume header */
    if (lseek(fd, 1024, SEEK_SET) == -1) {
        journal_log_error(NULL, "Failed to seek to volume header");
        return -EIO;
    }
    
    if (write(fd, vh, sizeof(*vh)) != sizeof(*vh)) {
        journal_log_error(NULL, "Failed to write volume header");
        return -EIO;
    }
    
    journal_log_error(NULL, "Journaling disabled");
    return 0;
}