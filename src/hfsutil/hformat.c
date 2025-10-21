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
 * $Id: hformat.c,v 1.9 1998/11/02 22:08:31 rob Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "../../include/config.h"
# endif

# include <unistd.h>

# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# include "hfs.h"
# include "hcwd.h"
# include "hfsutil.h"
# include "suid.h"
# include "hformat.h"
# include "hfs_detect.h"

# define O_FORCE	0x01

extern char *optarg;
extern int optind;

/*
 * NAME:	usage()
 * DESCRIPTION:	display usage message
 */
static
void usage(void)
{
  fprintf(stderr, "Usage: %s [-f] [-l label] [-t fstype] path [partition-no]\n", argv0);
  fprintf(stderr, "  -f          Force formatting (overwrite existing partitions)\n");
  fprintf(stderr, "  -l label    Set volume label (default: 'Untitled')\n");
  fprintf(stderr, "  -t fstype   Filesystem type: 'hfs' or 'hfs+' (default: hfs)\n");
  fprintf(stderr, "\nFilesystem type can also be specified by program name:\n");
  fprintf(stderr, "  mkfs.hfs    - Format as HFS\n");
  fprintf(stderr, "  mkfs.hfs+   - Format as HFS+\n");
}

/*
 * NAME:	do_format()
 * DESCRIPTION:	call hfs_format() with necessary privileges
 */
static
hfsvol *do_format(const char *path, int partno, int mode, const char *vname)
{
  hfsvol *vol = 0;

  suid_enable();

  if (hfs_format(path, partno, mode, vname, 0, 0) != -1)
    vol = hfs_mount(path, partno, HFS_MODE_ANY);

  suid_disable();

  return vol;
}

/*
 * NAME:	hformat->main()
 * DESCRIPTION:	implement hformat command
 */
int hformat_main(int argc, char *argv[])
{
  const char *vname;
  char *path = 0;
  hfsvol *vol;
  hfsvolent ent;
  int nparts, partno, options = 0, result = 0;
  const char *progname;
  int force_fs_type = 0;  /* 0=auto/HFS, 1=HFS, 2=HFS+ */

  vname = "Untitled";

  /* Determine program name and default filesystem type */
  progname = strrchr(argv[0], '/');
  if (progname == NULL)
    progname = argv[0];
  else
    progname++;

  /* Check if called as mkfs.hfs or mkfs.hfs+ */
  if (strcmp(progname, "mkfs.hfs") == 0) {
    force_fs_type = 1;  /* Force HFS */
  } else if (strcmp(progname, "mkfs.hfs+") == 0 || strcmp(progname, "mkfs.hfsplus") == 0) {
    force_fs_type = 2;  /* Force HFS+ */
  }

  while (1)
    {
      int opt;

      opt = getopt(argc, argv, "fl:t:");
      if (opt == EOF)
	break;

      switch (opt)
	{
	case '?':
	  usage();
	  goto fail;

	case 'f':
	  options |= O_FORCE;
	  break;

	case 'l':
	  vname = optarg;
	  break;

	case 't':
	  if (strcmp(optarg, "hfs") == 0) {
	    force_fs_type = 1;
	  } else if (strcmp(optarg, "hfs+") == 0 || strcmp(optarg, "hfsplus") == 0) {
	    force_fs_type = 2;
	  } else {
	    fprintf(stderr, "%s: invalid filesystem type '%s' (use 'hfs' or 'hfs+')\n", argv0, optarg);
	    goto fail;
	  }
	  break;
	}
    }

  if (argc - optind < 1 || argc - optind > 2)
    {
      usage();
      goto fail;
    }

  path = hfsutil_abspath(argv[optind]);
  if (path == 0)
    {
      fprintf(stderr, "%s: not enough memory\n", argv0);
      goto fail;
    }

  suid_enable();
  nparts = hfs_nparts(path);
  suid_disable();

  if (argc - optind == 2)
    {
      partno = atoi(argv[optind + 1]);

      if (nparts != -1 && partno == 0)
	{
	  if (options & O_FORCE)
	    {
	      fprintf(stderr,
		      "%s: warning: erasing partition information\n", argv0);
	    }
	  else
	    {
	      fprintf(stderr, "%s: medium is partitioned; "
		      "select partition > 0 or use -f\n", argv0);
	      goto fail;
	    }
	}
    }
  else
    {
      if (nparts > 1)
	{
	  fprintf(stderr,
		  "%s: must specify partition number (%d available)\n",
		  argv0, nparts);
	  goto fail;
	}
      else if (nparts == -1)
	partno = 0;
      else
	partno = 1;
    }

  /* Display formatting information */
  if (force_fs_type == 2) {
    printf("Formatting %s as HFS+ volume '%s'...\n", path, vname);
    /* TODO: Implement HFS+ formatting */
    fprintf(stderr, "%s: HFS+ formatting not yet implemented\n", argv0);
    goto fail;
  } else {
    printf("Formatting %s as HFS volume '%s'...\n", path, vname);
  }

  vol = do_format(path, partno, 0, vname);
  if (vol == 0)
    {
      hfsutil_perror(path);
      goto fail;
    }

  hfs_vstat(vol, &ent);
  hfsutil_pinfo(&ent);
  
  /* Display filesystem type information */
  printf("Filesystem type: %s\n", (force_fs_type == 2) ? "HFS+" : "HFS");

  if (hcwd_mounted(ent.name, ent.crdate, path, partno) == -1)
    {
      perror("Failed to record mount");
      result = 1;
    }

  hfsutil_unmount(vol, &result);

  free(path);

  return result;

fail:
  if (path)
    free(path);

  return 1;
}
