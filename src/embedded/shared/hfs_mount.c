/*
 * hfs_mount.c - HFS mounting functions for mkfs.hfs
 * Extracted from libhfs/hfs.c for standalone mkfs.hfs utility
 * Copyright (C) 1996-1998 Robert Leslie
 */

#include "libhfs.h"
#include <sys/stat.h>

/* Global variables */
hfsvol *hfs_mounts = 0;		/* linked list of mounted volumes */
hfsvol *curvol = 0;		/* current volume */

/*
 * NAME:	hfs_mount()
 * DESCRIPTION:	open an HFS volume; return volume descriptor or 0 (error)
 */
hfsvol *hfs_mount(const char *path, int pnum, int mode)
{
  hfsvol *vol, *check;

  /* see if the volume is already mounted */

  for (check = hfs_mounts; check; check = check->next)
    {
      if (check->pnum == pnum && v_same(check, path) == 1)
	{
	  /* verify compatible read/write mode */

	  if (((check->flags & HFS_VOL_READONLY) &&
	       ! (mode & HFS_MODE_RDWR)) ||
	      (! (check->flags & HFS_VOL_READONLY) &&
	       (mode & (HFS_MODE_RDWR | HFS_MODE_ANY))))
	    {
	      vol = check;
	      goto done;
	    }
	}
    }

  vol = ALLOC(hfsvol, 1);
  if (vol == 0)
    {
      ERROR(ENOMEM, "not enough memory for volume descriptor");
      return 0;
    }

  v_init(vol, mode);

  /* open the medium */

  switch (mode & HFS_MODE_MASK)
    {
    case HFS_MODE_RDWR:
    case HFS_MODE_ANY:
      if (v_open(vol, path, HFS_MODE_RDWR) != -1)
	break;

      if ((mode & HFS_MODE_MASK) == HFS_MODE_RDWR)
	goto fail;

    case HFS_MODE_RDONLY:
    default:
      vol->flags |= HFS_VOL_READONLY;

      if (v_open(vol, path, HFS_MODE_RDONLY) == -1)
	goto fail;
    }

  /* mount the volume */

  if (v_geometry(vol, pnum) == -1 ||
      v_mount(vol) == -1)
    goto fail;

  /* add to linked list of volumes */

  vol->prev = 0;
  vol->next = hfs_mounts;

  if (hfs_mounts)
    hfs_mounts->prev = vol;

  hfs_mounts = vol;

done:
  ++vol->refs;
  curvol = vol;

  return vol;

fail:
  if (vol)
    {
      v_close(vol);
      FREE(vol);
    }

  return 0;
}

/*
 * NAME:	hfs_umount()
 * DESCRIPTION:	close an HFS volume
 */
int hfs_umount(hfsvol *vol)
{
  hfsfile *file;

  if (getvol(&vol) == -1)
    goto fail;

  /* close all open files and directories */

  while (vol->files)
    {
      file = vol->files;
      if (hfs_close(file) == -1)
	goto fail;
    }

  while (vol->dirs)
    {
      if (hfs_closedir(vol->dirs) == -1)
	goto fail;
    }

  /* close the volume */

  --vol->refs;
  if (vol->refs == 0)
    {
      if (v_flush(vol) == -1)
	goto fail;

      /* remove from linked list */

      if (vol->prev)
	vol->prev->next = vol->next;
      if (vol->next)
	vol->next->prev = vol->prev;

      if (vol == hfs_mounts)
	hfs_mounts = vol->next;

      if (vol == curvol)
	curvol = 0;

      v_close(vol);
      FREE(vol);
    }

  return 0;

fail:
  return -1;
}

/*
 * NAME:	hfs_vstat()
 * DESCRIPTION:	return volume statistics
 */
int hfs_vstat(hfsvol *vol, hfsvolent *ent)
{
  if (getvol(&vol) == -1)
    return -1;

  strcpy(ent->name, vol->mdb.drVN);
  ent->flags = 0;

  if (vol->flags & HFS_VOL_READONLY)
    ent->flags |= HFS_ISLOCKED;

  ent->totbytes  = vol->mdb.drNmAlBlks * vol->mdb.drAlBlkSiz;
  ent->freebytes = vol->mdb.drFreeBks  * vol->mdb.drAlBlkSiz;

  ent->alblocksz = vol->mdb.drAlBlkSiz;
  ent->clumpsz   = vol->mdb.drClpSiz;

  ent->numfiles = vol->mdb.drFilCnt;
  ent->numdirs  = vol->mdb.drDirCnt;

  ent->crdate = d_ltime(vol->mdb.drCrDate);
  ent->mddate = d_ltime(vol->mdb.drLsMod);
  ent->bkdate = d_ltime(vol->mdb.drVolBkUp);

  ent->blessed = vol->mdb.drFndrInfo[0];

  return 0;
}

/*
 * NAME:	hfs_nparts()
 * DESCRIPTION:	return the number of HFS partitions present on a medium
 */
int hfs_nparts(const char *path)
{
  hfsvol vol;
  struct stat st;

  v_init(&vol, HFS_MODE_RDONLY);

  if (v_open(&vol, path, HFS_MODE_RDONLY) == -1)
    {
      v_close(&vol);
      return -1;
    }

  if (v_geometry(&vol, 0) == -1)
    {
      v_close(&vol);
      return -1;
    }

  /* Check if this is a regular file (non-partitioned) or block device */
  if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
    /* Regular file - assume non-partitioned HFS volume */
    v_close(&vol);
    return -1;
  }

  v_close(&vol);

  /* For block devices, we'd need to check partition table */
  /* For now, assume non-partitioned */
  return -1;
}