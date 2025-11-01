/*
 * hfs_mount_util.c - HFS mounting functions for mount.hfs
 * Extracted from src/hfsutil/hmount.c for standalone mount.hfs utility
 * Copyright (C) 1996-1998 Robert Leslie
 */

#include "mount_hfs.h"

/*
 * NAME:	hfs_mount_volume()
 * DESCRIPTION:	mount HFS/HFS+ volume and display information
 */
int hfs_mount_volume(const char *path, int partno, int mount_options)
{
  char *abs_path = 0;
  hfsvol *vol;
  hfsvolent ent;
  int nparts, result = 0;

  /* Convert to absolute path */
  abs_path = hfsutil_abspath(path);
  if (abs_path == 0)
    {
      fprintf(stderr, "mount.hfs: not enough memory\n");
      return 1;
    }

  /* Get partition information */
  suid_enable();
  nparts = hfs_nparts(abs_path);
  suid_disable();

  if (nparts >= 0)
    {
      if (mount_options & MOUNT_VERBOSE)
        printf("%s: contains %d HFS partition%s\n", abs_path, nparts,
               nparts == 1 ? "" : "s");
    }

  /* Validate partition number */
  if (partno == -1)
    {
      if (nparts > 1)
        {
          fprintf(stderr, "mount.hfs: must specify partition number\n");
          goto fail;
        }
      else if (nparts == -1)
        partno = 0;
      else
        partno = 1;
    }

  /* Mount the volume */
  suid_enable();
  vol = hfs_mount(abs_path, partno, HFS_MODE_ANY);
  suid_disable();

  if (vol == 0)
    {
      hfsutil_perror(abs_path);
      goto fail;
    }

  /* Get and display volume information */
  if (hfs_vstat(vol, &ent) == 0)
    {
      if (mount_options & MOUNT_VERBOSE)
        {
          hfsutil_pinfo(&ent);
        }
      else
        {
          printf("Mounted HFS volume: %s\n", ent.name);
          printf("Volume size: %lu bytes\n", ent.totbytes);
          printf("Free space: %lu bytes\n", ent.freebytes);
        }
    }

  /* Record the mount (simplified version) */
  if (mount_options & MOUNT_RECORD)
    {
      if (hcwd_mounted(ent.name, ent.crdate, abs_path, partno) == -1)
        {
          perror("Failed to record mount");
          result = 1;
        }
    }

  /* Unmount the volume */
  hfsutil_unmount(vol, &result);

  free(abs_path);
  return result;

fail:
  if (abs_path)
    free(abs_path);
  return 1;
}

/*
 * NAME:	hcwd_mounted()
 * DESCRIPTION:	record mounted volume (simplified stub)
 */
int hcwd_mounted(const char *vname, unsigned long crdate, const char *path, int partno)
{
  /* Simplified implementation - just return success */
  /* In a full implementation, this would record the mount in a database */
  (void)vname;
  (void)crdate;
  (void)path;
  (void)partno;
  
  return 0;
}