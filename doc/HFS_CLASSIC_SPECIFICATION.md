# HFS Classic (Hierarchical File System) Specification

**Source:** Wikipedia and Apple Developer Documentation  
**URL:** https://en.wikipedia.org/wiki/Hierarchical_File_System_(Apple)  
**Also known as:** HFS Standard, Mac OS Standard

---

## Overview

Hierarchical File System (HFS) is a proprietary file system developed by Apple Inc. for use in computer systems running Mac OS. Originally designed for use on floppy and hard disks, it can also be found on read-only media such as CD-ROMs.

**Historical Timeline:**
- **September 1985**: Introduced with Hard Disk 20 for Macintosh
- **January 1986**: Included in Macintosh Plus 128K ROM
- **1998**: Succeeded by HFS Plus (HFS+)
- **Mac OS X**: HFS volumes cannot be used for booting
- **Mac OS X 10.6 (2009)**: HFS volumes become read-only
- **macOS Sierra 10.12 (2016)**: HFS Standard officially unsupported
- **macOS 10.15 Catalina (2019)**: HFS Standard completely removed (35 years of support ended)

---

## Design Overview

An HFS volume consists of five main structures:

### 1. Boot Blocks (Logical Blocks 0-1)

- **Location:** Bytes 0-1023
- **Size:** 1024 bytes (2 × 512-byte sectors)
- **Purpose:** Contains system startup information
- **Contents:**
  - Names of System and Shell files (usually Finder)
  - Boot code for startup

### 2. Master Directory Block (MDB)

- **Location:** Logical block 2 (byte offset 1024)
- **Size:** 512 bytes
- **Signature:** 0x4244 ('BD' in ASCII)
- **Purpose:** Defines volume metadata
- **Contains:**
  - Date & time stamps (creation, modification, backup)
  - Locations of other volume structures
  - Size of logical structures (allocation blocks)
  - Volume name and attributes

**Critical:** The MDB is the primary control structure for the entire volume.

### 3. Alternate Master Directory Block

- **Location:** Second-to-last logical block = **file_size - 1024 bytes**
- **Size:** 512 bytes
- **Signature:** 0x4244 ('BD' in ASCII)
- **Purpose:** Backup copy of MDB for disk repair
- **Update Policy:** Updated only when Catalog File or Extents Overflow File grow in size

### 4. Volume Bitmap

- **Location:** Starting at logical block 3
- **Purpose:** Tracks allocation block usage
- **Structure:** One bit per allocation block
  - Bit = 1: Block is in use
  - Bit = 0: Block is free
- **Size:** Variable, depends on total allocation blocks
  - For 65,535 blocks: ~8 KB (65,535 bits = 8,192 bytes)

### 5. Extent Overflow File

- **Structure:** B-tree
- **Purpose:** Records which allocation blocks belong to which files
- **Used when:** Initial three extents in Catalog File are exhausted
- **Additional feature:** Can store bad block extents to prevent allocation

### 6. Catalog File

- **Structure:** B-tree
- **Purpose:** Stores records for all files and directories
- **Indexed by:** Catalog Node ID (CNID)

**Record Types:**

| Record Type | Contents |
|-------------|----------|
| File Thread Record | File name, parent directory CNID |
| File Record | CNID, file size, timestamps, extents, Finder info |
| Directory Thread Record | Directory name, parent CNID |
| Directory Record | CNID, item count, timestamps, Finder info |

---

## Master Directory Block (MDB) Structure

All values stored in **big-endian** byte order.

### Key Fields

| Offset | Size | Field Name | Description |
|--------|------|------------|-------------|
| +0 | 2 | drSigWord | Signature: 0x4244 ('BD') |
| +2 | 4 | drCrDate | Volume creation date |
| +6 | 4 | drLsMod | Date/time of last modification |
| +10 | 2 | drAtrb | Volume attributes |
| +12 | 2 | drNmFls | Number of files in root directory |
| +14 | 2 | drVBMSt | First block of volume bitmap |
| +16 | 2 | drAllocPtr | Start of next allocation search |
| +18 | 2 | drNmAlBlks | Number of allocation blocks |
| +20 | 4 | drAlBlkSiz | Size of allocation blocks (bytes) |
| +24 | 4 | drClpSiz | Default clump size |
| +28 | 2 | drAlBlSt | First allocation block |
| +30 | 4 | drNxtCNID | Next unused catalog node ID |
| +34 | 2 | drFreeBks | Number of free allocation blocks |
| +36 | 28 | drVN | Volume name (Pascal string, 27 chars max) |
| +64 | 4 | drVolBkUp | Date/time of last backup |
| +68 | 2 | drVSeqNum | Volume backup sequence number |
| +70 | 4 | drWrCnt | Volume write count |
| +74 | 4 | drXTClpSiz | Extents file clump size |
| +78 | 4 | drCTClpSiz | Catalog file clump size |
| +82 | 2 | drNmRtDirs | Number of directories in root |
| +84 | 4 | drFilCnt | Number of files on volume |
| +88 | 4 | drDirCnt | Number of directories on volume |
| +92 | 32 | drFndrInfo | Finder information |
| ... | ... | ... | (additional fields) |

### Important Field Values

#### drAtrb (Volume Attributes, offset +10)

| Bit | Meaning |
|-----|---------|
| 8 | kHFSVolumeUnmountedBit (0x0100): Volume unmounted cleanly |
| 9 | kHFSVolumeSparedBlocksBit: Volume has bad blocks spared |
| 10 | kHFSVolumeNoCacheRequiredBit: No cache required |
| 11 | kHFSBootVolumeInconsistentBit: Boot volume is inconsistent |
| 12 | kHFSCatalogNodeIDsReusedBit: Catalog node IDs reused |
| 15 | kHFSVolumeSoftwareLockBit: Volume is software locked |

**Default for new volume:** 0x0100 (unmounted bit set)

#### drNxtCNID (Next Catalog Node ID, offset +30)

- **Purpose:** Next unused CNID for new files/folders
- **Value:** Must be >= 16
- **Reserved CNIDs:**
  - 1: Root folder parent
  - 2: Root folder
  - 3: Extents overflow file
  - 4: Catalog file
  - 5: Bad block file
  - 6-15: Reserved for future use

---

## Allocation Blocks

HFS groups 512-byte logical blocks into larger **allocation blocks**:

- **Addressing:** 16-bit (range: 0-65,535)
- **Maximum blocks:** 65,535 per volume
- **Block size:** Variable, depends on volume size
  - Calculated as: volume_size / 65,535
  - Minimum: 512 bytes (1 logical block)
  - Maximum: ~32 KB (typical for 2 GB volumes)

### Allocation Block Size Examples

| Volume Size | Allocation Block Size | Minimum File Size |
|-------------|----------------------|-------------------|
| 32 MB | 512 bytes | 512 bytes |
| 256 MB | 4 KB | 4 KB |
| 1 GB | 16 KB | 16 KB |
| 2 GB | 32 KB | 32 KB |

**Problem:** On a 1 GB disk, even a 1-byte file wastes 16 KB of space. This led to partitioning strategies to reduce waste.

---

## Filenames

- **Encoding:** MacRoman (8-bit character set)
- **Maximum length:** 31 characters
- **Forbidden character:** Colon (:) - used as path separator
- **Case:** Case-preserving but case-insensitive
- **Null characters:** Discouraged but technically allowed

---

## B-tree Structure

HFS uses B-trees for the Catalog File and Extents Overflow File.

### B-tree Characteristics

- **Node size:** 512 bytes (fixed)
- **Branching factor:** Varies based on key size
- **Order:** Sorted by key comparison
- **Performance:** O(log n) lookup time

### Node Types

| Type | Description |
|------|-------------|
| Header Node | Node 0, contains B-tree metadata |
| Index Node | Contains pointers to other nodes |
| Leaf Node | Contains actual data records |
| Map Node | Bitmap of free/used nodes |

---

## Date/Time Format

HFS uses **Mac absolute time**: unsigned 32-bit seconds since midnight, January 1, 1904 GMT.

**Time Range:**
- Minimum: January 1, 1904 00:00:00
- Maximum: February 6, 2040 06:28:15 (32-bit overflow)

**Conversion to Unix time:**
```c
unix_time = mac_time - 2082844800
```

**Conversion from Unix time:**
```c
mac_time = unix_time + 2082844800
```

---

## Limitations

### 1. Single Catalog File Bottleneck

- **Problem:** All file/directory records in one B-tree
- **Impact:** Only one process can write at a time in multitasking OS
- **Consequence:** Performance degradation under heavy I/O

### 2. 65,535 File Limit

- **Cause:** 16-bit allocation block addressing
- **Impact:** Maximum 65,535 files per volume (regardless of size)
- **Workaround:** Partition large disks into multiple volumes

### 3. Allocation Block Waste

- **Cause:** Fixed number of allocation blocks (65,535)
- **Impact:** Large allocation blocks on large volumes
- **Example:** 16 KB minimum file size on 1 GB volume

### 4. Reliability Concerns

- **Problem:** Damage to Catalog File can destroy entire filesystem
- **Contrast:** Other filesystems (FAT, UFS) distribute metadata
- **Mitigation:** Alternate MDB provides some recovery capability

### 5. No Built-in Journaling

- **Problem:** No transaction log for crash recovery
- **Impact:** Inconsistent state after power failure
- **Solution:** Requires manual fsck on next boot

---

## Comparison with HFS Plus

| Feature | HFS Classic | HFS Plus |
|---------|-------------|----------|
| **Introduction** | 1985 | 1998 |
| **Structure Name** | Master Directory Block | Volume Header |
| **Signature** | 0x4244 ('BD') | 0x482B ('H+') |
| **Allocation Blocks** | 16-bit (65,535 max) | 32-bit (4 billion max) |
| **Filenames** | 31 chars MacRoman | 255 chars Unicode |
| **Max Volume Size** | 2 TB | 8 EB (exabytes) |
| **Max File Size** | 2 GB | 8 EB |
| **Catalog Node Size** | 512 bytes | 4 KB |
| **Alternate Location** | file_size - 1024 | file_size - 1024 |
| **Date Range** | 1904-2040 | 1904-2040 |
| **Journaling** | No | Optional |
| **Unicode Support** | No | Yes (UTF-16) |

---

## Implementation Notes

### Creating an HFS Volume

1. **Write Boot Blocks** (bytes 0-1023)
   - Can be empty or contain boot code

2. **Write Master Directory Block** at offset 1024:
   - Set drSigWord = 0x4244
   - Set drAtrb = 0x0100 (unmounted bit)
   - Set drNxtCNID = 16
   - Calculate and set drNmAlBlks, drAlBlkSiz
   - Set volume name in drVN
   - Initialize timestamps

3. **Create Volume Bitmap** starting at block 3:
   - Set bits for system areas (boot, MDB, bitmap itself)
   - Clear bits for free space

4. **Initialize Extents Overflow File**:
   - Create B-tree with 512-byte nodes
   - Set up header node

5. **Initialize Catalog File**:
   - Create B-tree with 512-byte nodes
   - Add root folder record (CNID = 2)
   - Add thread records

6. **Write Alternate MDB** at offset (file_size - 1024):
   - Exact copy of primary MDB

### Common Errors

❌ **Wrong alternate MDB offset:**
```c
// INCORRECT - uses sector count
offset = (total_sectors - 1) * sector_size;
```

✅ **Correct alternate MDB offset:**
```c
// CORRECT - 1024 bytes before end
offset = file_size - 1024;
```

❌ **drNxtCNID = 0:**
- Must be >= 16 (first user CNID)

❌ **drAtrb = 0:**
- Should be 0x0100 (unmounted bit set)

❌ **Wrong node size:**
- HFS uses 512-byte nodes, not 4096 like HFS+

---

## References

- **Inside Macintosh: Files** - Apple's original HFS documentation
- **The HFS Primer** (MWJ, May 25, 2003) - Detailed structure guide
- **Practical File System Design** (Dominic Giampaolo, 1999) - Critique of HFS design
- **hfsutils** - Open-source tools for HFS manipulation

---

## Legacy Status

**macOS Support Timeline:**
- **Mac OS X 10.6 (2009)**: Read-only support only
- **macOS 10.12 (2016)**: Officially deprecated
- **macOS 10.15 (2019)**: Completely removed

**Modern Alternatives:**
- HFS Plus (HFS+) - Replaced HFS in 1998
- APFS (Apple File System) - Replaced HFS+ in 2017

---

*This document is a reference extracted from Wikipedia and Apple documentation for offline use in the hfsutils project.*
