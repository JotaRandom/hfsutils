/*
 * hfsutils - tools for reading and writing Macintosh HFS volumes
 * Copyright (C) 1996-1998 Robert Leslie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: hcopy.c,v 1.8 1998/04/11 08:26:56 rob Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "../../include/config.h"
# endif

# include <unistd.h>

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <errno.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <dirent.h>
# include <limits.h>

# include "hfs.h"
# include "hcwd.h"
# include "hfsutil.h"
# include "hcopy.h"
# include "copyin.h"
# include "copyout.h"

extern int optind;

/*
 * NAME:	automode_unix()
 * DESCRIPTION:	automatically choose copyin transfer mode for UNIX path
 */
static
cpifunc automode_unix(const char *path)
{
  int i;
  struct {
    const char *ext;
    cpifunc func;
  } exts[] = {
    { ".bin",  cpi_macb },
    { ".hqx",  cpi_binh },

    { ".txt",  cpi_text },
    { ".c",    cpi_text },
    { ".h",    cpi_text },
    { ".html", cpi_text },
    { ".htm",  cpi_text },
    { ".rtf",  cpi_text },

    { 0,       0        }
  };

  path += strlen(path);

  for (i = 0; exts[i].ext; ++i)
    {
      if (strcasecmp(path - strlen(exts[i].ext), exts[i].ext) == 0)
	return exts[i].func;
    }

  return cpi_raw;
}

/*
 * NAME:	copy_dir_recursive()
 * DESCRIPTION:	recursively copy a directory from UNIX to HFS
 */
static
int copy_dir_recursive(hfsvol *vol, const char *unixpath, 
		       const char *hfspath, int mode, cpifunc copyfile)
{
  DIR *dir;
  struct dirent *entry;
  struct stat sbuf;
  char unixbuf[PATH_MAX];
  char hfsbuf[PATH_MAX];
  int result = 0;

  /* Open the UNIX directory */
  dir = opendir(unixpath);
  if (dir == NULL)
    {
      ERROR(errno, "cannot open directory");
      return -1;
    }

  /* Create the HFS directory */
  if (hfs_mkdir(vol, hfspath) == -1 && errno != EEXIST)
    {
      ERROR(errno, hfs_error);
      closedir(dir);
      return -1;
    }

  /* Iterate through directory entries */
  while ((entry = readdir(dir)) != NULL)
    {
      /* Skip . and .. */
      if (strcmp(entry->d_name, ".") == 0 || 
	  strcmp(entry->d_name, "..") == 0)
	continue;

      /* Build full UNIX path */
      snprintf(unixbuf, sizeof(unixbuf), "%s/%s", unixpath, entry->d_name);

      /* Build HFS path */
      snprintf(hfsbuf, sizeof(hfsbuf), "%s:%s", hfspath, entry->d_name);

      /* Check if it's a directory */
      if (stat(unixbuf, &sbuf) != -1 && S_ISDIR(sbuf.st_mode))
	{
	  /* Recursively copy subdirectory */
	  if (copy_dir_recursive(vol, unixbuf, hfsbuf, mode, copyfile) == -1)
	    {
	      result = 1;
	    }
	}
      else
	{
	  /* Copy file */
	  cpifunc func = copyfile;
	  if (mode == 'a')
	    func = automode_unix(unixbuf);

	  if (func(unixbuf, vol, hfsbuf) == -1)
	    {
	      ERROR(errno, cpi_error);
	      hfsutil_perrorp(unixbuf);
	      result = 1;
	    }
	}
    }

  closedir(dir);
  return result;
}

/*
 * NAME:	do_copyin()
 * DESCRIPTION:	copy files from UNIX to HFS
 */
static
int do_copyin(hfsvol *vol, int argc, char *argv[], const char *dest, 
	       int mode, int recursive)
{
  hfsdirent ent;
  struct stat sbuf;
  cpifunc copyfile = cpi_raw;
  int i, result = 0;

  if (argc > 1 && (hfs_stat(vol, dest, &ent) == -1 ||
		   ! (ent.flags & HFS_ISDIR)))
    {
      ERROR(ENOTDIR, 0);
      hfsutil_perrorp(dest);

      return 1;
    }

  switch (mode)
    {
    case 'm':
      copyfile = cpi_macb;
      break;

    case 'b':
      copyfile = cpi_binh;
      break;

    case 't':
      copyfile = cpi_text;
      break;

    case 'r':
      copyfile = cpi_raw;
      break;
    }

  for (i = 0; i < argc; ++i)
    {
      if (stat(argv[i], &sbuf) != -1 &&
	  S_ISDIR(sbuf.st_mode))
	{
	  /* Handle directory based on recursive flag */
	  if (!recursive)
	    {
	      ERROR(EISDIR, 0);
	      hfsutil_perrorp(argv[i]);
	      result = 1;
	    }
	  else
	    {
	      /* Get the directory name for the HFS path */
	      const char *dirname = strrchr(argv[i], '/');
	      if (dirname == NULL)
		dirname = argv[i];
	      else
		dirname++;

	      /* Use dest directly - it should be the full HFS path */
	      if (copy_dir_recursive(vol, argv[i], dest, mode, copyfile) == -1)
		{
		  ERROR(errno, cpi_error);
		  hfsutil_perrorp(argv[i]);
		  result = 1;
		}
	    }
	}
      else
	{
	  if (mode == 'a')
	    copyfile = automode_unix(argv[i]);

	  if (copyfile(argv[i], vol, dest) == -1)
	    {
	      ERROR(errno, cpi_error);
	      hfsutil_perrorp(argv[i]);

	      result = 1;
	    }
	}
    }

  return result;
}

/*
 * NAME:	automode_hfs()
 * DESCRIPTION:	automatically choose copyout transfer mode for HFS path
 */
static
cpofunc automode_hfs(hfsvol *vol, const char *path)
{
  hfsdirent ent;

  if (hfs_stat(vol, path, &ent) != -1)
    {
      if (strcmp(ent.u.file.type, "TEXT") == 0 ||
	  strcmp(ent.u.file.type, "ttro") == 0)
	return cpo_text;
      else if (ent.u.file.rsize == 0)
	return cpo_raw;
    }

  return cpo_macb;
}

/*
 * NAME:	do_copyout()
 * DESCRIPTION:	copy files from HFS to UNIX
 */
static
int do_copyout(hfsvol *vol, int argc, char *argv[], const char *dest, 
	        int mode, int recursive)
{
  struct stat sbuf;
  hfsdirent ent;
  cpofunc copyfile = cpo_macb;
  int i, result = 0;

  if (argc > 1 && (stat(dest, &sbuf) == -1 ||
		   ! S_ISDIR(sbuf.st_mode)))
    {
      ERROR(ENOTDIR, 0);
      hfsutil_perrorp(dest);

      return 1;
    }

  switch (mode)
    {
    case 'm':
      copyfile = cpo_macb;
      break;

    case 'b':
      copyfile = cpo_binh;
      break;

    case 't':
      copyfile = cpo_text;
      break;

    case 'r':
      copyfile = cpo_raw;
      break;
    }

  for (i = 0; i < argc; ++i)
    {
      if (hfs_stat(vol, argv[i], &ent) != -1 &&
	  (ent.flags & HFS_ISDIR))
	{
	  ERROR(EISDIR, 0);
	  hfsutil_perrorp(argv[i]);

	  result = 1;
	}
      else
	{
	  if (mode == 'a')
	    copyfile = automode_hfs(vol, argv[i]);

	  if (copyfile(vol, argv[i], dest) == -1)
	    {
	      ERROR(errno, cpo_error);
	      hfsutil_perrorp(argv[i]);

	      result = 1;
	    }
	}
    }

  return result;
}

/*
 * NAME:	usage()
 * DESCRIPTION:	display usage message
 */
static
int usage(void)
{
  fprintf(stderr, "Usage: %s [-m|-b|-t|-r|-a] [-R] source-path [...] target-path\n",
	  argv0);

  return 1;
}

/*
 * NAME:	hcopy->main()
 * DESCRIPTION:	implement hcopy command
 */
int hcopy_main(int argc, char *argv[])
{
  int nargs, mode = 'a', result = 0, recursive = 0;
  const char *target;
  int fargc;
  char **fargv;
  hfsvol *vol;
  int (*copy)(hfsvol *, int, char *[], const char *, int, int);

  while (1)
    {
      int opt;

      opt = getopt(argc, argv, "mbtraR");
      if (opt == EOF)
	break;

      switch (opt)
	{
	case '?':
	  return usage();

	case 'R':
	  recursive = 1;
	  break;

	default:
	  mode = opt;
	}
    }

  nargs = argc - optind;

  if (nargs < 2)
    return usage();

  target = argv[argc - 1];

  if (strchr(target, ':') && target[0] != '.' && target[0] != '/')
    {
      vol = hfsutil_remount(hcwd_getvol(-1), HFS_MODE_ANY);
      if (vol == 0)
	return 1;

      copy  = do_copyin;
      fargc = nargs - 1;
      fargv = &argv[optind];
    }
  else
    {
      vol = hfsutil_remount(hcwd_getvol(-1), HFS_MODE_RDONLY);
      if (vol == 0)
	return 1;

      copy  = do_copyout;
      fargv = hfsutil_glob(vol, nargs - 1, &argv[optind], &fargc, &result);
    }

  if (result == 0)
    result = copy(vol, fargc, fargv, target, mode, recursive);

  hfsutil_unmount(vol, &result);

  if (fargv && fargv != &argv[optind])
    free(fargv);

  return result;
}
