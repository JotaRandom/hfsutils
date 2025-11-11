# HFS/HFS+ Implementation Notes for hfsutils

**Project:** hfsutils  
**Purpose:** Development reference for HFS and HFS+ implementation  
**Date:** November 2025

---

## Critical Differences: HFS vs HFS+

When implementing tools for both filesystems, it is **essential** to keep them separate:

| Aspect | HFS Classic | HFS Plus |
|--------|-------------|----------|
| **Primary Structure** | Master Directory Block (MDB) | Volume Header |
| **Signature** | 0x4244 ('BD') | 0x482B ('H+') or 0x4858 ('HX') |
| **Signature Offset** | 1024 | 1024 |
| **Primary Location** | Offset 1024 | Offset 1024 |
| **Alternate Location** | **file_size - 1024** | **file_size - 1024** |
| **Attributes Field** | drAtrb (offset +10, 2 bytes) | attributes (offset +4, 4 bytes) |
| **Next CNID Field** | drNxtCNID (offset +30, 4 bytes) | nextCatalogID (offset +64, 4 bytes) |
| **Allocation Addressing** | 16-bit (max 65,535 blocks) | 32-bit (max ~4 billion blocks) |
| **B-tree Node Size** | 512 bytes | 4096 bytes (typically) |
| **Filenames** | MacRoman, 31 chars | UTF-16 Unicode, 255 chars |
| **Clump Sizes** | drClpSiz, drXTClpSiz, drCTClpSiz | rsrcClumpSize, dataClumpSize |

---

## Common Formula: Alternate Header Location

**BOTH HFS AND HFS+ use the same formula:**

```c
alternate_offset = device_size_in_bytes - 1024;
```

**Do NOT use:**
```c
// WRONG - This calculates the last sector, not 1024 bytes before end
alternate_offset = (total_sectors - 1) * sector_size;
```

### Why the Confusion?

For devices with 512-byte sectors:
- Last sector offset: `device_size - 512`
- Second-to-last sector: `device_size - 1024`

The alternate header is at the **second-to-last sector**, which equals `device_size - 1024` for 512-byte sectors. But the specification says "1024 bytes before end", not "second-to-last sector", because sector size can vary.

---

## HFS (Classic) Implementation Checklist

### Master Directory Block (MDB)

**Location:** Offset 1024, size 512 bytes

**Required fields for new volume:**

```c
// At offset 1024
uint16_t drSigWord = 0x4244;           // 'BD' signature

// At offset 1024 + 10
uint16_t drAtrb = 0x0100;              // Unmounted bit set

// At offset 1024 + 30
uint32_t drNxtCNID = 0x00000010;       // 16, first user CNID

// Volume name at offset 1024 + 36
// Pascal string: length byte + up to 27 characters
```

### Alternate MDB

**Location:** `device_size - 1024`

**Write:** Exact copy of primary MDB

### B-tree Specifications

- **Node size:** 512 bytes (fixed for HFS)
- **Catalog File:** CNID 4
- **Extents Overflow File:** CNID 3

### Testing Validation

```bash
# Check primary MDB signature
xxd -s 1024 -l 2 -p volume.hfs
# Expected: 4244

# Check alternate MDB signature
FILESIZE=$(stat -c%s volume.hfs)
ALTOFFSET=$((FILESIZE - 1024))
xxd -s $ALTOFFSET -l 2 -p volume.hfs
# Expected: 4244

# Check drAtrb (unmounted bit)
xxd -s 1028 -l 2 -p volume.hfs
# Expected: 0100

# Check drNxtCNID
xxd -s 1108 -l 4 -p volume.hfs
# Expected: 00000010
```

---

## HFS+ Implementation Checklist

### Volume Header

**Location:** Offset 1024, size 512 bytes

**Required fields for new volume:**

```c
// At offset 1024
uint16_t signature = 0x482B;            // 'H+' (big-endian)

// At offset 1024 + 2
uint16_t version = 0x0004;              // Version 4

// At offset 1024 + 4
uint32_t attributes = 0x00000100;       // Bit 8: unmounted

// At offset 1024 + 40
uint32_t blockSize = /* calculated */;

// At offset 1024 + 44
uint32_t totalBlocks = /* calculated */;

// At offset 1024 + 48
uint32_t freeBlocks = /* calculated */;

// At offset 1024 + 56
uint32_t rsrcClumpSize = blockSize * 4; // Typical value

// At offset 1024 + 60
uint32_t dataClumpSize = blockSize * 4; // Typical value

// At offset 1024 + 64
uint32_t nextCatalogID = 0x00000010;    // 16 (big-endian)
```

### Field Offset Verification

**CRITICAL:** Volume Header fields must be at exact offsets. Missing fields cause misalignment.

| Field | Offset | Size | Must Not Be Zero |
|-------|--------|------|------------------|
| signature | +0 | 2 | ✓ (0x482B) |
| version | +2 | 2 | ✓ (4) |
| attributes | +4 | 4 | ✓ (0x0100 minimum) |
| lastMountedVersion | +8 | 4 | |
| journalInfoBlock | +12 | 4 | |
| createDate | +16 | 4 | ✓ |
| modifyDate | +20 | 4 | ✓ |
| backupDate | +24 | 4 | |
| checkedDate | +28 | 4 | |
| fileCount | +32 | 4 | |
| folderCount | +36 | 4 | |
| blockSize | +40 | 4 | ✓ |
| totalBlocks | +44 | 4 | ✓ |
| freeBlocks | +48 | 4 | ✓ |
| nextAllocation | +52 | 4 | |
| **rsrcClumpSize** | **+56** | **4** | **✓** |
| **dataClumpSize** | **+60** | **4** | **✓** |
| **nextCatalogID** | **+64** | **4** | **✓ (16 min)** |
| writeCount | +68 | 4 | |
| encodingsBitmap | +72 | 8 | |
| finderInfo | +80 | 32 | |
| allocationFile | +112 | 80 | ✓ |
| extentsFile | +192 | 80 | ✓ |
| catalogFile | +272 | 80 | ✓ |
| attributesFile | +352 | 80 | |
| startupFile | +432 | 80 | |

**Common error:** Forgetting rsrcClumpSize and dataClumpSize causes nextCatalogID to be written at offset +56 instead of +64, making it read as 0.

### Alternate Volume Header

**Location:** `device_size - 1024`

**Write:** Exact copy of primary Volume Header

### B-tree Specifications

- **Node size:** 4096 bytes (typical), can be 8192
- **Catalog File:** CNID 4, keyCompareType for Unicode
- **Extents Overflow File:** CNID 3
- **Attributes File:** CNID 8 (optional)

### Testing Validation

```bash
# Check primary Volume Header signature
xxd -s 1024 -l 2 -p volume.hfsplus
# Expected: 482b

# Check alternate Volume Header signature
FILESIZE=$(stat -c%s volume.hfsplus)
ALTOFFSET=$((FILESIZE - 1024))
xxd -s $ALTOFFSET -l 2 -p volume.hfsplus
# Expected: 482b

# Check attributes (unmounted bit)
xxd -s 1028 -l 4 -p volume.hfsplus
# Expected: 00000100 or 00002100 (if journaled)

# Check rsrcClumpSize (should be non-zero)
xxd -s 1080 -l 4 -p volume.hfsplus
# Expected: non-zero value (e.g., 00004000 for 16KB)

# Check dataClumpSize (should be non-zero)
xxd -s 1084 -l 4 -p volume.hfsplus
# Expected: non-zero value (e.g., 00004000 for 16KB)

# Check nextCatalogID
xxd -s 1088 -l 4 -p volume.hfsplus
# Expected: 00000010 (16 decimal)
```

---

## Date/Time Handling

Both HFS and HFS+ use **Mac absolute time**:
- Format: Unsigned 32-bit integer
- Epoch: Midnight, January 1, 1904 GMT
- Range: 1904-01-01 to 2040-02-06

### Conversion Functions

```c
#include <time.h>

#define MAC_EPOCH_OFFSET 2082844800UL

// Unix time to Mac time
uint32_t unix_to_mac_time(time_t unix_time) {
    return (uint32_t)(unix_time + MAC_EPOCH_OFFSET);
}

// Mac time to Unix time
time_t mac_to_unix_time(uint32_t mac_time) {
    return (time_t)(mac_time - MAC_EPOCH_OFFSET);
}

// Get current Mac time
uint32_t get_current_mac_time(void) {
    return unix_to_mac_time(time(NULL));
}
```

---

## Byte Order (Endianness)

**All multi-byte fields are big-endian** on disk for both HFS and HFS+.

### Conversion Macros

```c
#include <stdint.h>

// Convert host to big-endian
static inline uint16_t htobe16_safe(uint16_t x) {
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        return ((x & 0xFF) << 8) | ((x >> 8) & 0xFF);
    #else
        return x;
    #endif
}

static inline uint32_t htobe32_safe(uint32_t x) {
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        return ((x & 0xFF) << 24) |
               ((x & 0xFF00) << 8) |
               ((x >> 8) & 0xFF00) |
               ((x >> 24) & 0xFF);
    #else
        return x;
    #endif
}

// Convert big-endian to host
static inline uint16_t be16toh_safe(uint16_t x) {
    return htobe16_safe(x);  // Same operation
}

static inline uint32_t be32toh_safe(uint32_t x) {
    return htobe32_safe(x);  // Same operation
}
```

### Writing to Disk

```c
// WRONG - host byte order
uint32_t value = 16;
write(fd, &value, sizeof(value));

// CORRECT - big-endian
uint32_t value = 16;
uint32_t be_value = htobe32_safe(value);
write(fd, &be_value, sizeof(be_value));

// ALTERNATIVE - byte array
unsigned char buffer[4];
buffer[0] = (value >> 24) & 0xFF;
buffer[1] = (value >> 16) & 0xFF;
buffer[2] = (value >> 8) & 0xFF;
buffer[3] = value & 0xFF;
write(fd, buffer, 4);
```

---

## B-tree Implementation Notes

### Node Descriptor

Every B-tree node starts with a Node Descriptor (14 bytes):

```c
struct NodeDescriptor {
    uint32_t fLink;       // Forward link (next node)
    uint32_t bLink;       // Backward link (previous node)
    int8_t   kind;        // Node type
    uint8_t  height;      // Node level (0 = leaf)
    uint16_t numRecords;  // Number of records
    uint16_t reserved;    // Reserved
};
```

**Node Types:**
- -1 (0xFF): Index node
- 0: Header node
- 1: Map node  
- 2: Leaf node

### Header Node (Node 0)

```c
struct BTHeaderRec {
    uint16_t treeDepth;
    uint32_t rootNode;
    uint32_t leafRecords;
    uint32_t firstLeafNode;
    uint32_t lastLeafNode;
    uint16_t nodeSize;        // 512 (HFS) or 4096 (HFS+)
    uint16_t maxKeyLength;
    uint32_t totalNodes;
    uint32_t freeNodes;
    uint16_t reserved1;
    uint32_t clumpSize;
    uint8_t  btreeType;       // 0=Catalog, 255=Extents
    uint8_t  keyCompareType;
    uint32_t attributes;
    uint8_t  reserved3[64];
};
```

### Common B-tree Errors

1. **Wrong node size**
   - HFS: Must be 512 bytes
   - HFS+: Typically 4096 bytes

2. **Incorrect node descriptor**
   - Header node must have kind=0
   - Leaf nodes must have kind=2

3. **Invalid key comparison**
   - HFS: Case-insensitive MacRoman
   - HFS+: Case-insensitive Unicode (or case-sensitive for HFSX)

---

## Interoperability Notes

### hfsutil vs mkfs Compatibility

**Known Issue:** hfsutil (libhfs-based) cannot mount volumes created by standalone mkfs.hfs.

**Error Message:**
```
hfsutil: malformed b*-tree header node (Input/output error)
```

**Likely Causes:**
1. B-tree node size mismatch
2. B-tree header field differences
3. Catalog/Extents initialization differences

**Investigation Needed:**
- Compare B-tree initialization in:
  - `src/mkfs/mkfs_hfs_format.c` (standalone mkfs)
  - `libhfs/btree.c` (hfsutil's libhfs)
- Check node descriptor and BTHeaderRec structure
- Verify catalog file initialization

**Workaround:**
- Use hformat from hfsutil for volumes that need hfsutil access
- Use standalone mkfs.hfs for volumes that don't need hfsutil

---

## Testing Strategy

### Unit Tests

For each mkfs implementation:

1. **Signature verification**
   - Primary header has correct signature
   - Alternate header has correct signature at file_size - 1024

2. **Field value validation**
   - attributes contains unmounted bit
   - nextCatalogID >= 16
   - Clump sizes are non-zero (HFS+ only)
   - All required fields are present

3. **Structure alignment**
   - Fields at correct byte offsets
   - No padding errors
   - Big-endian byte order

4. **B-tree structure**
   - Header node is valid
   - Node size is correct
   - Catalog contains root folder

### Integration Tests

1. **Cross-tool compatibility**
   - mkfs.hfs → fsck.hfs validation
   - mkfs.hfs+ → fsck.hfs+ validation
   - hformat → hfsutil operations

2. **Platform verification**
   - Linux: mount, read, write
   - macOS: diskutil verify

3. **Stress testing**
   - Various volume sizes
   - Different block sizes
   - Edge cases (minimum/maximum values)

---

## References

### Primary Documentation

- `doc/TN1150_HFS_PLUS_VOLUME_FORMAT.md` - Apple Technical Note (this project)
- `doc/HFS_CLASSIC_SPECIFICATION.md` - HFS Classic spec (this project)
- `doc/WIKIPEDIA_HFS_PLUS.md` - Wikipedia reference (this project)

### Source Code

- `src/mkfs/mkfs_hfs_format.c` - Standalone mkfs implementation
- `libhfs/` - Original libhfs implementation (hfsutil)
- `hfsck/` - Filesystem checker

### External Tools

- `diskutil` (macOS) - Apple's disk utility
- `hfsprogs` (Linux) - Linux HFS+ tools
- `hexdump`/`xxd` - Binary inspection

---

## Revision History

- **November 2025**: Initial version compiled from TN1150, Wikipedia, and implementation experience
- Includes fixes for alternate header location and HFS+ field alignment

---

*This document is an internal reference for the hfsutils project.*
