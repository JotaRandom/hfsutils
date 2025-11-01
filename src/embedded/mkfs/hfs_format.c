/*
 * hfs_format.c - Simplified HFS formatting functions for mkfs.hfs
 * Simplified version for standalone mkfs.hfs utility
 * Copyright (C) 1996-1998 Robert Leslie
 */

#include "mkfs_hfs.h"

/* Forward declarations */
static int validvname(const char *vname);
static int create_basic_hfs(hfsvol *vol, const char *vname);

/*
 * NAME:	validvname()
 * DESCRIPTION:	validate a volume name
 */
static
int validvname(const char *vname)
{
  int len;

  len = strlen(vname);

  if (len < 1 || len > HFS_MAX_VLEN)
    return 0;

  /* Check for invalid characters */
  while (*vname)
    {
      if (*vname == ':')
        return 0;
      ++vname;
    }

  return 1;
}

/*
 * NAME:	create_basic_hfs()
 * DESCRIPTION:	create basic HFS structures
 */
static
int create_basic_hfs(hfsvol *vol, const char *vname)
{
  unsigned int i;

  /* initialize volume geometry */
  vol->lpa = 1 + ((vol->vlen - 6) >> 16);

  if (vol->flags & HFS_OPT_2048)
    vol->lpa = (vol->lpa + 3) & ~3;

  vol->vbmsz = (vol->vlen / vol->lpa + 0x0fff) >> 12;

  /* Initialize Master Directory Block */
  vol->mdb.drSigWord  = HFS_SIGWORD;
  vol->mdb.drCrDate   = d_mtime(time(0));
  vol->mdb.drLsMod    = vol->mdb.drCrDate;
  vol->mdb.drAtrb     = 0;
  vol->mdb.drNmFls    = 0;
  vol->mdb.drVBMSt    = 3;
  vol->mdb.drAllocPtr = 0;

  vol->mdb.drAlBlkSiz = vol->lpa << HFS_BLOCKSZ_BITS;
  vol->mdb.drClpSiz   = vol->mdb.drAlBlkSiz << 2;
  vol->mdb.drAlBlSt   = vol->mdb.drVBMSt + vol->vbmsz;

  if (vol->flags & HFS_OPT_2048)
    vol->mdb.drAlBlSt = ((vol->vstart & 3) + vol->mdb.drAlBlSt + 3) & ~3;

  vol->mdb.drNmAlBlks = (vol->vlen - 2 - vol->mdb.drAlBlSt) / vol->lpa;
  vol->mdb.drNxtCNID  = HFS_CNID_ROOTDIR + 1;
  vol->mdb.drFreeBks  = vol->mdb.drNmAlBlks;

  strcpy(vol->mdb.drVN, vname);

  vol->mdb.drVolBkUp  = 0;
  vol->mdb.drVSeqNum  = 0;
  vol->mdb.drWrCnt    = 0;

  vol->mdb.drXTClpSiz = vol->mdb.drNmAlBlks / 128 * vol->mdb.drAlBlkSiz;
  vol->mdb.drCTClpSiz = vol->mdb.drXTClpSiz;

  vol->mdb.drNmRtDirs = 0;
  vol->mdb.drFilCnt   = 0;
  vol->mdb.drDirCnt   = 1;  /* root directory */

  for (i = 0; i < 8; ++i)
    vol->mdb.drFndrInfo[i] = 0;

  vol->mdb.drEmbedSigWord            = 0x0000;
  vol->mdb.drEmbedExtent.xdrStABN    = 0;
  vol->mdb.drEmbedExtent.xdrNumABlks = 0;

  /* Set minimal extents for system files */
  vol->mdb.drXTFlSize = vol->mdb.drAlBlkSiz;
  vol->mdb.drCTFlSize = vol->mdb.drAlBlkSiz;

  /* Initialize extent records */
  for (i = 0; i < 3; ++i)
    {
      vol->mdb.drXTExtRec[i].xdrStABN = 0;
      vol->mdb.drXTExtRec[i].xdrNumABlks = 0;
      vol->mdb.drCTExtRec[i].xdrStABN = 0;
      vol->mdb.drCTExtRec[i].xdrNumABlks = 0;
    }

  /* Set first extent for extents file */
  vol->mdb.drXTExtRec[0].xdrStABN = 0;
  vol->mdb.drXTExtRec[0].xdrNumABlks = 1;

  /* Set first extent for catalog file */
  vol->mdb.drCTExtRec[0].xdrStABN = 1;
  vol->mdb.drCTExtRec[0].xdrNumABlks = 1;

  vol->flags |= HFS_VOL_UPDATE_MDB | HFS_VOL_UPDATE_ALTMDB;

  /* initialize volume bitmap */
  vol->vbm = ALLOC(block, vol->vbmsz);
  if (vol->vbm == 0)
    {
      ERROR(ENOMEM, "not enough memory for volume bitmap");
      return -1;
    }

  memset(vol->vbm, 0, vol->vbmsz << HFS_BLOCKSZ_BITS);

  /* Mark first few allocation blocks as used (for system files) */
  if (vol->vbmsz > 0)
    {
      unsigned char *bitmap = (unsigned char *)vol->vbm;
      bitmap[0] = 0xC0;  /* Mark first 2 allocation blocks as used */
    }

  vol->flags |= HFS_VOL_UPDATE_VBM;

  return 0;
}

/*
 * NAME:	hfs_format()
 * DESCRIPTION:	write a new HFS filesystem (simplified version)
 */
int hfs_format(const char *path, int pnum, int mode, const char *vname,
	       unsigned int nbadblocks, const unsigned long badblocks[])
{
  hfsvol vol;

  /* Ignore bad blocks for now in simplified version */
  (void)nbadblocks;
  (void)badblocks;

  v_init(&vol, mode);

  if (! validvname(vname))
    {
      ERROR(EINVAL, "invalid volume name");
      goto fail;
    }

  if (v_open(&vol, path, HFS_MODE_RDWR) == -1 ||
      v_geometry(&vol, pnum) == -1)
    goto fail;

  if (create_basic_hfs(&vol, vname) == -1)
    goto fail;

  /* Mark volume as mounted for flushing */
  vol.flags |= HFS_VOL_MOUNTED;

  /* flush all pending changes */
  if (v_flush(&vol) == -1)
    goto fail;

  v_close(&vol);

  return 0;

fail:
  v_close(&vol);
  return -1;
}