# mount.hfs - Standalone HFS/HFS+ Filesystem Mounting Utility

This directory contains the source code for the standalone `mount.hfs` utility, which mounts HFS and HFS+ filesystems without requiring the full hfsutils suite.

## Files (to be implemented)

- `mount_main.c` - Main entry point for mount.hfs
- `hfs_mount.c` - HFS mounting logic
- `hfsplus_mount.c` - HFS+ mounting logic
- `mount_common.c` - Common mounting utilities

## Features

- Standalone executable with embedded dependencies
- Support for both HFS and HFS+ filesystem mounting
- Integration with system mount infrastructure
- Standard mount command-line interface
- Program name detection (mount.hfs, mount.hfs+, mount.hfsplus)
- Standard Unix/Linux exit codes

## Build

```bash
make mount.hfs
```

## Usage

```bash
mount.hfs [-r] [-v] [-o options] device mountpoint
mount.hfs+ [-r] [-v] [-o options] device mountpoint
mount.hfsplus [-r] [-v] [-o options] device mountpoint
```