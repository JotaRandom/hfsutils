/*
 * stubs.c - Stub functions for mkfs.hfs embedded library
 * Simplified implementations for standalone mkfs.hfs utility
 * Copyright (C) 1996-1998 Robert Leslie
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <endian.h>
#include "libhfs.h"

/* HFS+ Volume attributes */
#define HFSPLUS_VOL_JOURNALED   0x00002000

/* HFS+ Volume Header structure (simplified) */
struct HFSPlus_VolumeHeader {
    uint16_t signature;
    uint16_t version;
    uint32_t attributes;
    uint32_t lastMountedVersion;
    uint32_t journalInfoBlock;
    /* ... other fields ... */
};

/* Stub implementations for missing functions */

void f_init(hfsfile *file, hfsvol *vol, unsigned long cnid, const char *name)
{
    /* Simplified initialization */
    file->vol = vol;
    file->parid = 0;
    file->cat.u.fil.filFlNum = cnid;
    strcpy(file->name, name);
}

int r_unpackextkey(const byte *key, ExtKeyRec *rec)
{
    /* Stub implementation */
    return 0;
}

int r_compareextkeys(const ExtKeyRec *key1, const ExtKeyRec *key2)
{
    /* Stub implementation */
    return 0;
}

int r_unpackcatkey(const byte *key, CatKeyRec *rec)
{
    /* Stub implementation */
    return 0;
}

int r_comparecatkeys(const CatKeyRec *key1, const CatKeyRec *key2)
{
    /* Stub implementation */
    return 0;
}

void *l_open(const char *path, int mode)
{
    /* Simplified file opening */
    int fd = open(path, (mode == HFS_MODE_RDWR) ? O_RDWR : O_RDONLY);
    if (fd == -1)
        return 0;
    return (void *)(long)fd;
}

int l_close(hfsvol *vol)
{
    if (vol->priv)
    {
        int fd = (int)(long)vol->priv;
        close(fd);
        vol->priv = 0;
    }
    return 0;
}

unsigned long l_size(hfsvol *vol)
{
    if (!vol->priv)
        return 0;
    
    int fd = (int)(long)vol->priv;
    struct stat st;
    if (fstat(fd, &st) == -1)
        return 0;
    
    return st.st_size / HFS_BLOCKSZ;
}

int l_same(hfsvol *vol, const char *path)
{
    /* Simplified path comparison */
    return 0;
}

int m_findpmentry(hfsvol *vol, const char *type, Partition *map, unsigned long *bnum)
{
    /* For simplicity, assume no partitions */
    return 0;
}

int l_putblocks(hfsvol *vol, unsigned long start, unsigned int count, const block *blocks)
{
    if (!vol->priv)
        return -1;
    
    int fd = (int)(long)vol->priv;
    off_t offset = start * HFS_BLOCKSZ;
    
    if (lseek(fd, offset, SEEK_SET) == -1)
        return -1;
    
    if (write(fd, blocks, count * HFS_BLOCKSZ) != (ssize_t)(count * HFS_BLOCKSZ))
        return -1;
    
    return 0;
}

int l_putmdb(hfsvol *vol, const MDB *mdb, int backup)
{
    if (!vol->priv)
        return -1;
    
    int fd = (int)(long)vol->priv;
    off_t offset = backup ? (vol->vlen - 2) * HFS_BLOCKSZ : 2 * HFS_BLOCKSZ;
    
    if (lseek(fd, offset, SEEK_SET) == -1)
        return -1;
    
    if (write(fd, mdb, sizeof(MDB)) != sizeof(MDB))
        return -1;
    
    return 0;
}

int v_mount(hfsvol *vol)
{
    /* Simplified mount - just mark as mounted */
    vol->flags |= HFS_VOL_MOUNTED;
    return 0;
}

unsigned long d_ltime(unsigned long mtime)
{
    /* Convert MacOS time to Unix time */
    const unsigned long mac_unix_offset = 2082844800UL;
    
    if (mtime < mac_unix_offset)
        return 0;
    
    return mtime - mac_unix_offset;
}

int l_getmdb(hfsvol *vol, MDB *mdb, int backup)
{
    if (!vol->priv)
        return -1;
    
    int fd = (int)(long)vol->priv;
    off_t offset = backup ? (vol->vlen - 2) * HFS_BLOCKSZ : 2 * HFS_BLOCKSZ;
    
    if (lseek(fd, offset, SEEK_SET) == -1)
        return -1;
    
    if (read(fd, mdb, sizeof(MDB)) != sizeof(MDB))
        return -1;
    
    return 0;
}

/* Additional stub functions for linking */

const char *hfs_error = "no error";  /* Global error variable */

int hfs_close(hfsfile *file)
{
    /* Stub implementation */
    return 0;
}

int hfs_closedir(hfsdir *dir)
{
    /* Stub implementation */
    if (dir) {
        free(dir);
    }
    return 0;
}

/* Additional missing functions */

int l_getblock(void *priv, unsigned long block_num, void *buffer)
{
    if (!priv || !buffer)
        return -1;
    
    int fd = (int)(long)priv;
    off_t offset = block_num * HFS_BLOCKSZ;
    
    if (lseek(fd, offset, SEEK_SET) == -1)
        return -1;
    
    if (read(fd, buffer, HFS_BLOCKSZ) != HFS_BLOCKSZ)
        return -1;
    
    return 0;
}

int l_putblock(void *priv, unsigned long block_num, const void *buffer)
{
    if (!priv || !buffer)
        return -1;
    
    int fd = (int)(long)priv;
    off_t offset = block_num * HFS_BLOCKSZ;
    
    if (lseek(fd, offset, SEEK_SET) == -1)
        return -1;
    
    if (write(fd, buffer, HFS_BLOCKSZ) != HFS_BLOCKSZ)
        return -1;
    
    return 0;
}

int bt_readhdr(btree *bt)
{
    /* Simplified B-tree header reading */
    if (!bt || !bt->f.vol)
        return -1;
    
    /* Read first node which contains the header */
    node n;
    n.bt = bt;
    n.nnum = 0;
    
    /* Calculate node offset */
    unsigned long node_size = 512; /* Default node size */
    off_t offset = n.nnum * node_size;
    
    /* Read node data directly */
    if (l_getblock(n.bt->f.vol->priv, offset / HFS_BLOCKSZ, n.data) == -1)
        return -1;
    
    /* Parse node descriptor */
    memcpy(&n.nd, n.data, sizeof(NodeDescriptor));
    
    /* Copy header from node data */
    memcpy(&bt->hdr, n.data + sizeof(NodeDescriptor), sizeof(BTHdrRec));
    
    return 0;
}

int bt_getnode(node *n)
{
    /* Simplified node reading */
    if (!n || !n->bt || !n->bt->f.vol)
        return -1;
    
    /* Calculate node offset */
    unsigned long node_size = 512; /* Default node size */
    if (n->bt->hdr.bthNodeSize > 0)
        node_size = n->bt->hdr.bthNodeSize;
    
    off_t offset = n->nnum * node_size;
    
    /* Read node data */
    if (l_getblock(n->bt->f.vol->priv, offset / HFS_BLOCKSZ, n->data) == -1)
        return -1;
    
    /* Parse node descriptor */
    memcpy(&n->nd, n->data, sizeof(NodeDescriptor));
    
    return 0;
}

int bt_putnode(node *n)
{
    /* Simplified node writing */
    if (!n || !n->bt || !n->bt->f.vol)
        return -1;
    
    /* Update node descriptor in data */
    memcpy(n->data, &n->nd, sizeof(NodeDescriptor));
    
    /* Calculate node offset */
    unsigned long node_size = 512; /* Default node size */
    if (n->bt->hdr.bthNodeSize > 0)
        node_size = n->bt->hdr.bthNodeSize;
    
    off_t offset = n->nnum * node_size;
    
    /* Write node data */
    if (l_putblock(n->bt->f.vol->priv, offset / HFS_BLOCKSZ, n->data) == -1)
        return -1;
    
    return 0;
}

char *hfsutil_abspath(const char *path)
{
    /* Simple implementation - just duplicate the path */
    return strdup(path ? path : "");
}

void hfsutil_perror(const char *path)
{
    perror(path);
}

void hfsutil_pinfo(hfsvolent *ent)
{
    /* Stub implementation */
    if (ent) {
        printf("Volume: %s\n", ent->name);
    }
}

void hfsutil_unmount(hfsvol *vol, int *result)
{
    /* Stub implementation */
    if (result) *result = 0;
    hfs_umount(vol);
}

/* Journal management functions are now implemented in journal.c */

void f_selectfork(hfsfile *f, int fork)
{
    /* Simplified fork selection */
    if (f) {
        f->fork = fork;
    }
}