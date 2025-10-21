/*
 * hfsck - tool for checking and repairing the integrity of HFS volumes
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
 * $Id: main.c,v 1.8 1998/11/02 22:08:50 rob Exp $
 */

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <errno.h>

# include "hfsck.h"
# include "suid.h"
# include "version.h"
# include "journal.h"
# include "../include/hfsutil/hfs_detect.h"

int options;

extern int optind;

/*
 * NAME:	usage()
 * DESCRIPTION:	display usage message
 */
static
int usage(char *argv[])
{
  fprintf(stderr, "Usage: %s [-v] [-n] [-a] device-path [partition-no]\n",
	  argv[0]);

  return 1;
}

/*
 * NAME:	main()
 * DESCRIPTION:	program entry
 */
int main(int argc, char *argv[])
{
  char *path;
  int nparts, pnum, result;
  hfsvol vol;
  const char *progname;
  int force_fs_type = 0;  /* 0=auto, 1=HFS, 2=HFS+ */

  suid_init();

  /* Determine program name and filesystem type */
  progname = strrchr(argv[0], '/');
  if (progname == NULL)
    progname = argv[0];
  else
    progname++;

  /* Check if called as fsck.hfs or fsck.hfs+ */
  if (strcmp(progname, "fsck.hfs") == 0) {
    force_fs_type = 1;  /* Force HFS */
  } else if (strcmp(progname, "fsck.hfs+") == 0 || strcmp(progname, "fsck.hfsplus") == 0) {
    force_fs_type = 2;  /* Force HFS+ */
  }

  if (argc == 2)
    {
      if (strcmp(argv[1], "--version") == 0)
	{
	  printf("%s - %s\n", hfsutils_version, hfsutils_copyright);
	  printf("Supports HFS and HFS+ filesystem checking\n");
	  printf("`%s --license' for licensing information.\n", argv[0]);
	  return 0;
	}
      else if (strcmp(argv[1], "--license") == 0)
	{
	  printf("\n%s", hfsutils_license);
	  return 0;
	}
    }

  options = HFSCK_REPAIR;

  while (1)
    {
      int opt;

      opt = getopt(argc, argv, "vna");
      if (opt == EOF)
	break;

      switch (opt)
	{
	case '?':
	  return usage(argv);

	case 'v':
	  options |= HFSCK_VERBOSE;
	  break;

	case 'n':
	  options &= ~HFSCK_REPAIR;
	  break;

	case 'a':
	  options |= HFSCK_YES;
	  break;
	}
    }

  if (argc - optind < 1 ||
      argc - optind > 2)
    return usage(argv);

  path = argv[optind];

  suid_enable();
  nparts = hfs_nparts(path);
  suid_disable();

  if (nparts == 0)
    {
      fprintf(stderr, "%s: partitioned medium contains no HFS partitions\n",
	      argv[0]);
      return 1;
    }

  if (argc - optind == 2)
    {
      pnum = atoi(argv[optind + 1]);

      if (pnum < 0)
	{
	  fprintf(stderr, "%s: invalid partition number\n", argv[0]);
	  return 1;
	}

      if (nparts == -1 && pnum > 0)
	{
	  fprintf(stderr, "%s: warning: ignoring partition number for"
		  " non-partitioned medium\n", argv[0]);
	  pnum = 0;
	}
      else if (nparts > 0 && pnum == 0)
	{
	  fprintf(stderr, "%s: cannot specify whole medium"
		  " (has %d partition%s)\n", argv[0], nparts,
		  nparts == 1 ? "" : "s");
	  return 1;
	}
      else if (nparts > 0 && pnum > nparts)
	{
	  fprintf(stderr, "%s: invalid partition number (only %d available)\n",
		  argv[0], nparts);
	  return 1;
	}
    }
  else
    {
      if (nparts > 1)
	{
	  fprintf(stderr, "%s: must specify partition number (%d available)\n",
		  argv[0], nparts);
	  return 1;
	}
      else if (nparts == -1)
	pnum = 0;
      else
	pnum = 1;
    }

  v_init(&vol, HFS_OPT_NOCACHE);

  if (REPAIR)
    {
      suid_enable();
      result = v_open(&vol, path, HFS_MODE_RDWR);
      suid_disable();

      if (result == -1)
	{
	  vol.flags |= HFS_VOL_READONLY;

	  suid_enable();
	  result = v_open(&vol, path, HFS_MODE_RDONLY);
	  suid_disable();
	}
    }

  if (result == -1)
    {
      perror(path);
      return 1;
    }

  if (REPAIR && (vol.flags & HFS_VOL_READONLY))
    {
      fprintf(stderr, "%s: warning: %s not writable; cannot repair\n",
	      argv[0], path);

      options &= ~HFSCK_REPAIR;
    }

  /* Detect filesystem type and handle HFS+ journaling */
  if (force_fs_type != 0) {
    int fd;
    hfs_fs_type_t detected_type;
    
    suid_enable();
    fd = open(path, REPAIR ? O_RDWR : O_RDONLY);
    suid_disable();
    
    if (fd >= 0) {
      detected_type = hfs_detect_fs_type(fd);
      
      if (force_fs_type == 1 && detected_type != FS_TYPE_HFS) {
        fprintf(stderr, "%s: %s is not an HFS filesystem\n", argv[0], path);
        close(fd);
        return 1;
      } else if (force_fs_type == 2 && detected_type != FS_TYPE_HFSPLUS && detected_type != FS_TYPE_HFSX) {
        fprintf(stderr, "%s: %s is not an HFS+ filesystem\n", argv[0], path);
        close(fd);
        return 1;
      }
      
      if (VERBOSE) {
        const char *fs_name = (detected_type == FS_TYPE_HFS) ? "HFS" :
                             (detected_type == FS_TYPE_HFSPLUS) ? "HFS+" :
                             (detected_type == FS_TYPE_HFSX) ? "HFSX" : "Unknown";
        printf("Detected filesystem: %s\n", fs_name);
      }
      
      /* Handle HFS+ journaling if detected */
      if (detected_type == FS_TYPE_HFSPLUS || detected_type == FS_TYPE_HFSX) {
        struct HFSPlus_VolumeHeader vh;
        
        /* Read HFS+ Volume Header */
        if (lseek(fd, 1024, SEEK_SET) != -1 && 
            read(fd, &vh, sizeof(vh)) == sizeof(vh)) {
          
          uint32_t attributes = be32toh(vh.attributes);
          
          if (attributes & HFSPLUS_VOL_JOURNALED) {
            if (VERBOSE) {
              printf("HFS+ volume has journaling enabled\n");
            }
            
            /* Check journal validity */
            int journal_status = journal_is_valid(fd, &vh);
            
            if (journal_status < 0) {
              fprintf(stderr, "%s: warning: journal is corrupt\n", argv[0]);
              
              if (REPAIR) {
                if (VERBOSE) {
                  printf("Disabling corrupt journal\n");
                }
                journal_disable(fd, &vh);
              }
            } else if (journal_status > 0) {
              /* Replay journal transactions */
              if (VERBOSE) {
                printf("Replaying journal transactions\n");
              }
              
              int replay_result = journal_replay(fd, &vh, REPAIR);
              
              if (replay_result != 0) {
                fprintf(stderr, "%s: warning: journal replay failed\n", argv[0]);
                
                if (REPAIR) {
                  if (VERBOSE) {
                    printf("Disabling problematic journal\n");
                  }
                  journal_disable(fd, &vh);
                }
              } else if (VERBOSE) {
                printf("Journal replay completed successfully\n");
              }
            }
          } else if (VERBOSE) {
            printf("HFS+ volume does not have journaling enabled\n");
          }
        }
      }
      
      close(fd);
    }
  }

  if (v_geometry(&vol, pnum) == -1 ||
      l_getmdb(&vol, &vol.mdb, 0) == -1)
    {
      perror(path);
      v_close(&vol);
      return 1;
    }

  result = hfsck(&vol);

  vol.flags |= HFS_VOL_MOUNTED;

  if (v_close(&vol) == -1)
    {
      perror("closing volume");
      return 1;
    }

  return result;
}
