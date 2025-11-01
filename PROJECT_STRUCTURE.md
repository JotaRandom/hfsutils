# Project Structure for Standalone Utilities

This document describes the project structure created for separating hfsutils into standalone filesystem utilities.

## New Directory Structure

```
src/
├── mkfs/                    # mkfs.hfs standalone utility
│   └── README.md           # Documentation for mkfs.hfs
├── fsck/                    # fsck.hfs standalone utility  
│   └── README.md           # Documentation for fsck.hfs
├── mount/                   # mount.hfs standalone utility
│   └── README.md           # Documentation for mount.hfs
└── embedded/                # Embedded libraries for standalone utilities
    ├── README.md           # Documentation for embedded libraries
    ├── libhfs_subset/      # Minimal libhfs implementation
    ├── common/             # Shared utilities
    └── suid/               # Privilege management

build/
└── standalone/             # Build directory for standalone utilities
    └── README.md           # Build documentation
```

## Build System Updates

### New Makefile Targets

**Build Targets:**
- `make standalone` - Build all standalone utilities
- `make mkfs.hfs` - Build mkfs.hfs utility
- `make fsck.hfs` - Build fsck.hfs utility  
- `make mount.hfs` - Build mount.hfs utility

**Installation Targets:**
- `make install-linux` - Install mkfs.hfs, fsck.hfs, mount.hfs (Linux systems)
- `make install-other` - Install mkfs.hfs, fsck.hfs + hfsutils (BSD/other systems)
- `make install-complete` - Install all utilities (maximum compatibility)

### Variables Added

- `STANDALONE_UTILITIES` - List of standalone utilities
- `MOUNT_LINKS` - Symbolic links for mount.hfs variants

## Implementation Status

✅ **Task 1 Completed: Set up project structure for standalone utilities**
- ✅ Created new directory structure for mkfs, fsck, and mount components
- ✅ Set up build directories and organization for embedded libraries  
- ✅ Created initial Makefile targets for standalone builds
- ✅ Added documentation for each component

## Next Steps

The project structure is now ready for:
1. **Task 2**: Extract and prepare libhfs subset for embedding
2. **Task 3**: Implement mkfs.hfs standalone utility
3. **Task 4**: Implement fsck.hfs standalone utility
4. **Task 5**: Implement mount.hfs standalone utility

## Testing

The build system can be tested in a Linux/WSL environment with:
```bash
make standalone          # Build all standalone utilities
make install-linux      # Test Linux installation
make install-other      # Test other systems installation
make install-complete   # Test complete installation
```