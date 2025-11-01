/*
 * mount_hfs.h - Header file for mount.hfs embedded dependencies
 * Copyright (C) 1996-1998 Robert Leslie
 */

#ifndef MOUNT_HFS_H
#define MOUNT_HFS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

/* Include only the essential libhfs header */
#include "../shared/libhfs.h"

/* Include common utilities */
#include "../shared/hfs_detect.h"
#include "../shared/version.h"
#include "../shared/suid.h"
#include "../shared/common_utils.h"
#include "../shared/device_utils.h"
#include "../shared/error_utils.h"

/* Redefine ERROR macro to not use goto */
#undef ERROR
#define ERROR(code, str) do { hfs_error = (str); errno = (code); } while (0)

/* Mount options */
#define MOUNT_VERBOSE   0x0001
#define MOUNT_READONLY  0x0002
#define MOUNT_RECORD    0x0004

/* Volume management functions */
void v_init(hfsvol *vol, int flags);
int v_open(hfsvol *vol, const char *path, int mode);
int v_geometry(hfsvol *vol, int pnum);
int v_close(hfsvol *vol);
int getvol(hfsvol **vol);

/* HFS mounting functions */
int hfs_mount_volume(const char *path, int partno, int mount_options);

/* Mount recording function */
int hcwd_mounted(const char *vname, unsigned long crdate, const char *path, int partno);

/* Global variables */
extern const char *argv0, *bargv0;

/* Error handling */
extern const char *hfs_error;

/* Additional type definitions */
typedef unsigned char block[HFS_BLOCKSZ];
typedef unsigned char byte;

#endif /* MOUNT_HFS_H */