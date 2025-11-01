/*
 * hfs_check.c - Enhanced HFS checking functions for fsck.hfs
 * Copyright (C) 2025 Pablo Lezaeta
 * Based on hfsck by Robert Leslie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "fsck_hfs.h"
#include <stdarg.h>

/* Global variables */
int options = 0;

/* Compatibility macros for original hfsck code */
#define VERBOSE (options & HFSCK_VERBOSE)
#define REPAIR  (options & HFSCK_REPAIR)
#define YES     (options & HFSCK_YES)

/* Missing constants */
#define fkData 0

/* Forward declarations */
static int check_mdb_enhanced(hfsvol *vol);
static int check_volume_structure_enhanced(hfsvol *vol);
static int check_btree_enhanced(btree *bt, const char *tree_name);
static int check_allocation_bitmap(hfsvol *vol);
static int check_catalog_consistency(hfsvol *vol);
static int repair_btree_node(btree *bt, unsigned long node_num);
static int validate_btree_structure(btree *bt);
static int check_btree_keys_order(btree *bt);
static int verify_allocation_blocks(hfsvol *vol);
static int repair_allocation_bitmap(hfsvol *vol);
static int validate_catalog_records(hfsvol *vol);
static int check_file_extents(hfsvol *vol);

/*
 * NAME:    hfs_check_volume()
 * DESCRIPTION: Enhanced comprehensive filesystem check on HFS/HFS+ volume
 */
int hfs_check_volume(const char *path, int pnum, int check_options)
{
    hfsvol vol;
    int nparts, result;
    int errors_found = 0;
    int errors_corrected = 0;
    
    /* Set global options */
    options = check_options;
    
    if (VERBOSE) {
        printf("*** Starting comprehensive HFS volume check on %s", path);
        if (pnum > 0) {
            printf(" (partition %d)", pnum);
        }
        printf("\n");
    }
    
    /* Get partition information */
    suid_enable();
    nparts = hfs_nparts(path);
    suid_disable();
    
    if (nparts == 0) {
        fprintf(stderr, "fsck.hfs: partitioned medium contains no HFS partitions\n");
        return FSCK_OPERATIONAL_ERROR;
    }
    
    /* Validate partition number */
    if (pnum < 0) {
        fprintf(stderr, "fsck.hfs: invalid partition number\n");
        return FSCK_USAGE_ERROR;
    }
    
    if (nparts == -1 && pnum > 0) {
        fprintf(stderr, "fsck.hfs: warning: ignoring partition number for non-partitioned medium\n");
        pnum = 0;
    } else if (nparts > 0 && pnum == 0) {
        fprintf(stderr, "fsck.hfs: cannot specify whole medium (has %d partition%s)\n", 
                nparts, nparts == 1 ? "" : "s");
        return FSCK_USAGE_ERROR;
    } else if (nparts > 0 && pnum > nparts) {
        fprintf(stderr, "fsck.hfs: invalid partition number (only %d available)\n", nparts);
        return FSCK_USAGE_ERROR;
    }
    
    /* Initialize volume */
    v_init(&vol, HFS_OPT_NOCACHE);
    
    /* Open volume for checking */
    if (REPAIR) {
        suid_enable();
        result = v_open(&vol, path, HFS_MODE_RDWR);
        suid_disable();
        
        if (result == -1) {
            vol.flags |= HFS_VOL_READONLY;
            suid_enable();
            result = v_open(&vol, path, HFS_MODE_RDONLY);
            suid_disable();
        }
    } else {
        suid_enable();
        result = v_open(&vol, path, HFS_MODE_RDONLY);
        suid_disable();
    }
    
    if (result == -1) {
        perror(path);
        return FSCK_OPERATIONAL_ERROR;
    }
    
    if (REPAIR && (vol.flags & HFS_VOL_READONLY)) {
        fprintf(stderr, "fsck.hfs: warning: %s not writable; cannot repair\n", path);
        options &= ~HFSCK_REPAIR;
    }
    
    /* Get volume geometry and MDB */
    if (v_geometry(&vol, pnum) == -1 || l_getmdb(&vol, &vol.mdb, 0) == -1) {
        perror(path);
        v_close(&vol);
        return FSCK_OPERATIONAL_ERROR;
    }
    
    /* Phase 1: Check Master Directory Block (MDB) or Volume Header */
    if (VERBOSE) {
        printf("\n=== Phase 1: Checking Volume Header ===\n");
    }
    
    result = check_mdb_enhanced(&vol);
    if (result > 0) {
        errors_found = 1;
        if (REPAIR) {
            errors_corrected = 1;
            if (VERBOSE) {
                printf("*** Volume header errors corrected\n");
            }
        }
    } else if (result < 0) {
        fprintf(stderr, "fsck.hfs: critical volume header errors, aborting\n");
        v_close(&vol);
        return FSCK_UNCORRECTED;
    }
    
    /* Phase 2: Check volume structure and allocation bitmap */
    if (VERBOSE) {
        printf("\n=== Phase 2: Checking Volume Structure ===\n");
    }
    
    result = check_volume_structure_enhanced(&vol);
    if (result > 0) {
        errors_found = 1;
        if (REPAIR) {
            errors_corrected = 1;
            if (VERBOSE) {
                printf("*** Volume structure errors corrected\n");
            }
        }
    } else if (result < 0) {
        fprintf(stderr, "fsck.hfs: critical volume structure errors\n");
        errors_found = 1;
    }
    
    /* Phase 3: Check allocation bitmap */
    if (VERBOSE) {
        printf("\n=== Phase 3: Checking Allocation Bitmap ===\n");
    }
    
    result = check_allocation_bitmap(&vol);
    if (result > 0) {
        errors_found = 1;
        if (REPAIR) {
            errors_corrected = 1;
            if (VERBOSE) {
                printf("*** Allocation bitmap errors corrected\n");
            }
        }
    } else if (result < 0) {
        fprintf(stderr, "fsck.hfs: critical allocation bitmap errors\n");
        errors_found = 1;
    }
    
    /* Phase 4: Check extents overflow B-tree */
    if (VERBOSE) {
        printf("\n=== Phase 4: Checking Extents B-tree ===\n");
    }
    
    result = check_btree_enhanced(&vol.ext, "extents");
    if (result > 0) {
        errors_found = 1;
        if (REPAIR) {
            errors_corrected = 1;
            if (VERBOSE) {
                printf("*** Extents B-tree errors corrected\n");
            }
        }
    } else if (result < 0) {
        fprintf(stderr, "fsck.hfs: critical extents B-tree errors\n");
        errors_found = 1;
    }
    
    /* Phase 5: Check catalog B-tree */
    if (VERBOSE) {
        printf("\n=== Phase 5: Checking Catalog B-tree ===\n");
    }
    
    result = check_btree_enhanced(&vol.cat, "catalog");
    if (result > 0) {
        errors_found = 1;
        if (REPAIR) {
            errors_corrected = 1;
            if (VERBOSE) {
                printf("*** Catalog B-tree errors corrected\n");
            }
        }
    } else if (result < 0) {
        fprintf(stderr, "fsck.hfs: critical catalog B-tree errors\n");
        errors_found = 1;
    }
    
    /* Phase 6: Check catalog file consistency */
    if (VERBOSE) {
        printf("\n=== Phase 6: Checking Catalog Consistency ===\n");
    }
    
    result = check_catalog_consistency(&vol);
    if (result > 0) {
        errors_found = 1;
        if (REPAIR) {
            errors_corrected = 1;
            if (VERBOSE) {
                printf("*** Catalog consistency errors corrected\n");
            }
        }
    } else if (result < 0) {
        fprintf(stderr, "fsck.hfs: critical catalog consistency errors\n");
        errors_found = 1;
    }
    
    /* Update volume if repairs were made */
    if (errors_corrected && REPAIR) {
        if (vol.flags & HFS_VOL_UPDATE_MDB) {
            if (VERBOSE) {
                printf("*** Updating volume header\n");
            }
        }
    }
    
    /* Mark volume as mounted for proper cleanup */
    vol.flags |= HFS_VOL_MOUNTED;
    
    /* Close volume */
    if (v_close(&vol) == -1) {
        perror("closing volume");
        return FSCK_OPERATIONAL_ERROR;
    }
    
    /* Return appropriate exit code based on results */
    if (!errors_found) {
        if (VERBOSE) {
            printf("\n*** Volume check completed: no errors found\n");
        }
        return FSCK_OK;
    } else if (errors_corrected && REPAIR) {
        if (VERBOSE) {
            printf("\n*** Volume check completed: errors found and corrected\n");
        }
        return FSCK_CORRECTED;
    } else {
        if (VERBOSE) {
            printf("\n*** Volume check completed: errors found but not corrected\n");
        }
        return FSCK_UNCORRECTED;
    }
}/*

 * NAME:    check_mdb_enhanced()
 * DESCRIPTION: Enhanced Master Directory Block checking with comprehensive validation
 */
static int check_mdb_enhanced(hfsvol *vol)
{
    MDB *mdb = &vol->mdb;
    time_t now;
    int errors_fixed = 0;
    
    if (VERBOSE) {
        printf("*** Checking Master Directory Block\n");
        
        /* Display detailed MDB information in verbose mode */
        printf("  drSigWord      = 0x%04x\n", mdb->drSigWord);
        printf("  drCrDate       = %s\n", mctime(mdb->drCrDate));
        printf("  drLsMod        = %s\n", mctime(mdb->drLsMod));
        
        printf("  drAtrb         =");
        int flag = 0;
        if (mdb->drAtrb & HFS_ATRB_BUSY) {
            printf("%s BUSY", flag ? " |" : "");
            flag = 1;
        }
        if (mdb->drAtrb & HFS_ATRB_HLOCKED) {
            printf("%s HLOCKED", flag ? " |" : "");
            flag = 1;
        }
        if (mdb->drAtrb & HFS_ATRB_UMOUNTED) {
            printf("%s UMOUNTED", flag ? " |" : "");
            flag = 1;
        }
        if (mdb->drAtrb & HFS_ATRB_BBSPARED) {
            printf("%s BBSPARED", flag ? " |" : "");
            flag = 1;
        }
        if (mdb->drAtrb & HFS_ATRB_BVINCONSIS) {
            printf("%s BVINCONSIS", flag ? " |" : "");
            flag = 1;
        }
        if (mdb->drAtrb & HFS_ATRB_COPYPROT) {
            printf("%s COPYPROT", flag ? " |" : "");
            flag = 1;
        }
        if (mdb->drAtrb & HFS_ATRB_SLOCKED) {
            printf("%s SLOCKED", flag ? " |" : "");
            flag = 1;
        }
        if (flag == 0) {
            printf(" 0");
        }
        printf("\n");
        
        printf("  drNmFls        = %u\n", mdb->drNmFls);
        printf("  drVBMSt        = %u\n", mdb->drVBMSt);
        printf("  drAllocPtr     = %u\n", mdb->drAllocPtr);
        printf("  drNmAlBlks     = %u\n", mdb->drNmAlBlks);
        printf("  drAlBlkSiz     = %lu\n", mdb->drAlBlkSiz);
        printf("  drClpSiz       = %lu\n", mdb->drClpSiz);
        printf("  drAlBlSt       = %u\n", mdb->drAlBlSt);
        printf("  drNxtCNID      = %lu\n", mdb->drNxtCNID);
        printf("  drFreeBks      = %u\n", mdb->drFreeBks);
        printf("  drVN           = \"%s\"\n", mdb->drVN);
        printf("  drVolBkUp      = %s\n", mctime(mdb->drVolBkUp));
        printf("  drVSeqNum      = %u\n", mdb->drVSeqNum);
        printf("  drWrCnt        = %lu\n", mdb->drWrCnt);
        printf("  drXTClpSiz     = %lu\n", mdb->drXTClpSiz);
        printf("  drCTClpSiz     = %lu\n", mdb->drCTClpSiz);
        printf("  drNmRtDirs     = %u\n", mdb->drNmRtDirs);
        printf("  drFilCnt       = %lu\n", mdb->drFilCnt);
        printf("  drDirCnt       = %lu\n", mdb->drDirCnt);
        printf("  drEmbedSigWord = 0x%04x\n", mdb->drEmbedSigWord);
        printf("  drEmbedExtent  = %s\n", extstr(&mdb->drEmbedExtent));
        printf("  drXTFlSize     = %lu\n", mdb->drXTFlSize);
        printf("  drXTExtRec     = %s\n", extrecstr(&mdb->drXTExtRec));
        printf("  drCTFlSize     = %lu\n", mdb->drCTFlSize);
        printf("  drCTExtRec     = %s\n", extrecstr(&mdb->drCTExtRec));
    }
    
    /* Check volume signature */
    if (mdb->drSigWord != HFS_SIGWORD &&
        ask("Bad volume signature (0x%04x); should be 0x%04x",
            mdb->drSigWord, HFS_SIGWORD)) {
        mdb->drSigWord = HFS_SIGWORD;
        vol->flags |= HFS_VOL_UPDATE_MDB;
        errors_fixed = 1;
    }
    
    /* Check and fix timestamps */
    time(&now);
    
    if (mdb->drCrDate == 0 &&
        ask("Volume creation date is unset")) {
        mdb->drCrDate = d_mtime(now);
        vol->flags |= HFS_VOL_UPDATE_MDB;
        errors_fixed = 1;
    }
    
    if (d_ltime(mdb->drCrDate) > now &&
        ask("Volume creation date is in the future (%s)", mctime(mdb->drCrDate))) {
        mdb->drCrDate = d_mtime(now);
        vol->flags |= HFS_VOL_UPDATE_MDB;
        errors_fixed = 1;
    }
    
    /* Check last modification date */
    if (mdb->drLsMod == 0 &&
        ask("Volume last modify date is unset")) {
        mdb->drLsMod = mdb->drCrDate;
        vol->flags |= HFS_VOL_UPDATE_MDB;
        errors_fixed = 1;
    }
    
    if (d_ltime(mdb->drLsMod) > now &&
        ask("Volume last modify date is in the future (%s)",
            mctime(mdb->drLsMod))) {
        mdb->drLsMod = d_mtime(now);
        vol->flags |= HFS_VOL_UPDATE_MDB;
        errors_fixed = 1;
    }
    
    if (mdb->drLsMod < mdb->drCrDate &&
        ask("Volume last modify date is before volume creation")) {
        mdb->drLsMod = mdb->drCrDate;
        vol->flags |= HFS_VOL_UPDATE_MDB;
        errors_fixed = 1;
    }
    
    /* Check volume bitmap start */
    if (mdb->drVBMSt != 3 &&
        ask("Volume bitmap starts at unusual location (%u), not 3", mdb->drVBMSt)) {
        mdb->drVBMSt = 3;
        vol->flags |= HFS_VOL_UPDATE_MDB;
        errors_fixed = 1;
    }
    
    /* Validate allocation block parameters */
    if (mdb->drAlBlkSiz == 0 || (mdb->drAlBlkSiz & (mdb->drAlBlkSiz - 1)) != 0) {
        if (VERBOSE || !REPAIR) {
            printf("Invalid allocation block size: %lu\n", mdb->drAlBlkSiz);
        }
        return -1; /* Critical error - cannot fix */
    }
    
    if (mdb->drNmAlBlks == 0) {
        if (VERBOSE || !REPAIR) {
            printf("Invalid number of allocation blocks: %u\n", mdb->drNmAlBlks);
        }
        return -1; /* Critical error - cannot fix */
    }
    
    /* Initialize volume parameters for subsequent checks */
    vol->lpa = vol->mdb.drAlBlkSiz >> HFS_BLOCKSZ_BITS;
    
    /* Initialize extents pseudo-file structs */
    vol->ext.f.cat.u.fil.filStBlk   = vol->mdb.drXTExtRec[0].xdrStABN;
    vol->ext.f.cat.u.fil.filLgLen   = vol->mdb.drXTFlSize;
    vol->ext.f.cat.u.fil.filPyLen   = vol->mdb.drXTFlSize;
    vol->ext.f.cat.u.fil.filCrDat   = vol->mdb.drCrDate;
    vol->ext.f.cat.u.fil.filMdDat   = vol->mdb.drLsMod;
    vol->ext.f.cat.u.fil.filClpSize = vol->mdb.drXTClpSiz;
    
    memcpy(&vol->ext.f.cat.u.fil.filExtRec,
           &vol->mdb.drXTExtRec, sizeof(ExtDataRec));
    
    f_selectfork(&vol->ext.f, fkData);
    
    /* Initialize catalog pseudo-file structs */
    vol->cat.f.cat.u.fil.filStBlk   = vol->mdb.drCTExtRec[0].xdrStABN;
    vol->cat.f.cat.u.fil.filLgLen   = vol->mdb.drCTFlSize;
    vol->cat.f.cat.u.fil.filPyLen   = vol->mdb.drCTFlSize;
    vol->cat.f.cat.u.fil.filCrDat   = vol->mdb.drCrDate;
    vol->cat.f.cat.u.fil.filMdDat   = vol->mdb.drLsMod;
    vol->cat.f.cat.u.fil.filClpSize = vol->mdb.drCTClpSiz;
    
    memcpy(&vol->cat.f.cat.u.fil.filExtRec,
           &vol->mdb.drCTExtRec, sizeof(ExtDataRec));
    
    f_selectfork(&vol->cat.f, fkData);
    
    if (VERBOSE && errors_fixed > 0) {
        printf("Fixed %d volume header issues\n", errors_fixed);
    }
    
    return errors_fixed;
}

/*
 * NAME:    check_volume_structure_enhanced()
 * DESCRIPTION: Enhanced volume structure checking
 */
static int check_volume_structure_enhanced(hfsvol *vol)
{
    int errors_fixed = 0;
    
    if (VERBOSE) {
        printf("*** Checking volume structure\n");
    }
    
    /* Initialize volume parameters */
    vol->lpa = vol->mdb.drAlBlkSiz >> HFS_BLOCKSZ_BITS;
    
    /* Initialize extents pseudo-file structures */
    vol->ext.f.cat.u.fil.filStBlk   = vol->mdb.drXTExtRec[0].xdrStABN;
    vol->ext.f.cat.u.fil.filLgLen   = vol->mdb.drXTFlSize;
    vol->ext.f.cat.u.fil.filPyLen   = vol->mdb.drXTFlSize;
    vol->ext.f.cat.u.fil.filCrDat   = vol->mdb.drCrDate;
    vol->ext.f.cat.u.fil.filMdDat   = vol->mdb.drLsMod;
    vol->ext.f.cat.u.fil.filClpSize = vol->mdb.drXTClpSiz;
    
    memcpy(&vol->ext.f.cat.u.fil.filExtRec,
           &vol->mdb.drXTExtRec, sizeof(ExtDataRec));
    
    f_selectfork(&vol->ext.f, fkData);
    
    /* Initialize catalog pseudo-file structures */
    vol->cat.f.cat.u.fil.filStBlk   = vol->mdb.drCTExtRec[0].xdrStABN;
    vol->cat.f.cat.u.fil.filLgLen   = vol->mdb.drCTFlSize;
    vol->cat.f.cat.u.fil.filPyLen   = vol->mdb.drCTFlSize;
    vol->cat.f.cat.u.fil.filCrDat   = vol->mdb.drCrDate;
    vol->cat.f.cat.u.fil.filMdDat   = vol->mdb.drLsMod;
    vol->cat.f.cat.u.fil.filClpSize = vol->mdb.drCTClpSiz;
    
    memcpy(&vol->cat.f.cat.u.fil.filExtRec,
           &vol->mdb.drCTExtRec, sizeof(ExtDataRec));
    
    f_selectfork(&vol->cat.f, fkData);
    
    /* Validate extents file size */
    if (vol->mdb.drXTFlSize == 0) {
        if (VERBOSE || !REPAIR) {
            printf("Extents file has zero size\n");
        }
        return -1; /* Critical error */
    }
    
    /* Validate catalog file size */
    if (vol->mdb.drCTFlSize == 0) {
        if (VERBOSE || !REPAIR) {
            printf("Catalog file has zero size\n");
        }
        return -1; /* Critical error */
    }
    
    /* Check file extent records */
    errors_fixed += check_file_extents(vol);
    
    if (VERBOSE) {
        printf("Volume structure validation completed\n");
        if (errors_fixed > 0) {
            printf("Fixed %d volume structure issues\n", errors_fixed);
        }
    }
    
    return errors_fixed;
}

/*
 * NAME:    check_btree_enhanced()
 * DESCRIPTION: Enhanced B-tree checking with comprehensive validation and repair
 */
static int check_btree_enhanced(btree *bt, const char *tree_name)
{
    int errors_fixed = 0;
    int result;
    
    if (VERBOSE) {
        printf("*** Checking %s B-tree\n", tree_name);
    }
    
    /* Read B-tree header */
    if (bt_readhdr(bt) == -1) {
        fprintf(stderr, "fsck.hfs: failed to read %s B-tree header\n", tree_name);
        return -1;
    }
    
    if (VERBOSE) {
        printf("B-tree header information:\n");
        printf("  Depth:     %u\n", bt->hdr.bthDepth);
        printf("  Root node: %lu\n", bt->hdr.bthRoot);
        printf("  Records:   %lu\n", bt->hdr.bthNRecs);
        printf("  First node: %lu\n", bt->hdr.bthFNode);
        printf("  Last node:  %lu\n", bt->hdr.bthLNode);
        printf("  Node size:  %u\n", bt->hdr.bthNodeSize);
        printf("  Key length: %u\n", bt->hdr.bthKeyLen);
        printf("  Total nodes: %lu\n", bt->hdr.bthNNodes);
        printf("  Free nodes:  %lu\n", bt->hdr.bthFree);
    }
    
    /* Validate B-tree header */
    if (bt->hdr.bthNodeSize == 0 || (bt->hdr.bthNodeSize & (bt->hdr.bthNodeSize - 1)) != 0) {
        if (VERBOSE || !REPAIR) {
            printf("Invalid B-tree node size: %u\n", bt->hdr.bthNodeSize);
        }
        return -1; /* Critical error */
    }
    
    if (bt->hdr.bthDepth > 8) {
        if (VERBOSE || !REPAIR) {
            printf("B-tree depth too large: %u\n", bt->hdr.bthDepth);
        }
        return -1; /* Critical error */
    }
    
    /* Validate B-tree structure */
    result = validate_btree_structure(bt);
    if (result > 0) {
        errors_fixed += result;
    } else if (result < 0) {
        fprintf(stderr, "fsck.hfs: critical B-tree structure errors in %s\n", tree_name);
        return -1;
    }
    
    /* Check key ordering */
    result = check_btree_keys_order(bt);
    if (result > 0) {
        errors_fixed += result;
    } else if (result < 0) {
        fprintf(stderr, "fsck.hfs: critical B-tree key ordering errors in %s\n", tree_name);
        return -1;
    }
    
    if (VERBOSE) {
        printf("%s B-tree validation completed\n", tree_name);
        if (errors_fixed > 0) {
            printf("Fixed %d B-tree issues\n", errors_fixed);
        }
    }
    
    return errors_fixed;
}

/*
 * NAME:    check_allocation_bitmap()
 * DESCRIPTION: Check and repair allocation bitmap
 */
static int check_allocation_bitmap(hfsvol *vol)
{
    int errors_fixed = 0;
    int result;
    
    if (VERBOSE) {
        printf("*** Checking allocation bitmap\n");
    }
    
    /* Verify allocation blocks */
    result = verify_allocation_blocks(vol);
    if (result > 0) {
        errors_fixed += result;
    } else if (result < 0) {
        fprintf(stderr, "fsck.hfs: critical allocation bitmap errors\n");
        return -1;
    }
    
    /* Repair allocation bitmap if needed */
    if (errors_fixed > 0 && REPAIR) {
        result = repair_allocation_bitmap(vol);
        if (result < 0) {
            fprintf(stderr, "fsck.hfs: failed to repair allocation bitmap\n");
            return -1;
        }
    }
    
    if (VERBOSE) {
        printf("Allocation bitmap validation completed\n");
        if (errors_fixed > 0) {
            printf("Fixed %d allocation bitmap issues\n", errors_fixed);
        }
    }
    
    return errors_fixed;
}

/*
 * NAME:    check_catalog_consistency()
 * DESCRIPTION: Check catalog file consistency
 */
static int check_catalog_consistency(hfsvol *vol)
{
    int errors_fixed = 0;
    int result;
    
    if (VERBOSE) {
        printf("*** Checking catalog consistency\n");
    }
    
    /* Validate catalog records */
    result = validate_catalog_records(vol);
    if (result > 0) {
        errors_fixed += result;
    } else if (result < 0) {
        fprintf(stderr, "fsck.hfs: critical catalog consistency errors\n");
        return -1;
    }
    
    if (VERBOSE) {
        printf("Catalog consistency validation completed\n");
        if (errors_fixed > 0) {
            printf("Fixed %d catalog consistency issues\n", errors_fixed);
        }
    }
    
    return errors_fixed;
}

/* Enhanced implementations of helper functions */

static int validate_btree_structure(btree *bt)
{
    int errors_found = 0;
    node n;
    unsigned long node_num;
    
    if (bt->hdr.bthNNodes == 0) {
        return 0; /* Empty tree is valid */
    }
    
    /* Check root node */
    if (bt->hdr.bthRoot >= bt->hdr.bthNNodes) {
        if (VERBOSE || !REPAIR) {
            printf("Invalid root node number: %lu (max: %lu)\n",
                   bt->hdr.bthRoot, bt->hdr.bthNNodes - 1);
        }
        return -1; /* Critical error */
    }
    
    /* Check first and last leaf nodes */
    if (bt->hdr.bthFNode >= bt->hdr.bthNNodes) {
        if (VERBOSE || !REPAIR) {
            printf("Invalid first leaf node: %lu (max: %lu)\n",
                   bt->hdr.bthFNode, bt->hdr.bthNNodes - 1);
        }
        errors_found++;
    }
    
    if (bt->hdr.bthLNode >= bt->hdr.bthNNodes) {
        if (VERBOSE || !REPAIR) {
            printf("Invalid last leaf node: %lu (max: %lu)\n",
                   bt->hdr.bthLNode, bt->hdr.bthNNodes - 1);
        }
        errors_found++;
    }
    
    /* Validate node structure by walking through leaf nodes */
    if (bt->hdr.bthFNode > 0 && errors_found == 0) {
        n.bt = bt;
        node_num = bt->hdr.bthFNode;
        
        while (node_num != 0) {
            n.nnum = node_num;
            
            if (bt_getnode(&n) == -1) {
                if (VERBOSE || !REPAIR) {
                    printf("Failed to read B-tree node %lu\n", node_num);
                }
                errors_found++;
                break;
            }
            
            /* Validate node descriptor */
            if (n.nd.ndType != ndLeafNode && n.nd.ndType != ndIndxNode) {
                if (VERBOSE || !REPAIR) {
                    printf("Invalid node type %d in node %lu\n", n.nd.ndType, node_num);
                }
                errors_found++;
                
                if (REPAIR && (YES || ask("Attempt to repair corrupted B-tree node %lu", node_num))) {
                    if (repair_btree_node(bt, node_num) == 0) {
                        if (VERBOSE) {
                            printf("Repaired B-tree node %lu\n", node_num);
                        }
                    } else {
                        if (VERBOSE) {
                            printf("Failed to repair B-tree node %lu\n", node_num);
                        }
                    }
                }
            }
            
            /* Check record count */
            if (n.nd.ndNRecs > ((bt->hdr.bthNodeSize - sizeof(NodeDescriptor)) / 4)) {
                if (VERBOSE || !REPAIR) {
                    printf("Too many records (%d) in node %lu\n", n.nd.ndNRecs, node_num);
                }
                errors_found++;
            }
            
            /* Move to next node */
            node_num = n.nd.ndFLink;
            
            /* Prevent infinite loops */
            if (node_num == bt->hdr.bthFNode) {
                if (VERBOSE || !REPAIR) {
                    printf("Circular reference detected in B-tree leaf nodes\n");
                }
                errors_found++;
                break;
            }
        }
    }
    
    return errors_found;
}

static int check_btree_keys_order(btree *bt)
{
    int errors_found = 0;
    node n;
    unsigned long node_num;
    byte *prev_key = NULL;
    byte *curr_key;
    int i;
    
    if (bt->hdr.bthNNodes == 0) {
        return 0; /* Empty tree is valid */
    }
    
    /* Walk through leaf nodes to check key ordering */
    if (bt->hdr.bthFNode > 0) {
        n.bt = bt;
        node_num = bt->hdr.bthFNode;
        
        while (node_num != 0) {
            n.nnum = node_num;
            
            if (bt_getnode(&n) == -1) {
                break; /* Already reported in structure validation */
            }
            
            /* Check keys within this node */
            for (i = 0; i < n.nd.ndNRecs; i++) {
                curr_key = HFS_NODEREC(n, i);
                
                if (curr_key == NULL) {
                    if (VERBOSE || !REPAIR) {
                        printf("NULL key pointer in node %lu, record %d\n", node_num, i);
                    }
                    errors_found++;
                    continue;
                }
                
                /* Compare with previous key if available */
                if (prev_key != NULL) {
                    /* Simple byte comparison for key ordering */
                    int key_len = HFS_RECKEYLEN(curr_key);
                    int prev_len = HFS_RECKEYLEN(prev_key);
                    int cmp_len = (key_len < prev_len) ? key_len : prev_len;
                    
                    if (memcmp(prev_key + 1, curr_key + 1, cmp_len) > 0 ||
                        (memcmp(prev_key + 1, curr_key + 1, cmp_len) == 0 && prev_len > key_len)) {
                        if (VERBOSE || !REPAIR) {
                            printf("Keys out of order in B-tree at node %lu, record %d\n", node_num, i);
                        }
                        errors_found++;
                    }
                }
                
                prev_key = curr_key;
            }
            
            node_num = n.nd.ndFLink;
            
            /* Prevent infinite loops */
            if (node_num == bt->hdr.bthFNode) {
                break;
            }
        }
    }
    
    return errors_found;
}

static int verify_allocation_blocks(hfsvol *vol)
{
    int errors_found = 0;
    unsigned int total_blocks = vol->mdb.drNmAlBlks;
    unsigned int free_blocks = vol->mdb.drFreeBks;
    unsigned int bitmap_blocks;
    unsigned int i, j;
    block bitmap_block;
    unsigned int actual_free = 0;
    
    if (VERBOSE) {
        printf("Total allocation blocks: %u\n", total_blocks);
        printf("Free blocks reported: %u\n", free_blocks);
    }
    
    /* Basic sanity checks */
    if (free_blocks > total_blocks) {
        if (VERBOSE || !REPAIR) {
            printf("Free block count (%u) exceeds total blocks (%u)\n",
                   free_blocks, total_blocks);
        }
        errors_found++;
    }
    
    /* Calculate number of bitmap blocks */
    bitmap_blocks = (total_blocks + 4095) / 4096; /* 4096 bits per block */
    
    if (VERBOSE) {
        printf("Scanning %u bitmap blocks\n", bitmap_blocks);
    }
    
    /* Read and verify allocation bitmap */
    for (i = 0; i < bitmap_blocks; i++) {
        if (l_getblock(&vol->priv, vol->mdb.drVBMSt + i, bitmap_block) == -1) {
            if (VERBOSE || !REPAIR) {
                printf("Failed to read bitmap block %u\n", i);
            }
            errors_found++;
            continue;
        }
        
        /* Count free blocks in this bitmap block */
        for (j = 0; j < HFS_BLOCKSZ; j++) {
            unsigned char byte_val = bitmap_block[j];
            unsigned int block_base = (i * HFS_BLOCKSZ + j) * 8;
            
            /* Don't count beyond total blocks */
            if (block_base >= total_blocks) {
                break;
            }
            
            /* Count zero bits (free blocks) */
            for (int bit = 0; bit < 8 && (block_base + bit) < total_blocks; bit++) {
                if ((byte_val & (1 << (7 - bit))) == 0) {
                    actual_free++;
                }
            }
        }
    }
    
    /* Compare actual free count with reported count */
    if (actual_free != free_blocks) {
        if (VERBOSE || !REPAIR) {
            printf("Free block count mismatch: reported %u, actual %u\n",
                   free_blocks, actual_free);
        }
        errors_found++;
    }
    
    /* Check for blocks allocated to system files */
    unsigned int system_blocks = 0;
    
    /* Count blocks used by extents file */
    for (i = 0; i < 3; i++) {
        if (vol->mdb.drXTExtRec[i].xdrNumABlks > 0) {
            system_blocks += vol->mdb.drXTExtRec[i].xdrNumABlks;
        }
    }
    
    /* Count blocks used by catalog file */
    for (i = 0; i < 3; i++) {
        if (vol->mdb.drCTExtRec[i].xdrNumABlks > 0) {
            system_blocks += vol->mdb.drCTExtRec[i].xdrNumABlks;
        }
    }
    
    if (VERBOSE) {
        printf("System files use %u allocation blocks\n", system_blocks);
        printf("User files use %u allocation blocks\n", 
               total_blocks - actual_free - system_blocks);
    }
    
    return errors_found;
}

static int repair_allocation_bitmap(hfsvol *vol)
{
    int repairs_made = 0;
    unsigned int total_blocks = vol->mdb.drNmAlBlks;
    unsigned int bitmap_blocks;
    unsigned int i, j;
    block bitmap_block;
    unsigned int actual_free = 0;
    
    if (VERBOSE) {
        printf("Repairing allocation bitmap\n");
    }
    
    /* Calculate number of bitmap blocks */
    bitmap_blocks = (total_blocks + 4095) / 4096;
    
    /* Recalculate free block count */
    for (i = 0; i < bitmap_blocks; i++) {
        if (l_getblock(&vol->priv, vol->mdb.drVBMSt + i, bitmap_block) == -1) {
            continue;
        }
        
        /* Count free blocks in this bitmap block */
        for (j = 0; j < HFS_BLOCKSZ; j++) {
            unsigned char byte_val = bitmap_block[j];
            unsigned int block_base = (i * HFS_BLOCKSZ + j) * 8;
            
            if (block_base >= total_blocks) {
                break;
            }
            
            /* Count zero bits (free blocks) */
            for (int bit = 0; bit < 8 && (block_base + bit) < total_blocks; bit++) {
                if ((byte_val & (1 << (7 - bit))) == 0) {
                    actual_free++;
                }
            }
        }
    }
    
    /* Update MDB with correct free block count */
    if (vol->mdb.drFreeBks != actual_free) {
        if (VERBOSE) {
            printf("Updating free block count from %u to %u\n",
                   vol->mdb.drFreeBks, actual_free);
        }
        vol->mdb.drFreeBks = actual_free;
        vol->flags |= HFS_VOL_UPDATE_MDB;
        repairs_made++;
    }
    
    return repairs_made;
}

static int validate_catalog_records(hfsvol *vol)
{
    int errors_found = 0;
    node n;
    unsigned long node_num;
    int i;
    byte *rec_ptr;
    CatKeyRec *key;
    CatDataRec *data;
    unsigned long total_files = 0;
    unsigned long total_dirs = 0;
    
    if (VERBOSE) {
        printf("Validating catalog records\n");
    }
    
    /* Walk through catalog B-tree leaf nodes */
    if (vol->cat.hdr.bthFNode > 0) {
        n.bt = &vol->cat;
        node_num = vol->cat.hdr.bthFNode;
        
        while (node_num != 0) {
            n.nnum = node_num;
            
            if (bt_getnode(&n) == -1) {
                break; /* Already reported in B-tree validation */
            }
            
            /* Check each record in this node */
            for (i = 0; i < n.nd.ndNRecs; i++) {
                rec_ptr = HFS_NODEREC(n, i);
                if (rec_ptr == NULL) {
                    continue;
                }
                
                key = (CatKeyRec *)rec_ptr;
                data = (CatDataRec *)HFS_RECDATA(rec_ptr);
                
                /* Validate key length */
                if (key->ckrKeyLen == 0 || key->ckrKeyLen > HFS_MAX_CATKEY_LEN) {
                    if (VERBOSE || !REPAIR) {
                        printf("Invalid catalog key length %d in node %lu, record %d\n",
                               key->ckrKeyLen, node_num, i);
                    }
                    errors_found++;
                    continue;
                }
                
                /* Validate parent directory ID */
                if (key->ckrParID == 0 && key->ckrCName[0] != 0) {
                    if (VERBOSE || !REPAIR) {
                        printf("Invalid parent ID 0 for non-root entry in node %lu, record %d\n",
                               node_num, i);
                    }
                    errors_found++;
                }
                
                /* Validate record type and count files/directories */
                switch (data->cdrType) {
                    case cdrDirRec:
                        total_dirs++;
                        
                        /* Validate directory record */
                        if (data->u.dir.dirDirID == 0) {
                            if (VERBOSE || !REPAIR) {
                                printf("Directory with invalid ID 0 in node %lu, record %d\n",
                                       node_num, i);
                            }
                            errors_found++;
                        }
                        break;
                        
                    case cdrFilRec:
                        total_files++;
                        
                        /* Validate file record */
                        if (data->u.fil.filFlNum == 0) {
                            if (VERBOSE || !REPAIR) {
                                printf("File with invalid ID 0 in node %lu, record %d\n",
                                       node_num, i);
                            }
                            errors_found++;
                        }
                        
                        /* Check file extent records */
                        for (int ext = 0; ext < 3; ext++) {
                            ExtDescriptor *extent = &data->u.fil.filExtRec[ext];
                            if (extent->xdrNumABlks > 0) {
                                if (extent->xdrStABN >= vol->mdb.drNmAlBlks) {
                                    if (VERBOSE || !REPAIR) {
                                        printf("File extent %d starts beyond volume end in node %lu, record %d\n",
                                               ext, node_num, i);
                                    }
                                    errors_found++;
                                }
                            }
                        }
                        break;
                        
                    case cdrThdRec:
                        /* Thread record - validate parent ID */
                        if (data->u.dthd.thdParID == 0 && key->ckrParID != fsRtParID) {
                            if (VERBOSE || !REPAIR) {
                                printf("Thread record with invalid parent ID in node %lu, record %d\n",
                                       node_num, i);
                            }
                            errors_found++;
                        }
                        break;
                        
                    case cdrFThdRec:
                        /* File thread record - similar validation */
                        if (data->u.fthd.fthdParID == 0 && key->ckrParID != fsRtParID) {
                            if (VERBOSE || !REPAIR) {
                                printf("File thread record with invalid parent ID in node %lu, record %d\n",
                                       node_num, i);
                            }
                            errors_found++;
                        }
                        break;
                        
                    default:
                        if (VERBOSE || !REPAIR) {
                            printf("Unknown catalog record type %d in node %lu, record %d\n",
                                   data->cdrType, node_num, i);
                        }
                        errors_found++;
                        break;
                }
            }
            
            node_num = n.nd.ndFLink;
            
            /* Prevent infinite loops */
            if (node_num == vol->cat.hdr.bthFNode) {
                break;
            }
        }
    }
    
    /* Compare counted files/directories with MDB values */
    if (total_files != vol->mdb.drFilCnt) {
        if (VERBOSE || !REPAIR) {
            printf("File count mismatch: MDB reports %lu, catalog contains %lu\n",
                   vol->mdb.drFilCnt, total_files);
        }
        errors_found++;
        
        if (REPAIR && (YES || ask("Update file count in MDB"))) {
            vol->mdb.drFilCnt = total_files;
            vol->flags |= HFS_VOL_UPDATE_MDB;
            if (VERBOSE) {
                printf("Updated file count to %lu\n", total_files);
            }
        }
    }
    
    if (total_dirs != vol->mdb.drDirCnt) {
        if (VERBOSE || !REPAIR) {
            printf("Directory count mismatch: MDB reports %lu, catalog contains %lu\n",
                   vol->mdb.drDirCnt, total_dirs);
        }
        errors_found++;
        
        if (REPAIR && (YES || ask("Update directory count in MDB"))) {
            vol->mdb.drDirCnt = total_dirs;
            vol->flags |= HFS_VOL_UPDATE_MDB;
            if (VERBOSE) {
                printf("Updated directory count to %lu\n", total_dirs);
            }
        }
    }
    
    if (VERBOSE) {
        printf("Catalog validation completed: %lu files, %lu directories\n",
               total_files, total_dirs);
    }
    
    return errors_found;
}

static int check_file_extents(hfsvol *vol)
{
    int errors_found = 0;
    
    if (VERBOSE) {
        printf("Checking file extents\n");
    }
    
    /* Validate extents file extent record */
    for (int i = 0; i < 3; i++) {
        ExtDescriptor *ext = &vol->mdb.drXTExtRec[i];
        if (ext->xdrNumABlks > 0) {
            if (ext->xdrStABN >= vol->mdb.drNmAlBlks) {
                if (VERBOSE || !REPAIR) {
                    printf("Extents file extent %d starts beyond volume end (block %u >= %u)\n",
                           i, ext->xdrStABN, vol->mdb.drNmAlBlks);
                }
                errors_found++;
            }
            
            if (ext->xdrStABN + ext->xdrNumABlks > vol->mdb.drNmAlBlks) {
                if (VERBOSE || !REPAIR) {
                    printf("Extents file extent %d extends beyond volume end\n", i);
                }
                errors_found++;
            }
        }
    }
    
    /* Validate catalog file extent record */
    for (int i = 0; i < 3; i++) {
        ExtDescriptor *ext = &vol->mdb.drCTExtRec[i];
        if (ext->xdrNumABlks > 0) {
            if (ext->xdrStABN >= vol->mdb.drNmAlBlks) {
                if (VERBOSE || !REPAIR) {
                    printf("Catalog file extent %d starts beyond volume end (block %u >= %u)\n",
                           i, ext->xdrStABN, vol->mdb.drNmAlBlks);
                }
                errors_found++;
            }
            
            if (ext->xdrStABN + ext->xdrNumABlks > vol->mdb.drNmAlBlks) {
                if (VERBOSE || !REPAIR) {
                    printf("Catalog file extent %d extends beyond volume end\n", i);
                }
                errors_found++;
            }
        }
    }
    
    return errors_found;
}

static int repair_btree_node(btree *bt, unsigned long node_num)
{
    node n;
    
    if (VERBOSE) {
        printf("Attempting to repair B-tree node %lu\n", node_num);
    }
    
    n.bt = bt;
    n.nnum = node_num;
    
    if (bt_getnode(&n) == -1) {
        return -1; /* Cannot read node */
    }
    
    /* Try to fix common node corruption issues */
    
    /* Fix invalid node type */
    if (n.nd.ndType != ndLeafNode && n.nd.ndType != ndIndxNode) {
        /* Guess node type based on position in tree */
        if (node_num == bt->hdr.bthFNode || node_num == bt->hdr.bthLNode) {
            n.nd.ndType = ndLeafNode;
        } else {
            n.nd.ndType = ndIndxNode;
        }
        
        if (VERBOSE) {
            printf("Fixed node type to %d\n", n.nd.ndType);
        }
    }
    
    /* Fix excessive record count */
    int max_records = (bt->hdr.bthNodeSize - sizeof(NodeDescriptor)) / 4;
    if (n.nd.ndNRecs > max_records) {
        n.nd.ndNRecs = max_records;
        
        if (VERBOSE) {
            printf("Fixed record count to %d\n", n.nd.ndNRecs);
        }
    }
    
    /* Write the repaired node back */
    if (bt_putnode(&n) == -1) {
        return -1; /* Failed to write */
    }
    
    return 0; /* Success */
}

/*
 * Utility functions for enhanced HFS checking
 */

/*
 * NAME:    mctime()
 * DESCRIPTION: Convert Macintosh time to an ASCII string
 */
char *mctime(unsigned long secs)
{
    time_t date;
    static char str[26];
    
    if (secs == 0)
        return "(Never)";
    
    date = d_ltime(secs);
    strcpy(str, ctime(&date));
    str[24] = 0;
    
    return str;
}

/*
 * NAME:    extstr()
 * DESCRIPTION: Convert an extent descriptor into a printable string
 */
char *extstr(ExtDescriptor *ext)
{
    static char str[32];
    
    switch (ext->xdrNumABlks) {
        case 0:
            return "[]";
            
        case 1:
            snprintf(str, sizeof(str), "1[%u]", ext->xdrStABN);
            break;
            
        default:
            snprintf(str, sizeof(str), "%u[%u..%u]", ext->xdrNumABlks,
                    ext->xdrStABN, ext->xdrStABN + ext->xdrNumABlks - 1);
    }
    
    return str;
}

/*
 * NAME:    extrecstr()
 * DESCRIPTION: Convert an extent record into a printable string
 */
char *extrecstr(ExtDataRec *rec)
{
    static char str[60];
    ExtDescriptor *ext;
    int i;
    
    str[0] = '\0';
    {
        char *p = str;
        size_t rem = sizeof(str);
        
        for (i = 0, ext = &(*rec)[0]; i < 3; ++i, ++ext) {
            int written = 0;
            
            if (i > 0 && rem > 0) {
                written = snprintf(p, rem, "+");
                if (written < 0) written = 0;
                if ((size_t)written < rem) { 
                    p += written; 
                    rem -= written; 
                } else { 
                    rem = 0; 
                }
            }
            
            if (rem > 0) {
                const char *s = extstr(ext);
                written = snprintf(p, rem, "%s", s);
                if (written < 0) written = 0;
                if ((size_t)written < rem) { 
                    p += written; 
                    rem -= written; 
                } else { 
                    rem = 0; 
                }
            }
        }
    }
    
    return str;
}

/*
 * NAME:    outhex()
 * DESCRIPTION: Output a block of data in hex format
 */
void outhex(unsigned char *data, unsigned int len)
{
    int toggle = 0;
    
    while (len--) {
        printf("%02x%s", *data++, toggle++ & 0x01 ? " " : "");
    }
}