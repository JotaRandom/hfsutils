# Technical Note TN1150: HFS Plus Volume Format

**Source:** Apple Inc. Developer Documentation  
**Date:** March 5, 2004  
**URL:** https://developer.apple.com/library/archive/technotes/tn/tn1150.html

---

## Overview

This technical note describes the on-disk data structures of the HFS Plus volume format. HFS Plus is the default volume format used on Macintosh computers since Mac OS 8.1.

**Key Points:**
- HFS Plus uses Unicode for filenames (up to 255 UTF-16 characters)
- Supports volumes up to 8 exabytes (2^63 bytes)
- Uses 32-bit allocation block numbers (vs 16-bit in HFS)
- Default catalog node size is 4 KB (vs 512 bytes in HFS)
- B-tree based catalog and extents overflow files

---

## Volume Structure

An HFS Plus volume consists of the following structures in order:

1. **Reserved Boot Blocks** (1024 bytes at offset 0)
2. **Volume Header** (512 bytes at offset 1024)
3. **Allocation File**
4. **Extents Overflow File**
5. **Catalog File**
6. **Attributes File** (optional, for extended attributes)
7. **Startup File** (optional, for non-Mac OS systems)
8. **Alternate Volume Header** (512 bytes at offset = file_size - 1024)

### Critical Formula

**Alternate Volume Header Location:**
```
offset = volume_size_in_bytes - 1024
```

**NOT:** `(total_sectors - 1) * sector_size`

This is explicitly stated in TN1150:
> "A copy of the volume header, the alternate volume header, is stored starting 
> 1024 bytes before the end of the volume."

---

## Volume Header Structure

The Volume Header is 512 bytes and begins at byte offset 1024 from the start of the volume.

### Field Layout (all values in big-endian)

| Offset | Size | Field Name | Description |
|--------|------|------------|-------------|
| +0 | 2 | signature | Volume signature (0x482B = 'H+' or 0x4858 = 'HX') |
| +2 | 2 | version | Volume version (4 for HFS+, 5 for HFSX) |
| +4 | 4 | attributes | Volume attributes bitmap |
| +8 | 4 | lastMountedVersion | Signature of OS that last mounted |
| +12 | 4 | journalInfoBlock | Block number of journal info (0 if no journal) |
| +16 | 4 | createDate | Volume creation date (Mac absolute time) |
| +20 | 4 | modifyDate | Volume last modification date |
| +24 | 4 | backupDate | Volume last backup date |
| +28 | 4 | checkedDate | Volume last consistency check date |
| +32 | 4 | fileCount | Number of files on volume |
| +36 | 4 | folderCount | Number of folders on volume |
| +40 | 4 | blockSize | Size of allocation blocks in bytes |
| +44 | 4 | totalBlocks | Total allocation blocks on volume |
| +48 | 4 | freeBlocks | Number of unused allocation blocks |
| +52 | 4 | nextAllocation | Hint for next allocation block search |
| +56 | 4 | rsrcClumpSize | Default resource fork clump size |
| +60 | 4 | dataClumpSize | Default data fork clump size |
| +64 | 4 | nextCatalogID | Next unused catalog node ID |
| +68 | 4 | writeCount | Volume write count |
| +72 | 8 | encodingsBitmap | Bitmap of encodings used on volume |
| +80 | 32 | finderInfo[8] | Information used by Finder |
| +112 | 80 | allocationFile | Allocation bitmap file fork data |
| +192 | 80 | extentsFile | Extents overflow file fork data |
| +272 | 80 | catalogFile | Catalog file fork data |
| +352 | 80 | attributesFile | Attributes file fork data |
| +432 | 80 | startupFile | Startup file fork data |

Total size: 512 bytes

### Important Field Values

#### signature (offset +0)
- **0x482B ('H+')**: Standard HFS Plus volume
- **0x4858 ('HX')**: HFSX (case-sensitive) volume

#### attributes (offset +4)
Bitmap of volume attributes:

| Bit | Constant | Meaning |
|-----|----------|---------|
| 0 | Reserved | (not used) |
| 7 | kHFSVolumeJournaledBit | Volume has a journal (0x2000) |
| 8 | kHFSVolumeUnmountedBit | Volume was unmounted cleanly (0x0100) |
| 9 | kHFSBootVolumeInconsistentBit | Boot volume is inconsistent |
| 10-14 | Reserved | (not used) |
| 15 | kHFSVolumeSoftwareLockBit | Volume is software locked |

**Critical:** Bit 8 (0x0100) MUST be set when creating a new volume to indicate it was unmounted cleanly.

#### nextCatalogID (offset +64)
- The next unused catalog node ID
- **MUST be >= 16** (kHFSFirstUserCatalogNodeID)
- Catalog node IDs 0-15 are reserved:
  - 1 = Root folder parent
  - 2 = Root folder
  - 3 = Extents file
  - 4 = Catalog file
  - 5 = Bad blocks file
  - 6 = Allocation file
  - 7 = Startup file
  - 8 = Attributes file
  - 9 = Repair catalog file
  - 10 = Bogus extents file
  - 11-15 = Reserved

#### rsrcClumpSize and dataClumpSize (offsets +56, +60)
- Default clump sizes for resource and data forks
- Typical value: 4 * blockSize
- **Must NOT be zero** - these fields are required for proper Volume Header structure

---

## B-tree Structure

HFS Plus uses B-trees for the Catalog File and Extents Overflow File.

### B-tree Header Node

The first node (node 0) of every B-tree is the header node:

**BTHeaderRec Structure:**

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| +0 | 2 | treeDepth | Current depth of tree |
| +2 | 4 | rootNode | Node number of root node |
| +6 | 4 | leafRecords | Number of leaf records |
| +10 | 4 | firstLeafNode | Node number of first leaf |
| +14 | 4 | lastLeafNode | Node number of last leaf |
| +18 | 2 | nodeSize | Size of a node (typically 4096 bytes) |
| +20 | 2 | maxKeyLength | Maximum key length |
| +22 | 4 | totalNodes | Total nodes in tree |
| +26 | 4 | freeNodes | Number of unused nodes |
| +30 | 2 | reserved1 | Reserved |
| +32 | 4 | clumpSize | Clump size (obsolete) |
| +36 | 1 | btreeType | Type of B-tree |
| +37 | 1 | keyCompareType | Key comparison method |
| +38 | 4 | attributes | B-tree attributes |
| +42 | 64 | reserved3 | Reserved |

**Critical Values:**
- nodeSize: Typically 4096 (4 KB) for HFS+, can be 8192 on modern macOS
- btreeType: 0 = Catalog, 255 = Extents
- For Catalog: keyCompareType should be appropriate for case sensitivity

---

## Catalog File

The Catalog File contains records for all files and folders on the volume.

### Catalog Key Structure

**For Folders and Files:**
```c
struct HFSPlusCatalogKey {
    uint16_t keyLength;
    uint32_t parentID;        // CNID of parent folder
    HFSUniStr255 nodeName;    // Unicode name
};
```

**HFSUniStr255:**
```c
struct HFSUniStr255 {
    uint16_t length;          // Number of Unicode characters
    uint16_t unicode[255];    // Unicode characters (UTF-16)
};
```

### Catalog Record Types

| Type | Value | Description |
|------|-------|-------------|
| kHFSPlusFolderRecord | 0x0001 | Folder record |
| kHFSPlusFileRecord | 0x0002 | File record |
| kHFSPlusFolderThreadRecord | 0x0003 | Folder thread |
| kHFSPlusFileThreadRecord | 0x0004 | File thread |

---

## Extents Overflow File

Stores additional extent records when a file's fork has more than 8 extents.

### Extent Descriptor

```c
struct HFSPlusExtentDescriptor {
    uint32_t startBlock;      // First allocation block
    uint32_t blockCount;      // Number of allocation blocks
};
```

Each fork can have up to 8 extent descriptors in its ForkData structure. Additional extents are stored in the Extents Overflow File.

---

## Allocation File

The Allocation File is a bitmap that tracks which allocation blocks are in use:
- 1 bit per allocation block
- Bit set (1) = block is in use
- Bit clear (0) = block is free

---

## Date/Time Format

HFS Plus uses **Mac absolute time**: seconds since midnight, January 1, 1904 GMT.

**Conversion to Unix time:**
```
unix_time = mac_time - 2082844800
```

**Time Range:**
- Minimum: January 1, 1904
- Maximum: February 6, 2040 (32-bit overflow)

---

## Differences from HFS Classic

| Feature | HFS Classic | HFS Plus |
|---------|-------------|----------|
| Structure Name | Master Directory Block (MDB) | Volume Header |
| Signature | 0x4244 ('BD') | 0x482B ('H+') |
| Allocation Blocks | 16-bit (max 65,535) | 32-bit (max 4.2 billion) |
| Filenames | 31 bytes MacRoman | 255 UTF-16 characters |
| Max Volume Size | 2 TB | 8 EB (exabytes) |
| Max File Size | 2 GB | 8 EB |
| Catalog Node Size | 512 bytes | 4 KB (default) |
| Alternate Location | file_size - 1024 | file_size - 1024 |

**Note:** Both HFS and HFS+ place their alternate structures at the same location: 1024 bytes before the end of the volume.

---

## Implementation Notes

### When Creating a New HFS+ Volume:

1. **Reserve boot blocks** (0-1023 bytes)
2. **Write Volume Header** at offset 1024:
   - Set signature to 0x482B
   - Set version to 4
   - **Set attributes to 0x0100** (unmounted bit)
   - Set blockSize, totalBlocks, freeBlocks
   - **Set rsrcClumpSize and dataClumpSize** (typically 4 * blockSize)
   - **Set nextCatalogID to 16** (kHFSFirstUserCatalogNodeID)
   - Fill in fork data for system files
3. **Create Allocation File** B-tree
4. **Create Extents Overflow File** B-tree
5. **Create Catalog File** B-tree with root folder
6. **Write Alternate Volume Header** at offset (volume_size - 1024)
   - Exact copy of primary Volume Header

### Common Errors:

❌ **Wrong alternate header offset:**
```c
// INCORRECT
offset = (total_sectors - 1) * sector_size;
```

✅ **Correct alternate header offset:**
```c
// CORRECT
offset = volume_size_in_bytes - 1024;
```

❌ **Missing clump size fields:**
- Forgetting rsrcClumpSize and dataClumpSize causes nextCatalogID to be misaligned

❌ **Wrong nextCatalogID:**
- Must be >= 16, not 0

❌ **Missing unmounted bit:**
- attributes should be 0x0100, not 0x0000

---

## References

- **Inside Macintosh: Files** - Original HFS/HFS+ documentation
- **TN1150** - This technical note (authoritative HFS+ specification)
- **File Manager Reference** - API documentation for file operations
- **Unicode Normalization Forms** - Used for filename comparison in HFSX

---

## Revision History

- **March 5, 2004**: Original publication
- **Archived**: Now available in Apple Developer Archive

---

*This document is a reference extracted from Apple Technical Note TN1150 for offline use in the hfsutils project.*
