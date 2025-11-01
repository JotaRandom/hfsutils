/*
 * hfs_common.h - Common header for HFS embedded utilities
 * Unified header for mkfs.hfs, fsck.hfs, and mount.hfs
 * Copyright (C) 1996-1998 Robert Leslie
 */

#ifndef HFS_COMMON_H
#define HFS_COMMON_H

/* Standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

/* HFS constants */
#define HFS_BLOCKSZ		512
#define HFS_BLOCKSZ_BITS	9
#define HFS_MAX_FLEN		31
#define HFS_MAX_VLEN		27
#define HFS_SIGWORD		0x4244

/* HFS mode constants */
#define HFS_MODE_RDONLY		0
#define HFS_MODE_RDWR		1
#define HFS_MODE_ANY		2
#define HFS_MODE_MASK		3

/* HFS volume flags */
#define HFS_VOL_READONLY	0x0004
#define HFS_VOL_MOUNTED		0x0008
#define HFS_VOL_OPEN		0x0001

/* HFS options */
#define HFS_OPT_2048		0x0200
#define HFS_OPT_NOCACHE		0x0100
#define HFS_VOL_OPT_MASK	0xff00

/* HFS CNID constants */
#define HFS_CNID_ROOTPAR	1
#define HFS_CNID_ROOTDIR	2
#define HFS_CNID_EXT		3
#define HFS_CNID_CAT		4
#define HFS_CNID_BADALLOC	5

/* Volume update flags */
#define HFS_VOL_UPDATE_MDB	0x0010
#define HFS_VOL_UPDATE_ALTMDB	0x0020
#define HFS_VOL_UPDATE_VBM	0x0040

/* Basic type definitions */
typedef unsigned char block[HFS_BLOCKSZ];
typedef unsigned char byte;

/* Forward declarations */
typedef struct _hfsvol_ hfsvol;
typedef struct _hfsfile_ hfsfile;
typedef struct _hfsdir_ hfsdir;

/* Volume information structure */
typedef struct {
  char name[HFS_MAX_VLEN + 1];
  int flags;
  unsigned long totbytes;
  unsigned long freebytes;
  unsigned long alblocksz;
  unsigned long clumpsz;
  unsigned long numfiles;
  unsigned long numdirs;
  time_t crdate;
  time_t mddate;
  time_t bkdate;
  unsigned long blessed;
} hfsvolent;

/* Directory entry structure */
typedef struct {
  char name[HFS_MAX_FLEN + 1];
  int flags;
  unsigned long cnid;
  unsigned long parid;
  time_t crdate;
  time_t mddate;
  time_t bkdate;
  short fdflags;
  struct {
    signed short v;
    signed short h;
  } fdlocation;
  union {
    struct {
      unsigned long dsize;
      unsigned long rsize;
      char type[5];
      char creator[5];
    } file;
    struct {
      unsigned short valence;
      struct {
        signed short top;
        signed short left;
        signed short bottom;
        signed short right;
      } rect;
    } dir;
  } u;
} hfsdirent;

/* Apple data structures (simplified) */
typedef struct {
  unsigned short xdrStABN;
  unsigned short xdrNumABlks;
} ExtDescriptor;

typedef ExtDescriptor ExtDataRec[3];

/* Master Directory Block (simplified) */
typedef struct {
  unsigned short drSigWord;
  unsigned long drCrDate;
  unsigned long drLsMod;
  unsigned short drAtrb;
  unsigned short drNmFls;
  unsigned short drVBMSt;
  unsigned short drAllocPtr;
  unsigned short drNmAlBlks;
  unsigned long drAlBlkSiz;
  unsigned long drClpSiz;
  unsigned short drAlBlSt;
  unsigned long drNxtCNID;
  unsigned short drFreeBks;
  char drVN[HFS_MAX_VLEN + 1];
  unsigned long drVolBkUp;
  unsigned short drVSeqNum;
  unsigned long drWrCnt;
  unsigned long drXTClpSiz;
  unsigned long drCTClpSiz;
  unsigned short drNmRtDirs;
  unsigned long drFilCnt;
  unsigned long drDirCnt;
  unsigned long drFndrInfo[8];
  unsigned short drEmbedSigWord;
  ExtDescriptor drEmbedExtent;
  unsigned long drXTFlSize;
  ExtDataRec drXTExtRec;
  unsigned long drCTFlSize;
  ExtDataRec drCTExtRec;
} MDB;

/* Volume structure (simplified) */
struct _hfsvol_ {
  void *priv;
  int flags;
  int pnum;
  unsigned long vstart;
  unsigned long vlen;
  unsigned long lpa;
  void *cache;
  block *vbm;
  unsigned int vbmsz;
  MDB mdb;
  unsigned long cwd;
  int refs;
  hfsfile *files;
  hfsdir *dirs;
  hfsvol *prev;
  hfsvol *next;
};

/* Utility macros */
#define ALLOC(type, n)		((type *) malloc(sizeof(type) * (n)))
#define FREE(ptr)		((ptr) ? (void) free((void *) ptr) : (void) 0)
#define ERROR(code, str)	do { hfs_error = (str); errno = (code); } while (0)

/* Global variables */
extern const char *hfs_error;
extern const char *argv0, *bargv0;
extern hfsvol *hfs_mounts;
extern hfsvol *curvol;

/* Core HFS functions */
int hfs_format(const char *path, int pnum, int mode, const char *vname,
               unsigned int nbadblocks, const unsigned long badblocks[]);
hfsvol *hfs_mount(const char *path, int pnum, int mode);
int hfs_umount(hfsvol *vol);
int hfs_vstat(hfsvol *vol, hfsvolent *ent);
int hfs_nparts(const char *path);

/* Volume management functions */
void v_init(hfsvol *vol, int flags);
int v_open(hfsvol *vol, const char *path, int mode);
int v_geometry(hfsvol *vol, int pnum);
int v_flush(hfsvol *vol);
int v_close(hfsvol *vol);
int v_same(hfsvol *vol, const char *path);
int getvol(hfsvol **vol);

/* Low-level I/O functions */
void *l_open(const char *path, int mode);
int l_close(hfsvol *vol);
unsigned long l_size(hfsvol *vol);
int l_same(hfsvol *vol, const char *path);
int l_putblocks(hfsvol *vol, unsigned long start, unsigned int count, const block *blocks);
int l_putmdb(hfsvol *vol, const MDB *mdb, int backup);
int l_getmdb(hfsvol *vol, MDB *mdb, int backup);

/* Data conversion functions */
unsigned long d_mtime(time_t t);
unsigned long d_ltime(unsigned long mtime);

/* HFS utility functions */
void hfsutil_perror(const char *msg);
void hfsutil_perrorp(const char *path);
void hfsutil_pinfo(hfsvolent *ent);
void hfsutil_unmount(hfsvol *vol, int *result);
int hfsutil_samepath(const char *path1, const char *path2);
char *hfsutil_abspath(const char *path);

/* SUID privilege management */
void suid_init(void);
void suid_enable(void);
void suid_disable(void);

/* Version information */
extern const char hfsutils_version[];
extern const char hfsutils_copyright[];
extern const char hfsutils_license[];

#endif /* HFS_COMMON_H */