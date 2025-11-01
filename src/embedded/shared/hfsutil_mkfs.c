/*
 * hfsutil_mkfs.c - HFS utility functions for mkfs.hfs
 * Extracted from src/hfsutil/hfsutil.c for standalone mkfs.hfs utility
 * Copyright (C) 1996-1998 Robert Leslie
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "libhfs.h"

/* Global variables */
const char *argv0 = "mkfs.hfs";
const char *bargv0 = "mkfs.hfs";

/*
 * NAME:	hfsutil_perror()
 * DESCRIPTION:	print HFS error
 */
void hfsutil_perror(const char *msg)
{
  const char *str = hfs_error;

  if (str == 0)
    {
      perror(msg);
      return;
    }

  if (msg && *msg)
    fprintf(stderr, "%s: %s (%s)\n", argv0, msg, str);
  else
    fprintf(stderr, "%s: %s\n", argv0, str);
}

/*
 * NAME:	hfsutil_perrorp()
 * DESCRIPTION:	print HFS error with pathname
 */
void hfsutil_perrorp(const char *path)
{
  const char *str = hfs_error;

  if (str == 0)
    {
      perror(path);
      return;
    }

  fprintf(stderr, "%s: %s (%s)\n", argv0, path, str);
}

/*
 * NAME:	hfsutil_pinfo()
 * DESCRIPTION:	print volume information
 */
void hfsutil_pinfo(hfsvolent *ent)
{
  printf("Volume name: \"%s\"\n", ent->name);
  printf("Volume size: %lu bytes (%lu allocation blocks)\n",
	 ent->totbytes, ent->totbytes / ent->alblocksz);
  printf("Volume free: %lu bytes (%lu allocation blocks)\n",
	 ent->freebytes, ent->freebytes / ent->alblocksz);
  printf("Allocation block size: %lu bytes\n", ent->alblocksz);
  printf("Clump size: %lu bytes\n", ent->clumpsz);
  printf("Number of files: %lu\n", ent->numfiles);
  printf("Number of directories: %lu\n", ent->numdirs);
  printf("Volume created: %s", ctime(&ent->crdate));
  printf("Volume modified: %s", ctime(&ent->mddate));
  if (ent->bkdate)
    printf("Volume backed up: %s", ctime(&ent->bkdate));
  else
    printf("Volume never backed up\n");
}

/*
 * NAME:	hfsutil_unmount()
 * DESCRIPTION:	unmount a volume
 */
void hfsutil_unmount(hfsvol *vol, int *result)
{
  if (hfs_umount(vol) == -1 && *result == 0)
    {
      hfsutil_perror("Error unmounting volume");
      *result = 1;
    }
}

/*
 * NAME:	hfsutil_samepath()
 * DESCRIPTION:	return 1 iff paths refer to same file/directory
 */
int hfsutil_samepath(const char *path1, const char *path2)
{
  struct stat sbuf1, sbuf2;

  return (stat(path1, &sbuf1) == 0 &&
	  stat(path2, &sbuf2) == 0 &&
	  sbuf1.st_dev == sbuf2.st_dev &&
	  sbuf1.st_ino == sbuf2.st_ino);
}

/*
 * NAME:	hfsutil_abspath()
 * DESCRIPTION:	make given UNIX path absolute (must be free()'d)
 */
char *hfsutil_abspath(const char *path)
{
  char *cwd, *buf;
  size_t len;

  if (path[0] == '/')
    return strdup(path);

  cwd = getenv("PWD");
  if (cwd && hfsutil_samepath(cwd, "."))
    {
      buf = malloc(strlen(cwd) + 1 + strlen(path) + 1);
      if (buf == 0)
	return 0;

      strcpy(buf, cwd);
    }
  else
    {
      len = 32;
      cwd = malloc(len);
      if (cwd == 0)
	return 0;

      while (getcwd(cwd, len) == 0)
	{
	  if (errno != ERANGE)
	    {
	      free(cwd);
	      return 0;
	    }

	  len <<= 1;
	  buf = realloc(cwd, len);
	  if (buf == 0)
	    {
	      free(cwd);
	      return 0;
	    }

	  cwd = buf;
	}

      buf = realloc(cwd, strlen(cwd) + 1 + strlen(path) + 1);
      if (buf == 0)
	{
	  free(cwd);
	  return 0;
	}
    }

  strcat(buf, "/");
  strcat(buf, path);

  return buf;
}