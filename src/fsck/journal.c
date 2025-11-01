/*
 * journal.c - Enhanced HFS+ journaling support for fsck.hfs
 * Copyright (C) 2025 Pablo Lezaeta
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "fsck_common.h"
#include "../embedded/fsck/fsck_hfs.h"
#include "journal.h"

/*
 * NAME:        journal_calculate_checksum()
 * DESCRIPTION: Calculate checksum for HFS+ journal structures with enhanced validation
 */
uint32_t journal_calculate_checksum(const void *data, size_t size)
{
    uint32_t sum = 0;
    const uint32_t *ptr = (const uint32_t *)data;
    
    /* Ensure size is multiple of 4 */
    if (size % 4 != 0) {
        if (VERBOSE) {
            printf("Warning: journal checksum size %zu is not multiple of 4\n", size);
        }
        size = (size / 4) * 4; /* Truncate to multiple of 4 */
    }
    
    /* Sum all 32-bit words in big-endian format */
    for (size_t i = 0; i < size / 4; i++) {
        sum += be32toh(ptr[i]);
    }
    
    return sum;
}

/*
 * NAME:        journal_log_error()
 * DESCRIPTION: Enhanced error logging with better error handling
 */
void journal_log_error(const char *device, const char *message)
{
    int fd = open("hfsutils.log", O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd >= 0) {
        char buf[512];
        time_t now = time(NULL);
        struct tm *tm = localtime(&now);
        
        /* Format: [YYYY-MM-DD HH:MM:SS] device: message */
        int written = snprintf(buf, sizeof(buf), "[%04d-%02d-%02d %02d:%02d:%02d] %s: %s\n",
                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                tm->tm_hour, tm->tm_min, tm->tm_sec,
                device ? device : "journal", message);
        
        if (written > 0 && written < (int)sizeof(buf)) {
            ssize_t result = write(fd, buf, written);
            if (result == -1 && VERBOSE) {
                /* Only report write errors in verbose mode to prevent loops */
                fprintf(stderr, "Warning: failed to write to log file: %s\n", strerror(errno));
            }
        }
        close(fd);
    } else if (VERBOSE) {
        fprintf(stderr, "Warning: failed to open log file: %s\n", strerror(errno));
    }
}

/*
 * NAME:        journal_is_valid()
 * DESCRIPTION: Enhanced journal validation with comprehensive checks
 */
int journal_is_valid(int fd, struct HFSPlus_VolumeHeader *vh)
{
    uint32_t attributes = be32toh(vh->attributes);
    
    /* Check if journaling is enabled */
    if (!(attributes & HFSPLUS_VOL_JOURNALED)) {
        if (VERBOSE) {
            printf("Volume is not journaled\n");
        }
        return 0; /* Not journaled, but valid */
    }
    
    uint32_t blockSize = be32toh(vh->blockSize);
    uint32_t jibBlock = be32toh(vh->journalInfoBlock);
    uint32_t totalBlocks = be32toh(vh->totalBlocks);
    
    /* Validate journal info block location */
    if (jibBlock == 0) {
        journal_log_error(NULL, "Journal info block is zero");
        return -1;
    }
    
    if (jibBlock >= totalBlocks) {
        journal_log_error(NULL, "Journal info block beyond volume end");
        return -1;
    }
    
    /* Read Journal Info Block */
    struct HFSPlus_JournalInfoBlock jib;
    off_t jib_offset = (off_t)jibBlock * blockSize;
    
    if (lseek(fd, jib_offset, SEEK_SET) == -1) {
        journal_log_error(NULL, "Failed to seek to Journal Info Block");
        return -1;
    }
    
    if (read(fd, &jib, sizeof(jib)) != sizeof(jib)) {
        journal_log_error(NULL, "Failed to read Journal Info Block");
        return -1;
    }
    
    uint32_t flags = be32toh(jib.flags);
    
    if (VERBOSE) {
        printf("Journal Info Block:\n");
        printf("  Flags: 0x%08x\n", flags);
        printf("  Offset: %llu\n", (unsigned long long)be64toh(jib.offset));
        printf("  Size: %llu\n", (unsigned long long)be64toh(jib.size));
    }
    
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
    
    /* Validate journal offset and size */
    uint64_t journal_offset = be64toh(jib.offset);
    uint64_t journal_size = be64toh(jib.size);
    
    if (journal_offset == 0) {
        journal_log_error(NULL, "Journal offset is zero");
        return -1;
    }
    
    if (journal_size == 0) {
        journal_log_error(NULL, "Journal size is zero");
        return -1;
    }
    
    /* Check if journal fits within volume */
    uint64_t volume_size = (uint64_t)totalBlocks * blockSize;
    if (journal_offset + journal_size > volume_size) {
        journal_log_error(NULL, "Journal extends beyond volume end");
        return -1;
    }
    
    /* Read Journal Header */
    struct HFSPlus_JournalHeader jh;
    
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
        char msg[64];
        snprintf(msg, sizeof(msg), "Invalid journal magic: 0x%08x", be32toh(jh.magic));
        journal_log_error(NULL, msg);
        return -1;
    }
    
    if (be32toh(jh.endian) != JOURNAL_ENDIAN) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Invalid journal endianness: 0x%08x", be32toh(jh.endian));
        journal_log_error(NULL, msg);
        return -1;
    }
    
    /* Validate journal header fields */
    uint64_t start = be64toh(jh.start);
    uint64_t end = be64toh(jh.end);
    uint64_t size = be64toh(jh.size);
    
    if (VERBOSE) {
        printf("Journal Header:\n");
        printf("  Magic: 0x%08x\n", be32toh(jh.magic));
        printf("  Endian: 0x%08x\n", be32toh(jh.endian));
        printf("  Start: %llu\n", (unsigned long long)start);
        printf("  End: %llu\n", (unsigned long long)end);
        printf("  Size: %llu\n", (unsigned long long)size);
        printf("  Block header size: %u\n", be32toh(jh.blhdrSize));
        printf("  Journal header size: %u\n", be32toh(jh.jhdrSize));
    }
    
    if (size != journal_size) {
        journal_log_error(NULL, "Journal header size doesn't match info block");
        return -1;
    }
    
    if (start > size || end > size) {
        journal_log_error(NULL, "Journal start/end pointers beyond journal size");
        return -1;
    }
    
    /* Validate checksum */
    uint32_t stored_checksum = be32toh(jh.checksum);
    struct HFSPlus_JournalHeader temp_jh = jh;
    temp_jh.checksum = 0;
    uint32_t calculated_checksum = journal_calculate_checksum(&temp_jh, sizeof(temp_jh));
    
    if (stored_checksum != calculated_checksum) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Journal header checksum mismatch: stored=0x%08x, calculated=0x%08x",
                stored_checksum, calculated_checksum);
        journal_log_error(NULL, msg);
        return -1;
    }
    
    if (VERBOSE) {
        printf("Journal validation successful\n");
    }
    
    return 1; /* Valid journal */
}

/*
 * NAME:        journal_replay()
 * DESCRIPTION: Enhanced journal replay with better error handling and validation
 */
int journal_replay(int fd, struct HFSPlus_VolumeHeader *vh, int repair_mode)
{
    uint32_t blockSize = be32toh(vh->blockSize);
    uint32_t jibBlock = be32toh(vh->journalInfoBlock);
    int transactions_replayed = 0;
    
    /* Read Journal Info Block */
    struct HFSPlus_JournalInfoBlock jib;
    off_t jib_offset = (off_t)jibBlock * blockSize;
    
    if (lseek(fd, jib_offset, SEEK_SET) == -1) {
        journal_log_error(NULL, "Failed to seek to Journal Info Block during replay");
        return -EIO;
    }
    
    if (read(fd, &jib, sizeof(jib)) != sizeof(jib)) {
        journal_log_error(NULL, "Failed to read Journal Info Block during replay");
        return -EIO;
    }
    
    uint32_t flags = be32toh(jib.flags);
    
    /* Check for unsupported configurations */
    if (flags & JOURNAL_ON_OTHER_DEVICE) {
        journal_log_error(NULL, "External journal not supported during replay");
        return -EINVAL;
    }
    
    /* Read Journal Header */
    struct HFSPlus_JournalHeader jh;
    uint64_t journal_offset = be64toh(jib.offset);
    
    if (lseek(fd, journal_offset, SEEK_SET) == -1) {
        journal_log_error(NULL, "Failed to seek to journal during replay");
        return -EIO;
    }
    
    if (read(fd, &jh, sizeof(jh)) != sizeof(jh)) {
        journal_log_error(NULL, "Failed to read journal header during replay");
        return -EIO;
    }
    
    /* Validate journal header */
    if (be32toh(jh.magic) != JOURNAL_MAGIC || 
        be32toh(jh.endian) != JOURNAL_ENDIAN) {
        if (repair_mode) {
            /* Mark journal for reinitialization */
            jib.flags = htobe32(be32toh(jib.flags) | JOURNAL_NEED_INIT);
            if (lseek(fd, jib_offset, SEEK_SET) != -1) {
                if (write(fd, &jib, sizeof(jib)) == sizeof(jib)) {
                    journal_log_error(NULL, "Marked journal for reinitialization due to corruption");
                } else {
                    journal_log_error(NULL, "Failed to mark journal for reinitialization");
                }
            }
        }
        journal_log_error(NULL, "Corrupt journal header during replay");
        return -EINVAL;
    }
    
    uint64_t start = be64toh(jh.start);
    uint64_t end = be64toh(jh.end);
    uint64_t journal_size = be64toh(jh.size);
    
    if (VERBOSE) {
        printf("Starting journal replay: start=%llu, end=%llu\n", 
               (unsigned long long)start, (unsigned long long)end);
    }
    
    /* Check if there are transactions to replay */
    if (start == end) {
        if (VERBOSE) {
            printf("Journal is clean, no transactions to replay\n");
        }
        return 0; /* No transactions to replay */
    }
    
    journal_log_error(NULL, "Replaying journal transactions");
    
    uint64_t pos = start;
    int max_transactions = 1000; /* Prevent infinite loops */
    int transaction_count = 0;
    
    /* Replay transactions */
    while (pos != end && transaction_count < max_transactions) {
        /* Read Block List Header */
        struct HFSPlus_BlockListHeader blh;
        uint64_t blh_offset = journal_offset + pos;
        
        if (lseek(fd, blh_offset, SEEK_SET) == -1) {
            journal_log_error(NULL, "Failed to seek to block list during replay");
            return -EIO;
        }
        
        if (read(fd, &blh, sizeof(blh)) != sizeof(blh)) {
            journal_log_error(NULL, "Failed to read block list header during replay");
            return -EIO;
        }
        
        /* Validate block list header */
        uint16_t bsize = be16toh(blh.bsize);
        uint16_t numBlocks = be16toh(blh.numBlocks);
        uint32_t stored_checksum = be32toh(blh.checksum);
        
        if (VERBOSE) {
            printf("Transaction %d: bsize=%u, numBlocks=%u\n", 
                   transaction_count, bsize, numBlocks);
        }
        
        /* Validate checksum */
        struct HFSPlus_BlockListHeader temp_blh = blh;
        temp_blh.checksum = 0;
        uint32_t calculated_checksum = journal_calculate_checksum(&temp_blh, sizeof(temp_blh));
        
        if (calculated_checksum != stored_checksum) {
            if (repair_mode) {
                jib.flags = htobe32(be32toh(jib.flags) | JOURNAL_NEED_INIT);
                if (lseek(fd, jib_offset, SEEK_SET) != -1) {
                    if (write(fd, &jib, sizeof(jib)) == sizeof(jib)) {
                        journal_log_error(NULL, "Marked journal for reinitialization due to checksum error");
                    }
                }
            }
            char msg[128];
            snprintf(msg, sizeof(msg), "Invalid block list checksum: stored=0x%08x, calculated=0x%08x",
                    stored_checksum, calculated_checksum);
            journal_log_error(NULL, msg);
            return -EINVAL;
        }
        
        /* Validate block count */
        if (numBlocks == 0 || numBlocks > 1000) { /* Reasonable limit */
            char msg[64];
            snprintf(msg, sizeof(msg), "Invalid block count in transaction: %u", numBlocks);
            journal_log_error(NULL, msg);
            return -EINVAL;
        }
        
        pos += sizeof(blh);
        
        /* Process each block in the transaction */
        for (uint16_t i = 0; i < numBlocks; i++) {
            struct HFSPlus_BlockInfo bi;
            
            if (lseek(fd, journal_offset + pos, SEEK_SET) == -1) {
                journal_log_error(NULL, "Failed to seek to block info during replay");
                return -EIO;
            }
            
            if (read(fd, &bi, sizeof(bi)) != sizeof(bi)) {
                journal_log_error(NULL, "Failed to read block info during replay");
                return -EIO;
            }
            
            uint32_t block_size = be32toh(bi.bsize);
            uint64_t block_num = be64toh(bi.bnum);
            uint64_t next_pos = be64toh(bi.next);
            
            /* Validate block info */
            if (block_size == 0 || block_size > blockSize * 8) { /* Reasonable limit */
                char msg[64];
                snprintf(msg, sizeof(msg), "Invalid block size in journal: %u", block_size);
                journal_log_error(NULL, msg);
                return -EINVAL;
            }
            
            if (block_num >= be32toh(vh->totalBlocks)) {
                char msg[64];
                snprintf(msg, sizeof(msg), "Block number beyond volume end: %llu", 
                        (unsigned long long)block_num);
                journal_log_error(NULL, msg);
                return -EINVAL;
            }
            
            /* Allocate buffer for block data */
            uint8_t *data = malloc(block_size);
            if (!data) {
                journal_log_error(NULL, "Memory allocation failed during replay");
                return -ENOMEM;
            }
            
            pos += sizeof(bi);
            
            /* Read block data from journal */
            if (lseek(fd, journal_offset + pos, SEEK_SET) == -1) {
                free(data);
                journal_log_error(NULL, "Failed to seek to block data during replay");
                return -EIO;
            }
            
            if (read(fd, data, block_size) != block_size) {
                free(data);
                journal_log_error(NULL, "Failed to read block data during replay");
                return -EIO;
            }
            
            /* Write block data to its location in the volume */
            if (repair_mode) {
                off_t block_offset = (off_t)block_num * blockSize;
                if (lseek(fd, block_offset, SEEK_SET) == -1) {
                    free(data);
                    journal_log_error(NULL, "Failed to seek to volume block during replay");
                    return -EIO;
                }
                
                if (write(fd, data, block_size) != block_size) {
                    free(data);
                    journal_log_error(NULL, "Failed to write volume block during replay");
                    return -EIO;
                }
                
                if (VERBOSE) {
                    printf("  Replayed block %llu (%u bytes)\n", 
                           (unsigned long long)block_num, block_size);
                }
            }
            
            free(data);
            pos = next_pos;
            
            /* Wrap around journal if necessary */
            if (pos >= journal_size) {
                pos = sizeof(struct HFSPlus_JournalHeader);
            }
        }
        
        transactions_replayed++;
        transaction_count++;
    }
    
    if (transaction_count >= max_transactions) {
        journal_log_error(NULL, "Too many transactions, possible corruption");
        return -EINVAL;
    }
    
    /* Update journal header to mark transactions as replayed */
    if (repair_mode && transactions_replayed > 0) {
        jh.start = htobe64(end);
        
        /* Recalculate checksum */
        jh.checksum = 0;
        uint32_t new_checksum = journal_calculate_checksum(&jh, sizeof(jh));
        jh.checksum = htobe32(new_checksum);
        
        if (lseek(fd, journal_offset, SEEK_SET) == -1) {
            journal_log_error(NULL, "Failed to seek to journal header for update");
            return -EIO;
        }
        
        if (write(fd, &jh, sizeof(jh)) != sizeof(jh)) {
            journal_log_error(NULL, "Failed to update journal header");
            return -EIO;
        }
        
        /* Sync to disk */
        if (fsync(fd) == -1) {
            journal_log_error(NULL, "Failed to sync journal updates");
            return -EIO;
        }
    }
    
    if (VERBOSE) {
        printf("Journal replay completed: %d transactions replayed\n", transactions_replayed);
    }
    
    char msg[64];
    snprintf(msg, sizeof(msg), "Journal replay completed successfully: %d transactions", 
             transactions_replayed);
    journal_log_error(NULL, msg);
    
    return transactions_replayed;
}

/*
 * NAME:        journal_disable()
 * DESCRIPTION: Enhanced journal disabling with proper cleanup
 */
int journal_disable(int fd, struct HFSPlus_VolumeHeader *vh)
{
    uint32_t attributes = be32toh(vh->attributes);
    
    if (VERBOSE) {
        printf("Disabling journaling on volume\n");
    }
    
    /* Clear journaling bit */
    attributes &= ~HFSPLUS_VOL_JOURNALED;
    vh->attributes = htobe32(attributes);
    
    /* Clear journal info block */
    vh->journalInfoBlock = 0;
    
    /* Write updated volume header */
    if (lseek(fd, 1024, SEEK_SET) == -1) {
        journal_log_error(NULL, "Failed to seek to volume header for journal disable");
        return -EIO;
    }
    
    if (write(fd, vh, sizeof(*vh)) != sizeof(*vh)) {
        journal_log_error(NULL, "Failed to write volume header for journal disable");
        return -EIO;
    }
    
    /* Write backup volume header */
    uint32_t totalBlocks = be32toh(vh->totalBlocks);
    uint32_t blockSize = be32toh(vh->blockSize);
    off_t backupOffset = (off_t)(totalBlocks - 2) * blockSize + 1024;
    
    if (lseek(fd, backupOffset, SEEK_SET) == -1) {
        journal_log_error(NULL, "Failed to seek to backup volume header for journal disable");
        return -EIO;
    }
    
    if (write(fd, vh, sizeof(*vh)) != sizeof(*vh)) {
        journal_log_error(NULL, "Failed to write backup volume header for journal disable");
        return -EIO;
    }
    
    /* Sync to disk */
    if (fsync(fd) == -1) {
        journal_log_error(NULL, "Failed to sync journal disable changes");
        return -EIO;
    }
    
    journal_log_error(NULL, "Journaling disabled successfully");
    
    if (VERBOSE) {
        printf("Journaling disabled successfully\n");
    }
    
    return 0;
}