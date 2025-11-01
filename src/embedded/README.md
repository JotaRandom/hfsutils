# Embedded Libraries for Standalone Utilities - Reorganized

This directory contains the reorganized embedded library code for the standalone filesystem utilities (mkfs.hfs, fsck.hfs, mount.hfs).

## New Directory Structure ✅

```
src/embedded/
├── mkfs/           # mkfs.hfs specific code
│   ├── hfs_format.c       # HFS formatting implementation
│   └── mkfs_hfs.h         # mkfs.hfs header
├── fsck/           # fsck.hfs specific code  
│   ├── hfs_check.c        # HFS checking implementation
│   └── fsck_hfs.h         # fsck.hfs header
├── mount/          # mount.hfs specific code
│   ├── hfs_mount_util.c   # HFS mounting implementation
│   └── mount_hfs.h        # mount.hfs header
├── shared/         # Shared code between all utilities
│   ├── libhfs.h           # Main libhfs header
│   ├── hfs.h              # HFS structures
│   ├── apple.h            # Apple data structures
│   ├── hfs_mount.c        # Core mounting functions
│   ├── volume_mkfs.c      # Volume management
│   ├── stubs.c            # Stub implementations
│   ├── hfsutil_mkfs.c     # HFS utility functions
│   ├── hfs_detect.c       # Filesystem detection
│   ├── version.c          # Version information
│   ├── suid.c             # Privilege management
│   ├── device_utils.c     # Device detection and partitioning
│   ├── error_utils.c      # Error handling and reporting
│   └── common_utils.c     # Common utility functions
└── Makefile        # Reorganized build system
```

## Benefits of Reorganization ✅

**1. Clear Separation of Concerns**
- Each utility has its own directory with specific code
- Shared code is clearly identified and reusable
- No more confusion about which files belong to which utility

**2. Reduced Complexity**
- Eliminated duplicate and unnecessary files
- Removed unused libhfs components
- Simplified build dependencies

**3. Better Maintainability**
- Easier to understand and modify individual utilities
- Clear dependency relationships
- Organized build system

**4. Cleaner Build System**
- Separate libraries for each utility
- Shared library for common code
- Clear compilation targets

## Build Targets ✅

**Libraries Generated:**
- `libmkfs.a` - mkfs.hfs specific code
- `libfsck.a` - fsck.hfs specific code  
- `libmount.a` - mount.hfs specific code
- `libshared.a` - Shared code for all utilities

**Build Commands:**
```bash
make clean          # Clean all build artifacts
make all            # Build all libraries
make libmkfs.a      # Build only mkfs.hfs library
make libmount.a     # Build only mount.hfs library
```

## Common Utilities Added ✅

**Task 2.4 Completed** - Common embedded utilities have been implemented:

**1. SUID Privilege Management** (`suid.c`, `suid.h`)
- Safe privilege escalation for device access
- Automatic privilege dropping after operations
- Cross-platform compatibility

**2. Device Detection and Partitioning** (`device_utils.c`, `device_utils.h`)
- Device validation and accessibility checks
- Partition table detection (Apple, MBR, GPT)
- Device size calculation and mount status checking
- Support for block devices and regular files

**3. Error Handling and Reporting** (`error_utils.c`, `error_utils.h`)
- Standardized error messages with program name
- Verbose mode support
- Error logging to file
- Standard exit codes for filesystem utilities
- Fatal error handling with cleanup

**4. Common Utility Functions** (`common_utils.c`, `common_utils.h`)
- Program type detection from executable name
- Filesystem type validation
- Version and license information display
- Partition number parsing
- Device path resolution

## Current Status

**✅ Structure Reorganized** - Clean directory organization implemented
**✅ Task 2.4 Complete** - Common embedded utilities created and integrated
**📋 Next Steps** - Move to Task 3.1 (Create mkfs.hfs main entry point)

The embedded library foundation is now complete with all common utilities properly implemented and integrated. The utilities provide consistent error handling, device management, and program behavior across all standalone tools.