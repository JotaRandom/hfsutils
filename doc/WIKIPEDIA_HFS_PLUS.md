# HFS Plus (HFS+) - Wikipedia Reference

**Source:** Wikipedia, the free encyclopedia  
**URL:** https://en.wikipedia.org/wiki/HFS_Plus  
**Also known as:** HFS Extended, Mac OS Extended, HFS+

---

## Overview

HFS Plus (HFS+) is a file system developed by Apple Inc. as the successor to HFS (Hierarchical File System). It was introduced in Mac OS 8.1 and was the primary file system for macOS until it was replaced by APFS in macOS High Sierra (2017).

**Timeline:**
- **January 19, 1998**: Introduced with Mac OS 8.1
- **1998-2017**: Primary filesystem for Mac OS and macOS
- **2017**: Replaced by APFS in macOS High Sierra 10.13
- **Still used**: For magnetic drives, Time Machine, and legacy systems

---

## History

### Development Background

- **Motivation**: Address limitations of HFS Classic
  - Support for larger volumes (beyond 2 TB)
  - More efficient space allocation
  - Unicode filename support

- **Design Goals**:
  - Backward compatibility with HFS
  - Improved performance
  - Better reliability
  - International character support

### Evolution

- **Mac OS 8.1 (1998)**: Initial release
- **Mac OS X 10.2.2 (2002)**: Added journaling support
- **Mac OS X 10.3 (2003)**: Journaling enabled by default
- **OS X 10.6 (2009)**: HFS Classic deprecated
- **macOS 10.13 (2017)**: APFS becomes default for SSDs
- **macOS 10.15 (2019)**: APFS required for boot volumes

---

## Design

An HFS+ volume consists of nine structures:

### 1. Boot Blocks

- **Location**: Sectors 0-1 (first 1024 bytes)
- **Purpose**: System startup information
- **Content**: Boot loader code (for non-Mac systems)

### 2. Volume Header

- **Location**: Sector 2 (byte offset 1024)
- **Size**: 512 bytes
- **Signature**: 0x482B ('H+') or 0x4858 ('HX' for case-sensitive)
- **Purpose**: Volume metadata and pointers to other structures

### 3. Allocation File

- **Type**: Regular file with special CNID (6)
- **Purpose**: Bitmap tracking allocation block usage
- **Structure**: One bit per allocation block

### 4. Catalog File

- **Type**: B-tree file with CNID 4
- **Purpose**: Database of all files and folders
- **Node Size**: 4 KB (default), 8 KB on modern macOS
- **Keys**: Unicode strings (up to 255 characters)

### 5. Extents Overflow File

- **Type**: B-tree file with CNID 3
- **Purpose**: Additional extent records for fragmented files
- **Used when**: File has more than 8 extents

### 6. Attributes File

- **Type**: B-tree file with CNID 8
- **Purpose**: Extended attributes (metadata)
- **Examples**: Resource forks, ACLs, Finder info

### 7. Startup File

- **Type**: Regular file with CNID 7
- **Purpose**: Boot loader for non-Mac operating systems
- **Usage**: Rarely used

### 8. Alternate Volume Header

- **Location**: **Second-to-last sector** (file_size - 1024 bytes)
- **Size**: 512 bytes
- **Purpose**: Backup copy of Volume Header
- **Critical**: Used for volume repair and recovery

### 9. Reserved Area

- **Location**: Last 512 bytes of volume
- **Purpose**: Reserved for future use

---

## Technical Specifications

### Volume Limits

| Specification | HFS Classic | HFS Plus |
|---------------|-------------|----------|
| **Max Volume Size** | 2 TB | 8 EB (exabytes) = 2^63 bytes |
| **Max File Size** | 2 GB | 8 EB (theoretically) |
| **Max Files** | 65,535 | ~4 billion (limited by 32-bit CNID) |
| **Allocation Blocks** | 16-bit addressing | 32-bit addressing |
| **Filename Length** | 31 bytes | 255 Unicode characters |
| **Character Encoding** | MacRoman | UTF-16 (Unicode) |

### Allocation Blocks

- **Size**: Variable, based on volume size
  - Minimum: Same as sector size (usually 512 bytes)
  - Maximum: Calculated to keep total blocks < 2^32
- **Addressing**: 32-bit (vs 16-bit in HFS)
- **Efficiency**: Significantly reduced waste on large volumes

### Filenames

- **Encoding**: UTF-16 Unicode
- **Maximum length**: 255 characters (not bytes)
- **Normalization**: Uses Unicode decomposition
- **Forbidden characters**: Colon (:) and NULL
- **Case sensitivity**:
  - HFS+: Case-preserving, case-insensitive
  - HFSX: Case-sensitive variant

---

## Volume Header Structure

Located at byte offset 1024, size 512 bytes.

### Critical Fields

| Field | Offset | Size | Description |
|-------|--------|------|-------------|
| signature | +0 | 2 | 0x482B ('H+') or 0x4858 ('HX') |
| version | +2 | 2 | 4 for HFS+, 5 for HFSX |
| attributes | +4 | 4 | Volume attributes bitmap |
| blockSize | +40 | 4 | Allocation block size in bytes |
| totalBlocks | +44 | 4 | Total allocation blocks |
| freeBlocks | +48 | 4 | Free allocation blocks |
| nextAllocation | +52 | 4 | Hint for next allocation |
| rsrcClumpSize | +56 | 4 | Default resource fork clump |
| dataClumpSize | +60 | 4 | Default data fork clump |
| nextCatalogID | +64 | 4 | Next unused CNID (>= 16) |

### Attributes Field (offset +4)

| Bit | Name | Value | Meaning |
|-----|------|-------|---------|
| 7 | kHFSVolumeJournaledBit | 0x2000 | Journaled volume |
| 8 | kHFSVolumeUnmountedBit | 0x0100 | Cleanly unmounted |
| 9 | kHFSBootVolumeInconsistentBit | 0x0200 | Needs repair |
| 15 | kHFSVolumeSoftwareLockBit | 0x8000 | Software locked |

**Note:** A newly created volume should have attributes = 0x0100 (unmounted bit set).

---

## Alternate Volume Header Location

**Critical Formula:**
```
offset = volume_size_in_bytes - 1024
```

**NOT:**
```c
offset = (total_sectors - 1) * sector_size;  // WRONG
```

Wikipedia states: *"The second-to-last sector contains the Alternate Volume Header."*

For 512-byte sectors:
- Last sector: offset = file_size - 512
- **Second-to-last sector: offset = file_size - 1024** ✓

This matches Apple TN1150 exactly.

---

## Journaling

HFS+ introduced optional journaling in Mac OS X 10.2.2 (2002).

### Journal Benefits

- **Crash recovery**: Quick recovery after power failure
- **Consistency**: Ensures metadata consistency
- **Performance**: Faster fsck times

### Journal Structure

- **Location**: Separate journal file or embedded in volume
- **Size**: User-configurable (default: 8-32 MB)
- **Transactions**: Atomic metadata updates
- **Replay**: Automatic on mount after crash

### Implementation Note

When journaling is enabled:
- attributes field bit 7 is set (0x2000)
- journalInfoBlock points to journal location
- All metadata updates are logged before writing

---

## Catalog B-tree

### Node Structure

- **Default node size**: 4096 bytes (4 KB)
- **Modern macOS**: 8192 bytes (8 KB)
- **Header node**: Node 0, contains B-tree metadata
- **Leaf nodes**: Contain file/folder records
- **Index nodes**: Contain pointers for tree navigation

### Record Types

| Type | Value | Description |
|------|-------|-------------|
| kHFSPlusFolderRecord | 0x0001 | Folder metadata |
| kHFSPlusFileRecord | 0x0002 | File metadata |
| kHFSPlusFolderThreadRecord | 0x0003 | Parent folder lookup |
| kHFSPlusFileThreadRecord | 0x0004 | Parent folder lookup |

### Catalog Key

```c
struct HFSPlusCatalogKey {
    uint16_t keyLength;
    uint32_t parentID;        // Parent folder CNID
    HFSUniStr255 nodeName;    // Unicode filename
};
```

---

## Known Issues and Criticisms

### Design Limitations

1. **No checksums**: Metadata not protected from corruption
2. **32-bit timestamps**: Limited to 1904-2040 range
3. **No snapshots**: Unlike modern filesystems (ZFS, Btrfs)
4. **No native encryption**: Requires FileVault overlay
5. **No compression**: No built-in transparent compression
6. **No sparse files**: Inefficient for large sparse files

### Platform-Specific Issues

#### Linux

- **Kernel module**: hfsplus driver
- **Tools**: hfsprogs package
- **Journaling**: **Not fully supported** - can cause corruption
  - Recommendation: Disable journaling for Linux use
- **2TB bug**: Fixed in 2009 (kernel 2.6.29)

#### Windows

- **Read-only**: Native support limited
- **Third-party**: Boot Camp drivers, MacDrive, Paragon HFS+
- **Reliability**: Varies by driver quality

---

## Comparison with APFS

| Feature | HFS+ | APFS |
|---------|------|------|
| **Introduction** | 1998 | 2017 |
| **Optimization** | Magnetic drives | SSDs/Flash |
| **Snapshots** | No | Yes |
| **Cloning** | No | Yes (instant) |
| **Encryption** | No (needs FileVault) | Native |
| **Compression** | No | Yes |
| **Checksums** | No | Yes |
| **Space sharing** | No | Yes (containers) |
| **Timestamps** | 32-bit (1904-2040) | 64-bit (nanosecond) |

---

## Implementation Guidance

### Creating HFS+ Volume

1. Write boot blocks (0-1023 bytes)
2. Write Volume Header at 1024:
   - signature = 0x482B
   - version = 4
   - **attributes = 0x0100** (unmounted)
   - blockSize = calculated
   - totalBlocks = calculated
   - freeBlocks = calculated
   - **rsrcClumpSize** = blockSize * 4 (typical)
   - **dataClumpSize** = blockSize * 4 (typical)
   - **nextCatalogID = 16** (first user CNID)
3. Create Allocation File (bitmap)
4. Create Extents B-tree (4 KB nodes)
5. Create Catalog B-tree (4 KB nodes)
6. Add root folder to Catalog
7. **Write Alternate Volume Header** at (file_size - 1024)

### Common Mistakes

❌ Placing alternate header at last sector  
✅ Place at file_size - 1024 (second-to-last sector for 512-byte sectors)

❌ Setting nextCatalogID = 0  
✅ Set nextCatalogID = 16 (kHFSFirstUserCatalogNodeID)

❌ Missing rsrcClumpSize and dataClumpSize  
✅ Set both to reasonable values (4 × blockSize typical)

❌ attributes = 0x00000000  
✅ attributes = 0x00000100 (unmounted bit)

---

## Cross-Platform Compatibility

### macOS

- **Full support**: All macOS versions through 10.14
- **Read/write**: Complete implementation
- **Journaling**: Fully supported
- **Tools**: diskutil, fsck_hfs, newfs_hfs

### Linux

- **Module**: hfsplus.ko
- **Mount**: `mount -t hfsplus /dev/sdX /mnt`
- **Journaling**: **Disable for safety**
- **Tools**: fsck.hfsplus, mkfs.hfsplus, hfsprogs

### FreeBSD/NetBSD

- **Support**: Read-only in most cases
- **Reliability**: Limited testing

### Windows

- **Native**: Read-only (removed in modern versions)
- **Boot Camp**: Apple-provided drivers
- **Third-party**: MacDrive, Paragon HFS+

---

## References

### Official Documentation

- Apple Technical Note TN1150: HFS Plus Volume Format
- Inside Macintosh: Files (Legacy)
- File Manager Reference (macOS)

### Tools

- **hfsutils**: Robert Leslie's original HFS tools
- **hfsprogs**: Linux HFS+ utilities
- **diskutil**: macOS disk utility
- **DiskWarrior**: Commercial HFS+ repair tool

### Specifications

- Unicode Standard (for filename normalization)
- B-tree Algorithms (Knuth, Volume 3)
- File System Forensics (carrier, 2005)

---

## See Also

- HFS Classic (predecessor)
- APFS (successor)
- Comparison of file systems
- List of file systems

---

*This document is a reference extracted from Wikipedia for offline use in the hfsutils project. Last updated from Wikipedia content dated November 1, 2025.*
