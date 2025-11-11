# HFS/HFS+ Documentation Index

**hfsutils Project Documentation**  
**Offline Reference Collection**

---

## Overview

This directory contains comprehensive offline documentation for HFS (Hierarchical File System) and HFS+ (HFS Plus) filesystems. All material has been extracted from authoritative sources for development reference without requiring internet access.

---

## Documentation Files

### 1. Apple Technical Note TN1150

**File:** [`TN1150_HFS_PLUS_VOLUME_FORMAT.md`](TN1150_HFS_PLUS_VOLUME_FORMAT.md)

**Source:** Apple Inc. Developer Documentation (March 5, 2004)

**Content:**
- Official HFS Plus Volume Format specification
- Complete Volume Header structure (512 bytes at offset 1024)
- Field-by-field breakdown with offsets and sizes
- B-tree structures (Catalog, Extents Overflow, Attributes)
- Alternate Volume Header location formula
- Critical constants (kHFSVolumeUnmountedBit, kHFSFirstUserCatalogNodeID, etc.)
- Differences from HFS Classic

**Key Topics:**
- Volume Header fields (signature, attributes, nextCatalogID, clump sizes)
- Catalog File structure
- Extents Overflow File
- Date/time format (Mac absolute time)
- Unicode filenames (UTF-16)

**Use When:**
- Implementing mkfs.hfs+ or fsck.hfs+
- Debugging HFS+ volume creation
- Validating Volume Header field values
- Understanding B-tree structures

---

### 2. HFS Classic Specification

**File:** [`HFS_CLASSIC_SPECIFICATION.md`](HFS_CLASSIC_SPECIFICATION.md)

**Sources:** Wikipedia + Apple Documentation

**Content:**
- Complete HFS Classic (HFS Standard) specification
- Master Directory Block (MDB) structure
- Five main volume structures (Boot, MDB, Bitmap, Extents, Catalog)
- Alternate MDB location and purpose
- Allocation block addressing (16-bit)
- MacRoman filenames (31 characters max)
- Historical context (1985-2019)

**Key Topics:**
- MDB fields (drSigWord, drAtrb, drNxtCNID)
- B-tree specifications (512-byte nodes)
- Limitations (65,535 file limit, allocation waste)
- Comparison with HFS+

**Use When:**
- Implementing mkfs.hfs or fsck.hfs
- Understanding HFS vs HFS+ differences
- Debugging HFS volume issues
- Historical reference

---

### 3. Wikipedia HFS+ Reference

**File:** [`WIKIPEDIA_HFS_PLUS.md`](WIKIPEDIA_HFS_PLUS.md)

**Source:** Wikipedia (November 2025)

**Content:**
- HFS+ design and evolution (1998-2017)
- Nine volume structures overview
- Technical specifications and limits
- Journaling support
- Cross-platform compatibility notes
- Known issues and criticisms
- APFS comparison

**Key Topics:**
- Historical timeline
- Platform support (macOS, Linux, Windows)
- Journaling implementation
- Catalog B-tree (4KB nodes)
- Unicode normalization
- Migration to APFS

**Use When:**
- Understanding HFS+ in broader context
- Cross-platform implementation considerations
- Learning about journaling
- Comparing with modern filesystems

---

### 4. Implementation Notes

**File:** [`HFS_IMPLEMENTATION_NOTES.md`](HFS_IMPLEMENTATION_NOTES.md)

**Source:** hfsutils project experience + compiled specifications

**Content:**
- Practical implementation guidance for hfsutils
- Critical differences between HFS and HFS+ (side-by-side tables)
- Common formulas and code patterns
- Byte order handling (endianness)
- Date/time conversion functions
- B-tree implementation details
- Testing strategies and validation

**Key Topics:**
- Alternate header location (CRITICAL: file_size - 1024)
- HFS MDB checklist
- HFS+ Volume Header checklist
- Field offset verification table
- Common errors and corrections
- Interoperability issues (hfsutil vs mkfs)
- Testing procedures

**Use When:**
- Developing or debugging mkfs/fsck tools
- Fixing specification conformance issues
- Writing validation tests
- Troubleshooting interoperability

---

## Quick Reference Guide

### Alternate Header Location (Both HFS and HFS+)

```c
// CORRECT
alternate_offset = device_size_in_bytes - 1024;

// INCORRECT
alternate_offset = (total_sectors - 1) * sector_size;
```

### Signature Values

| Filesystem | Signature | Location | Value |
|------------|-----------|----------|-------|
| HFS (MDB) | drSigWord | Offset 1024 + 0 | 0x4244 ('BD') |
| HFS+ (VH) | signature | Offset 1024 + 0 | 0x482B ('H+') |
| HFSX (VH) | signature | Offset 1024 + 0 | 0x4858 ('HX') |

### Unmounted Bit

| Filesystem | Field | Offset | Value |
|------------|-------|--------|-------|
| HFS | drAtrb | 1024 + 10 | 0x0100 (bit 8) |
| HFS+ | attributes | 1024 + 4 | 0x00000100 (bit 8) |

### Next Catalog ID

| Filesystem | Field | Offset | Minimum Value |
|------------|-------|--------|---------------|
| HFS | drNxtCNID | 1024 + 30 | 16 (0x00000010) |
| HFS+ | nextCatalogID | 1024 + 64 | 16 (0x00000010) |

**Important:** In HFS+, nextCatalogID is at offset +64 only if rsrcClumpSize (+56) and dataClumpSize (+60) are present.

---

## Validation Commands

### HFS Classic

```bash
# Primary MDB signature
xxd -s 1024 -l 2 -p volume.hfs
# Expected: 4244

# Alternate MDB signature
FILESIZE=$(stat -c%s volume.hfs)
xxd -s $((FILESIZE - 1024)) -l 2 -p volume.hfs
# Expected: 4244

# drAtrb (unmounted bit)
xxd -s 1028 -l 2 -p volume.hfs
# Expected: 0100

# drNxtCNID
xxd -s 1108 -l 4 -p volume.hfs
# Expected: 00000010
```

### HFS Plus

```bash
# Primary Volume Header signature
xxd -s 1024 -l 2 -p volume.hfsplus
# Expected: 482b

# Alternate Volume Header signature
FILESIZE=$(stat -c%s volume.hfsplus)
xxd -s $((FILESIZE - 1024)) -l 2 -p volume.hfsplus
# Expected: 482b

# attributes (unmounted bit)
xxd -s 1028 -l 4 -p volume.hfsplus
# Expected: 00000100 or 00002100

# rsrcClumpSize (must be non-zero)
xxd -s 1080 -l 4 -p volume.hfsplus

# dataClumpSize (must be non-zero)
xxd -s 1084 -l 4 -p volume.hfsplus

# nextCatalogID
xxd -s 1088 -l 4 -p volume.hfsplus
# Expected: 00000010
```

---

## Common Errors Fixed in hfsutils

### 1. Wrong Alternate Header Offset ✅ FIXED

**Problem:**
```c
offset = (total_sectors - 1) * sector_size;
```

**Solution:**
```c
offset = device_size - 1024;
```

**Files Fixed:**
- `src/mkfs/mkfs_hfs_format.c` (both HFS and HFS+)

---

### 2. Missing HFS+ Clump Size Fields ✅ FIXED

**Problem:** Volume Header missing rsrcClumpSize and dataClumpSize caused field misalignment.

**Solution:** Added both fields at offsets +56 and +60.

**Impact:** nextCatalogID now correctly at offset +64.

**Files Fixed:**
- `src/mkfs/mkfs_hfs_format.c`

---

### 3. Missing Unmounted Bit in HFS+ ✅ FIXED

**Problem:**
```c
uint32_t attributes = 0x00000000;
```

**Solution:**
```c
uint32_t attributes = 0x00000100;  // kHFSVolumeUnmountedBit
```

**Files Fixed:**
- `src/mkfs/mkfs_hfs_format.c`

---

### 4. B-tree Compatibility Issue ⚠️ UNDER INVESTIGATION

**Problem:** hfsutil cannot mount volumes created by standalone mkfs.hfs.

**Error:** `malformed b*-tree header node (Input/output error)`

**Status:** Requires investigation of B-tree initialization differences.

---

## Development Workflow

### 1. Before Making Changes

- Read relevant documentation (TN1150 for HFS+, HFS_CLASSIC for HFS)
- Check Implementation Notes for common patterns
- Review existing code in libhfs/ and src/mkfs/

### 2. During Development

- Use Implementation Notes checklists
- Validate byte offsets against specifications
- Test with validation commands
- Compare with working implementations

### 3. After Changes

- Run validation tests
- Check with hexdump/xxd
- Test cross-compatibility
- Update documentation if needed

---

## External References

### Online Resources (for updates)

- Apple Technical Note TN1150: https://developer.apple.com/library/archive/technotes/tn/tn1150.html
- Wikipedia HFS+: https://en.wikipedia.org/wiki/HFS_Plus
- Wikipedia HFS: https://en.wikipedia.org/wiki/Hierarchical_File_System_(Apple)

### Tools

- **macOS:** diskutil, newfs_hfs, fsck_hfs
- **Linux:** hfsprogs, fsck.hfsplus, mkfs.hfsplus
- **Cross-platform:** hexdump, xxd, dd

### Related Projects

- Original hfsutils: http://www.mars.org/home/rob/proj/hfs/
- Linux hfsprogs: https://github.com/0x09/hfsprogs
- APFS: https://developer.apple.com/documentation/foundation/file_system/about_apple_file_system

---

## Document Maintenance

### Updating This Documentation

1. **TN1150:** Check Apple Developer Archive for updates (unlikely, archived)
2. **Wikipedia:** Review Wikipedia pages periodically for corrections
3. **Implementation Notes:** Update based on project experience and bug fixes

### Version Control

All documentation is tracked in git. See commit history for changes and rationale.

---

## Contributing

When adding new documentation:

1. Use Markdown format
2. Include source attribution
3. Add entry to this index
4. Cross-reference related documents
5. Update Quick Reference if adding critical formulas

---

## License

- **TN1150:** © Apple Inc., archived for reference
- **Wikipedia Content:** CC BY-SA 4.0 License
- **Implementation Notes:** Part of hfsutils project

---

*Last updated: November 2025*
