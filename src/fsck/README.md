# fsck.hfs - Standalone HFS/HFS+ Filesystem Checking Utility

This directory contains the source code for the standalone `fsck.hfs` utility, which checks and repairs HFS and HFS+ filesystems without requiring the full hfsutils suite.

## Files (to be implemented)

- `fsck_main.c` - Main entry point for fsck.hfs
- `hfs_check.c` - HFS checking logic
- `hfsplus_check.c` - HFS+ checking logic
- `journal.c` - Journal management
- `fsck_common.c` - Common checking utilities

## Features

- Standalone executable with embedded dependencies
- Support for both HFS and HFS+ filesystem checking and repair
- HFS+ journal replay and management
- Standard fsck command-line interface
- Program name detection (fsck.hfs, fsck.hfs+, fsck.hfsplus)
- Standard Unix/Linux exit codes

## Build

```bash
make fsck.hfs
```

## Usage

```bash
fsck.hfs [-v] [-n] [-a] [-f] [-y] device [partition-no]
fsck.hfs+ [-v] [-n] [-a] [-f] [-y] device [partition-no]
fsck.hfsplus [-v] [-n] [-a] [-f] [-y] device [partition-no]
```