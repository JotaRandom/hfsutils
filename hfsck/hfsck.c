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
 * $Id: hfsck.c,v 1.7 1998/04/11 08:27:08 rob Exp $
 */

# include <stdio.h>

# include "hfsck.h"

# include "ck_mdb.h"
# include "ck_volume.h"
# include "ck_btree.h"

/* Standard fsck exit codes */
#define FSCK_OK                 0   /* No errors found */
#define FSCK_CORRECTED          1   /* Errors found and corrected */
#define FSCK_REBOOT_REQUIRED    2   /* System should be rebooted */
#define FSCK_UNCORRECTED        4   /* Errors found but not corrected */
#define FSCK_OPERATIONAL_ERROR  8   /* Operational error */

/*
 * NAME:	hfsck()
 * DESCRIPTION:	perform comprehensive filesystem check on HFS/HFS+ volume
 *              Returns standard fsck exit codes for proper integration
 *              with Unix/Linux/BSD systems
 */
int hfsck(hfsvol *vol)
{
  int errors_found = 0;
  int errors_corrected = 0;
  int result;

  if (VERBOSE) {
    printf("*** Checking HFS volume '%s'\n", vol->mdb.drVN);
  }

  /* Check Master Directory Block (MDB) or Volume Header */
  result = ck_mdb(vol);
  if (result) {
    errors_found = 1;
    if (REPAIR) {
      errors_corrected = 1;
      if (VERBOSE) {
        printf("*** MDB/Volume Header errors corrected\n");
      }
    }
  }

  /* Check volume structure and allocation */
  result = ck_volume(vol);
  if (result) {
    errors_found = 1;
    if (REPAIR) {
      errors_corrected = 1;
      if (VERBOSE) {
        printf("*** Volume structure errors corrected\n");
      }
    }
  }

  /* Check extents overflow B-tree */
  result = ck_btree(&vol->ext);
  if (result) {
    errors_found = 1;
    if (REPAIR) {
      errors_corrected = 1;
      if (VERBOSE) {
        printf("*** Extents B-tree errors corrected\n");
      }
    }
  }

  /* Check catalog B-tree */
  result = ck_btree(&vol->cat);
  if (result) {
    errors_found = 1;
    if (REPAIR) {
      errors_corrected = 1;
      if (VERBOSE) {
        printf("*** Catalog B-tree errors corrected\n");
      }
    }
  }

  /* Return appropriate exit code based on results */
  if (!errors_found) {
    if (VERBOSE) {
      printf("*** Volume check completed: no errors found\n");
    }
    return FSCK_OK;
  } else if (errors_corrected && REPAIR) {
    if (VERBOSE) {
      printf("*** Volume check completed: errors found and corrected\n");
    }
    return FSCK_CORRECTED;
  } else {
    if (VERBOSE) {
      printf("*** Volume check completed: errors found but not corrected\n");
    }
    return FSCK_UNCORRECTED;
  }
}
