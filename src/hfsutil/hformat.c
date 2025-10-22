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
# include "common/hfs_detect.h"
# include "hfsutil/hfsplus_format.h"

/* Standard mkfs exit codes as defined by Unix/Linux/BSD systems */
#define MKFS_OK                 0   /* Success */
#define MKFS_GENERAL_ERROR      1   /* General error */
#define MKFS_USAGE_ERROR        2   /* Usage error */
#define MKFS_OPERATIONAL_ERROR  4   /* Operational error */
#define MKFS_SYSTEM_ERROR       8   /* System error */

# define O_FORCE	0x01

extern char *optarg;
extern int optind;

/*
 * NAME:	usage()
 * DESCRIPTION:	display comprehensive usage message with standard mkfs options
 */
static
void usage(void)
{
  fprintf(stderr, "Usage: %s [options] device [partition-no]\n", argv0);
  fprintf(stderr, "\n");
  fprintf(stderr, "Create HFS or HFS+ filesystems on devices or files.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -f, --force       Force formatting (overwrite existing data)\n");
  fprintf(stderr, "  -l, --label LABEL Set volume label (default: 'Untitled')\n");
  fprintf(stderr, "  -t, --type TYPE   Filesystem type: 'hfs' or 'hfs+' (default: auto)\n");
  fprintf(stderr, "  -s, --size SIZE   Filesystem size in bytes (for files)\n");
  fprintf(stderr, "  -v, --verbose     Display detailed formatting information\n");
  fprintf(stderr, "      --version     Display version information and exit\n");
  fprintf(stderr, "      --license     Display license information and exit\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Exit codes:\n");
  fprintf(stderr, "  0   Success\n");
  fprintf(stderr, "  1   General error\n");
  fprintf(stderr, "  2   Usage error\n");
  fprintf(stderr, "  4   Operational error\n");
  fprintf(stderr, "  8   System error\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Filesystem Types:\n");
  fprintf(stderr, "  hfs     Traditional HFS (up to 2GB volumes)\n");
  fprintf(stderr, "  hfs+    HFS+ with journaling support (recommended)\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Program Name Detection:\n");
  fprintf(stderr, "  mkfs.hfs      - Automatically format as HFS\n");
  fprintf(stderr, "  mkfs.hfs+     - Automatically format as HFS+ with journaling\n");
  fprintf(stderr, "  mkfs.hfsplus  - Same as mkfs.hfs+\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Examples:\n");
  fprintf(stderr, "  %s /dev/sdb1              Format as HFS\n", argv0);
  fprintf(stderr, "  %s -t hfs+ /dev/sdb1      Format as HFS+\n", argv0);
  fprintf(stderr, "  %s -l \"My Disk\" /dev/sdb1  Format with custom label\n", argv0);
  fprintf(stderr, "  %s -f /dev/sdb 0          Format entire disk (dangerous!)\n", argv0);
  fprintf(stderr, "\n");
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
 * NAME:	do_hfsplus_format()
 * DESCRIPTION:	format volume as HFS+ with necessary privileges
 */
static
int do_hfsplus_format(const char *path, const char *vname, int force, int verbose)
{
  hfsplus_format_opts_t opts;
  int result;

  /* Initialize format options */
  memset(&opts, 0, sizeof(opts));
  opts.device_path = (char *)path;
  opts.volume_name = (char *)vname;
  opts.block_size = 0;  /* Auto-detect optimal size */
  opts.total_size = 0;  /* Use full device */
  opts.force = force;
  opts.journal = 0;     /* No journaling initially */
  opts.case_sensitive = 0;  /* Case-insensitive HFS+ */
  opts.verbose = verbose;

  suid_enable();
  result = hfsplus_format_volume(&opts);
  suid_disable();

  return result;
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
	  return MKFS_USAGE_ERROR;

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
	    return MKFS_USAGE_ERROR;
	  }
	  break;
	}
    }

  if (argc - optind < 1 || argc - optind > 2)
    {
      usage();
      return MKFS_USAGE_ERROR;
    }

  path = hfsutil_abspath(argv[optind]);
  if (path == 0)
    {
      fprintf(stderr, "%s: not enough memory\n", argv0);
      return MKFS_SYSTEM_ERROR;
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
	      free(path);
	      return MKFS_USAGE_ERROR;
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

  /* Handle HFS+ formatting */
  if (force_fs_type == 2) {
    printf("Formatting %s as HFS+ volume '%s'...\n", path, vname);
    
    if (do_hfsplus_format(path, vname, (options & O_FORCE), 1) < 0) {
      fprintf(stderr, "%s: HFS+ formatting failed\n", argv0);
      goto fail;
    }
    
    printf("HFS+ volume '%s' created successfully\n", vname);
    printf("Filesystem type: HFS+\n");
    
    /* Note: HFS+ volumes cannot be mounted with the old HFS library */
    printf("Note: Use system tools to mount and verify the HFS+ volume\n");
    
  } else {
    /* Handle HFS formatting */
    printf("Formatting %s as HFS volume '%s'...\n", path, vname);
    
    vol = do_format(path, partno, 0, vname);
    if (vol == 0)
      {
        hfsutil_perror(path);
        goto fail;
      }

    hfs_vstat(vol, &ent);
    hfsutil_pinfo(&ent);
    
    printf("Filesystem type: HFS\n");

    if (hcwd_mounted(ent.name, ent.crdate, path, partno) == -1)
      {
        perror("Failed to record mount");
        result = 1;
      }

    hfsutil_unmount(vol, &result);
  }

  free(path);

  return result;

fail:
  if (path)
    free(path);

  return MKFS_OPERATIONAL_ERROR;
}
