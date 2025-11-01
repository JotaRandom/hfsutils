# mkfs.hfs - Standalone HFS/HFS+ Filesystem Creation Utility

This directory contains the standalone `mkfs.hfs` utility, derived from the original `hformat` tool in hfsutils but designed to operate independently without requiring the full hfsutils installation.

## Overview

`mkfs.hfs` is a Unix/Linux-style filesystem creation utility that follows standard `mkfs` conventions and integrates properly with system tools and package managers.

## Features

- **Standalone Operation**: No dependency on hfsutils installation
- **Standard Interface**: Follows Unix/Linux mkfs conventions
- **Multiple Names**: Supports mkfs.hfs, mkfs.hfs+, and mkfs.hfsplus
- **Auto-detection**: Automatically detects filesystem type from program name
- **HFS Specification Compliant**: Implements HFS format according to Apple's official specification
- **Device Validation**: Comprehensive device and partition validation
- **Error Handling**: Standard exit codes and comprehensive error reporting

## Command-line Interface

```bash
mkfs.hfs [-f] [-l label] [-t type] [-v] device [partition-no]
mkfs.hfs+ [-f] [-l label] [-v] device [partition-no]
mkfs.hfsplus [-f] [-l label] [-v] device [partition-no]
```

### Options

- `-f, --force`: Force creation, overwrite existing filesystem
- `-l, --label NAME`: Set volume label/name (max 27 characters)
- `-t, --type TYPE`: Filesystem type: hfs, hfs+, hfsplus (auto-detect if not specified)
- `-v, --verbose`: Verbose output
- `-V, --version`: Display version information
- `-h, --help`: Display help message

### Arguments

- `device`: Block device or file to format
- `partition-no`: Partition number (optional, 0 for whole device)

## Examples

```bash
# Format partition as HFS
mkfs.hfs /dev/sdb1

# Format with custom label
mkfs.hfs -l "My Volume" /dev/sdb1

# Force format partition 1
mkfs.hfs -f /dev/sdb 1

# Format as HFS+ (using hfs+ program name)
mkfs.hfs+ /dev/sdb1

# Verbose formatting
mkfs.hfs -v -l "Test Volume" /dev/sdb1
```

## Exit Codes

- `0`: Success
- `1`: General error
- `2`: Usage error
- `4`: Operational error
- `8`: System error

## Implementation Details

### HFS Format Specification

The implementation follows Apple's official HFS specification from "Inside Macintosh: Files" (1992):

- **Boot Blocks** (blocks 0-1): Contains startup code with signature `$4C4B`
- **Master Directory Block** (block 2): Core metadata with signature `$4244`
- **Volume Bitmap** (block 3+): Allocation block usage tracking
- **Catalog File**: B*-tree storing file/directory hierarchy
- **Extents Overflow File**: B*-tree for additional file extents

### Key Features

1. **Big-endian byte order** as required by HFS specification
2. **Allocation block size** calculated based on volume size for efficiency
3. **Maximum 65,535 allocation blocks** per HFS volume
4. **HFS date format** (seconds since 1904-01-01 00:00:00 UTC)
5. **Pascal string format** for volume names (length byte + characters)

### Architecture

```
mkfs_hfs_main.c     # Main entry point and command-line parsing
mkfs_hfs_format.c   # HFS/HFS+ formatting implementation
mkfs_hfs.h          # Header with structures and function declarations
```

The utility uses embedded libraries from `../embedded/shared/` for:
- Device detection and validation
- Error handling and reporting
- SUID privilege management
- Common utility functions

## Building

```bash
# Build mkfs.hfs
make all

# Create symbolic links for different names
make links

# Install to system
make install

# Clean build artifacts
make clean
```

## Status

**✅ Task 3.1 Complete**: Main entry point implemented with:
- Standard mkfs command-line parsing
- Program name detection for mkfs.hfs, mkfs.hfs+, mkfs.hfsplus
- Standard mkfs exit codes and error handling

**✅ Task 3.2 Complete**: HFS formatting functionality implemented with:
- Complete HFS volume creation following Apple's specification
- Integration with embedded libhfs for volume operations
- Device validation and partition handling
- Proper HFS structure writing (boot blocks, MDB, bitmap, B*-trees)
- Volume verification after formatting
- Comprehensive error handling and verbose output

**✅ Task 3.3 Complete**: HFS+ formatting functionality implemented with:
- Complete HFS+ volume creation following Apple's specification
- HFS+ Volume Header creation with correct signatures
- Larger block sizes (4096 bytes) for better performance
- HFS+ allocation bitmap and system file layout
- Support for both HFS and HFS+ in same codebase
- Automatic filesystem type detection from program name
- Future-ready structure for journaling and case sensitivity

**📋 Next Steps**: 
- Task 3.4: Add full command-line interface compatibility

The foundation is now in place for a fully functional standalone mkfs.hfs utility that meets all the requirements for independent operation and standards compliance.