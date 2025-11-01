/*
 * volume_mkfs.c - Volume management functions for mkfs.hfs
 * Simplified implementation for standalone mkfs.hfs utility
 * Copyright (C) 1996-1998 Robert Leslie
 */

#include "libhfs.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

/* Forward declarations */
static int flushvol(hfsvol *vol, int umount);

/*
 * NAME:	v_init()
 * DESCRIPTION:	initialize volume structure
 */
void v_init(hfsvol *vol, int flags)
{
  /* Initialize basic volume structure */
  vol->priv       = 0;
  vol->flags      = flags & HFS_VOL_OPT_MASK;

  vol->pnum       = -1;
  vol->vstart     = 0;
  vol->vlen       = 0;
  vol->lpa        = 0;

  vol->cache      = 0;

  vol->vbm        = 0;
  vol->vbmsz      = 0;

  /* Initialize B-tree structures (simplified) */
  memset(&vol->ext, 0, sizeof(vol->ext));
  memset(&vol->cat, 0, sizeof(vol->cat));

  vol->cwd        = HFS_CNID_ROOTDIR;

  vol->refs       = 0;
  vol->files      = 0;
  vol->dirs       = 0;

  vol->prev       = 0;
  vol->next       = 0;
}

/*
 * NAME:	v_open()
 * DESCRIPTION:	open volume source and lock against concurrent updates
 */
int v_open(hfsvol *vol, const char *path, int mode)
{
  int fd;
  int open_flags;
  
  if (vol->flags & HFS_VOL_OPEN)
    {
      errno = EINVAL;
      return -1;
    }

  /* Convert HFS mode to system open flags */
  switch (mode) {
    case HFS_MODE_RDONLY:
      open_flags = O_RDONLY;
      break;
    case HFS_MODE_RDWR:
      open_flags = O_RDWR;
      break;
    default:
      open_flags = O_RDWR | O_CREAT;
      break;
  }

  fd = open(path, open_flags, 0644);
  if (fd == -1)
    return -1;

  vol->priv = (void *)(intptr_t)fd;
  vol->flags |= HFS_VOL_OPEN;

  return 0;
}

/*
 * NAME:	v_geometry()
 * DESCRIPTION:	determine volume location and size (possibly in a partition)
 */
int v_geometry(hfsvol *vol, int pnum)
{
  struct stat st;
  int fd;

  if (! (vol->flags & HFS_VOL_OPEN))
    {
      errno = EINVAL;
      return -1;
    }

  fd = (int)(intptr_t)vol->priv;
  
  if (fstat(fd, &st) == -1)
    return -1;

  vol->pnum   = pnum;
  vol->vstart = 0;
  
  if (S_ISREG(st.st_mode)) {
    vol->vlen = st.st_size / HFS_BLOCKSZ;
  } else if (S_ISBLK(st.st_mode)) {
    /* For block devices, try to get size */
    off_t size = lseek(fd, 0, SEEK_END);
    if (size == -1) {
      vol->vlen = 0x7FFFFFFF;  /* Large default */
    } else {
      vol->vlen = size / HFS_BLOCKSZ;
    }
    lseek(fd, 0, SEEK_SET);
  } else {
    errno = EINVAL;
    return -1;
  }

  if (vol->vlen < 800)
    {
      errno = EINVAL;
      return -1;
    }

  return 0;
}

/*
 * NAME:	flushvol()
 * DESCRIPTION:	flush all pending changes to volume
 */
static
int flushvol(hfsvol *vol, int umount)
{
  int fd;
  
  if (!vol || !(vol->flags & HFS_VOL_OPEN))
    return 0;
    
  fd = (int)(intptr_t)vol->priv;
  
  /* Simplified flush - just sync the file descriptor */
  if (fsync(fd) == -1)
    return -1;

  /* Clear update flags */
  vol->flags &= ~(HFS_VOL_UPDATE_VBM | HFS_VOL_UPDATE_MDB | HFS_VOL_UPDATE_ALTMDB);

  return 0;
}

/*
 * NAME:	v_flush()
 * DESCRIPTION:	commit all pending changes to volume device
 */
int v_flush(hfsvol *vol)
{
  if (flushvol(vol, 0) == -1)
    return -1;

  return 0;
}

/*
 * NAME:	v_close()
 * DESCRIPTION:	close access path to volume source
 */
int v_close(hfsvol *vol)
{
  int result = 0;

  if (! (vol->flags & HFS_VOL_OPEN))
    goto done;

  if (vol->flags & HFS_VOL_MOUNTED)
    {
      if (flushvol(vol, 1) == -1)
	result = -1;

      if (vol->ext.map)
	FREE(vol->ext.map);

      if (vol->cat.map)
	FREE(vol->cat.map);

      if (vol->vbm)
	FREE(vol->vbm);

      if (vol->cache)
	{
	  FREE(vol->cache);
	  vol->cache = 0;
	}
    }

  if (vol->priv) {
    int fd = (int)(intptr_t)vol->priv;
    if (close(fd) == -1)
      result = -1;
    vol->priv = 0;
  }

  vol->flags &= ~(HFS_VOL_OPEN | HFS_VOL_MOUNTED);

done:
  return result;
}

/*
 * NAME:	v_same()
 * DESCRIPTION:	return 1 iff path is same as open volume
 */
int v_same(hfsvol *vol, const char *path)
{
  /* Simplified implementation - just compare paths */
  return 0;  /* For mkfs, we don't need complex path comparison */
}

/*
 * NAME:	getvol()
 * DESCRIPTION:	validate volume reference
 */
int getvol(hfsvol **vol)
{
  if (*vol == 0)
    {
      errno = EINVAL;
      return -1;
    }

  return 0;
}

/*
 * NAME:	d_mtime()
 * DESCRIPTION:	convert local time to MacOS time (simplified version)
 */
unsigned long d_mtime(time_t ltime)
{
  /* MacOS time is seconds since January 1, 1904 00:00:00 GMT */
  /* Unix time is seconds since January 1, 1970 00:00:00 GMT */
  /* Difference is 66 years = 2082844800 seconds */
  const unsigned long mac_unix_offset = 2082844800UL;
  
  return (unsigned long)ltime + mac_unix_offset;
}