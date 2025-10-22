# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [4.1.0A.1] - 2025-10-21

### Added
- **Flexible Installation System**: Complete overhaul of installation paths for modern distributions
  - SBINDIR variable for configurable system binary directory
  - Smart symlink creation that detects merged vs separate bin/sbin systems
  - Proper distribution packaging support with PREFIX=/usr
  - Automatic handling of Arch Linux, Fedora 17+, Debian 8+, Ubuntu 15.04+ merged systems
- **Distribution Packaging Support**: Comprehensive packaging documentation
  - PACKAGING.md with detailed instructions for different distributions
  - Examples for Arch Linux, Debian/Ubuntu, Fedora/RHEL, openSUSE, macOS
  - Installation variable documentation and compatibility matrix

### Fixed
- **hfsck Installation Path**: Fixed hfsck to properly respect SBINDIR variable
  - Updated hfsck Makefiles to use SBINDIR instead of hardcoded /sbin
  - Ensured all system utilities install to correct directory for both traditional and merged systems
- **Build System Robustness**: Enhanced build system reliability
  - Fixed autotools configuration problems in hfsck
  - Added build-manual target to avoid autotools when needed
  - Improved .gitignore to use root-specific paths

### Changed
- **Installation System**: Modern distribution compatibility
  - Support both traditional (/usr/sbin) and merged (/usr/bin) filesystem layouts
  - Enhanced documentation with packaging examples and modern system compatibility
  - All installation paths now fully configurable

## [4.1.0A] - 2025-10-21

### Added
- **HFS+ Journaling Support**: Complete implementation of HFS+ journal replay and validation
  - journal.h/journal.c - Full journaling infrastructure
  - Journal transaction replay for crash recovery
  - Journal validation and corruption detection
  - Automatic journal disabling for corrupted journals
  - Comprehensive logging to hfsutils.log
- **Enhanced fsck.hfs+**: Advanced HFS+ filesystem checking with journaling support
  - Automatic journal replay before filesystem validation
  - Journal corruption detection and repair
  - Support for both journaled and non-journaled HFS+ volumes
- **Complete Manual Pages**: Comprehensive man pages for all filesystem utilities
  - mkfs.hfs.8, mkfs.hfs+.8 - Filesystem creation utilities
  - fsck.hfs.8, fsck.hfs+.8 - Filesystem checking utilities
  - Proper symlinks for .hfsplus variants
  - **Enhanced Installation**: Section 8 manual pages installed to correct directories
  - **GitHub Integration**: Complete CI/CD workflow and Copilot instructions
  - **Updated Documentation**: Comprehensive TODO file with project status

### Fixed
  - **Journal Write Warnings**: Fixed unused result warnings in journal operations
  - **Compilation Warnings**: Fixed setreuid/setregid unused result warnings
  - **Makefile Warnings**: Removed duplicate rules in hfsck Makefile
  - **Version Strings**: Cleaned up version information, removed "(Apple Silicon fork) for uniformity"
  - **Include Path Error**: Fixed typo in hfsck/main.c include path (../nclude -> ../include)
  - **Build System Issues**: Fixed autotools configuration problems in hfsck
    - Fixed config.status rule to use ./configure instead of ./config.status --recheck
    - Added robust fallback compilation when autotools fails
    - Improved error handling in build process
    - Fixed hfsck installation to respect SBINDIR variable instead of hardcoded /sbin

### Changed
  - **Code Organization**: Moved hfs_detect.h from include/hfsutil/ to include/common/
    - Better reflects its role as common utility used by multiple components (hfsck, hformat, etc.)
    - Maintains consistent project structure with other common utilities
    - Updated all include references accordingly
  - **Build System Improvements**: Enhanced build robustness and error handling
    - Added build-manual target to avoid autotools when needed
    - Improved .gitignore to use root-specific paths and avoid blocking source directories
    - Enhanced build.sh with better fallback logic
  - **Installation System Overhaul**: Modern distribution packaging support
    - Added SBINDIR variable for flexible system binary placement
    - Smart symlink creation for merged /bin systems (Arch, Fedora 17+, etc.)
    - Proper support for distribution packaging with PREFIX=/usr
    - Automatic detection of merged vs traditional filesystem layouts
  - **Project Status**: Updated TODO file to reflect completed features
  - **Build System**: Enhanced .gitignore for filesystem utility symlinks

## [4.0.0] - 2025-10-21 - "Apple Silicon Fork"

### Added
- **Modern Build System**: Complete rewrite with standard Unix build conventions
- **DESTDIR Support**: Full staging directory support for package building
- **Flexible Installation Paths**: Configurable PREFIX, BINDIR, LIBDIR, INCLUDEDIR, MANDIR
- **Compiler Variable Support**: CC, CXX, CFLAGS, CXXFLAGS, LDFLAGS support
- **Automatic Manual Page Installation**: All manual pages installed to proper locations
- **Library Installation**: libhfs.a and librsrc.a with development headers
- **Variable Propagation**: Proper passing of build variables to all sub-makefiles
- **Build Documentation**: Comprehensive BUILD.md with usage examples
- **Validation Scripts**: validate_build.sh and test_build.sh for verification
- **Cross-Compilation Support**: Easy cross-compilation via compiler variables
- **HFS/HFS+ Detection Framework**: Automatic filesystem type detection
- **Enhanced hfsck**: Supports both HFS and HFS+ with automatic detection
- **Enhanced hformat**: Framework for HFS and HFS+ formatting with type selection
- **Standard Filesystem Utilities**: fsck.hfs, fsck.hfs+, mkfs.hfs, mkfs.hfs+ symlinks
- **Date Limit Handling**: Graceful handling of HFS date limit (February 6, 2040)
- **Program Name Detection**: Utilities behave differently based on invocation name
- **Complete HFS+ Formatting**: Full HFS+ volume creation with proper structures
- **HFS+ Volume Header**: Complete HFS+ superblock creation and initialization
- **HFS+ System Files**: Allocation, extents, catalog, and attributes file setup
- **Block Size Optimization**: Automatic optimal block size selection for volumes
- **Endianness Handling**: Proper big-endian format for all HFS+ structures
- **mkfs.hfs/mkfs.hfs+ Commands**: Standard Unix filesystem creation utilities

### Changed
- **Unified Binary Architecture**: Single hfsutil executable with all utilities
- **Modernized Makefiles**: Updated all Makefiles with modern variable handling
- **Enhanced build.sh**: Now passes environment variables to sub-builds
- **Updated Installation Layout**: Proper Unix-style directory structure
- **Improved Documentation**: Updated README.md with new build system features
- **hfsck Enhancement**: Now detects filesystem type and supports program name variants
- **hformat Enhancement**: Added filesystem type selection (-t option) and program name detection
- **Installation Process**: Now creates standard filesystem utility symlinks
- **Command Integration**: Added mkfs.hfs, mkfs.hfs+, fsck.hfs+ to unified binary

### Removed
- **Legacy Tcl/Tk/X11 Components**: Removed for better maintainability and reduced dependencies
- **Obsolete Build Dependencies**: Simplified build process without autotools complexity

### Fixed
- **Build System Portability**: Resolved issues with modern compilers and systems
- **Include Path Issues**: Fixed header file location problems
- **Symlink Creation**: Proper symbolic link creation for traditional command names
- **HFS Date Validation**: Added proper handling for dates beyond HFS limit
- **Filesystem Detection**: Robust detection of HFS, HFS+, and HFSX filesystems
- **Compiler Warnings**: Fixed unused variable and return value warnings
- **Test Suite Organization**: Moved test scripts to test/ directory for better organization
- **Build Script Validation**: Enhanced build.sh with proper variable handling and validation

## [3.2.6] - 1998-11-02

### Added
- Simple next-CNID verification to scavenging
- HFS_OPT_2048 format option for 2048-byte physical-block file boundaries
- HFS_OPT_NOCACHE mount option to inhibit internal block cache
- HFS_OPT_ZERO format/mount option for zero-initialized blocks

### Fixed
- Problem where volumes were not marked as "cleanly unmounted"
- Scavenging failed to mark all extents from final B*-tree node
- Signed/unsigned compiler compatibility issues

## [3.2.5] - 1998-09-17

### Changed
- Miscellaneous internal `const' and other minor changes

## [3.2.4] - 1998-04-11

### Fixed
- Problem with handling of catalog and extents clump sizes
- 800K size check moved from hfs_format() into v_geometry()

### Changed
- OS interface now operates on full block rather than byte offsets
- Modified `configure' to search for tcl.h/tk.h header files

## [3.2.3] - 1998-04-11

### Fixed
- Problem with B*-tree node splitting code that could cause crashes

## [3.2.2] - 1998-04-11

### Added
- Support for "blessing" the MacOS System Folder
- New option to `hattrib' for setting blessed folder
- New "bless" Tcl volume command

### Changed
- Modified `configure' to use existing Tcl/Tk configuration information

## [3.2.1] - 1998-04-11

### Fixed
- Minor potential problem where BinHex translation might fail to recognize hqx header

## [3.2] - 1998-04-11

### Fixed
- Problem determining medium sizes under unusual conditions

### Changed
- Modified MDB fields to support Sequoia (HFS+) preparation

## [3.1.1] - 1998-04-11

### Fixed
- Problem related to partition locations on large media
- Problem with suid.c on some systems
- Problem with font selection in `xhfs'
- Copyouts no longer append `.txt' to text files with periods in filename

## [3.1] - 1998-04-11

### Added
- Enhanced API for managing partitions: hfs_zero(), hfs_mkpart(), hfs_nparts()
- Bad block sparing to hfs_format()
- Character set translation in "text" transfer mode (MacOS Standard Roman to Latin-1)

### Changed
- `hmount' and friends now properly handle relative pathnames
- hfs_mount() and hfs_format() are more strict with partition numbers
- Changed API for hfs_create() to return open file reference
- MacOS Standard Roman to Latin-1 character translation (not fully reversible)

### Fixed
- Bug in xhfs.tcl missing helper `ctime' procedure

## [3.0b2] - 1998-04-11

### Changed
- Block cache mechanism for consecutive block read/write in single chunks
- Improved efficiency of B*-tree search routines

## [3.0b1] - 1998-04-11

### Added
- GNU `configure' script for automatic configuration
- hfs_vsetattr() routine for modifying volume attributes
- Support for partition data blocks not starting at partition beginning

### Fixed
- Alternate MDB sometimes written to wrong location on non-partitioned media
- Extents Overflow file corruption with heavily fragmented file removal
- Volume allocation routine infinite loop possibility

### Changed
- Numerous internal changes for automatic configuration and portability
- hfs_mount() now honors software volume lock bit
- Significant librsrc development (read-only resource support)

## [2.1] - 1997-12-01

### Added
- Physical block caching for dramatic performance improvement
- New fields to hfsvolent and hfsdirent structures
- hfs_vstat(), hfs_stat(), and hfs_setattr() functions

### Changed
- Further hfsck development
- hfsdirent now includes separate union structure for files and directories

### Fixed
- Problem with `xhfs' terminating with "can't read data" error

## [2.0] - 1997-06-01

### Added
- Setuid-aware program modifications
- hfs_getfork() function

### Changed
- Renamed hfs_fork() to hfs_setfork()
- First non-beta release

## [1.19b] - 1997-04-01

### Added
- HFS globbing interface to Tcl
- Many UNIX-like options to `hls' and `hdir'

### Fixed
- Problem preventing large volume formatting
- Globbing routine now ignores Finder-invisible files

### Changed
- Adapted code for Tcl 7.6 and Tk 4.2
- Updated general documentation
- Continued `hfsck' development

## [1.18b] - 1997-02-01

### Fixed
- Many signed/unsigned argument passing conflicts
- Volume file locking made optional for unsupported systems

### Changed
- hfs_read() and hfs_write() now accept void* buffer pointer
- Portability fixes for AIX and BeOS

## [1.17b] - 1997-01-01

### Added
- Complete `hattrib' implementation for changing HFS file attributes
- `parid' field to hfsdirent structure
- Better error message reporting with pathnames

### Fixed
- Off-by-one error in catalog record key lengths
- Problem with backslash-quoted braces during globbing

### Changed
- Improved Makefile handling
- Regularized volume-unmounted flag handling
- Streamlined interface header files
- Updated libhfs documentation

## [1.16b] - 1996-12-01

### Added
- File locking to hfs_mount() to prevent concurrent access
- Mount flags argument to hfs_mount()
- Better multiple same-device mount handling

### Fixed
- Improved HFS path resolution robustness
- hfs_rename() destination path validity detection
- Problem where renaming volume made it inaccessible

### Changed
- Mount-time scavenging only marks bits, doesn't clear them
- Improved error message reporting for all programs

## [1.15b] - 1996-11-01

### Fixed
- Problem verifying directory thread existence
- Problem with catalog and extents clump sizes handling

### Changed
- Increased buffer size for native HFS-to-HFS copies

## [1.14b] - 1996-10-01

### Added
- Filename globbing support for command-line programs
- Volume bitmap reconstruction for uncleanly unmounted volumes
- Recursive directory deletion in `xhfs'
- Automatic volume flushing every 30 seconds in `xhfs'
- Initial `hfsck' and `librsrc' development

### Changed
- File threads now managed properly during file operations
- Relaxed BinHex and MacBinary header validation

## [1.13b] - 1996-09-01

### Added
- Recursive directory copies in `xhfs'
- Case-only file/directory renaming support
- Multiple file moving with `hrename'
- UNIX pathname "-" support in `hcopy' for stdin/stdout

### Fixed
- Two critical extent record key sorting bugs
- Problem with MacOS default file clump size semantics

## [1.12b] - 1996-08-01

### Added
- Automatic transfer mode selection (-a) for `hcopy'

### Fixed
- Mounting of some problematic partitioned media
- HFS timestamps now relative to current time zone

### Changed
- Increased copyin/copyout buffer sizes for better performance

## [1.11b] - 1996-07-01

### Added
- UNIX-to-HFS BinHex transfers (all transfer modes now complete)
- hfs_setattr() and hfs_fsetattr() routines
- Better `hvol' command output

### Fixed
- HFS filename sorting bug affecting search routines

### Changed
- BinHex implementation now uses stdio library
- Modified installation procedure with separate component targets
- Copyin routines now update file information for MacBinary/BinHex

## [1.10b] - 1996-06-01

### Added
- `hrename' command
- File renaming support in `xhfs'

### Fixed
- Serious bug in directory record updates for long names (>21 chars)

### Changed
- Implemented hfs_rename() and eliminated hfs_move()
- Minor changes to hcwd.c for volume/path changes

## [1.9b] - 1996-05-01

### Changed
- Modularized libhfs code
- Resolved portability issues for different datatypes and endianness

## [1.8.1b] - 1996-04-15

### Fixed
- Various portability bugs

## [1.8b] - 1996-04-01

### Added
- UNIX-to-HFS MacBinary II transfers
- HFS-to-HFS copy support
- Generic `hcopy' command replacing `hcat'

### Fixed
- MacBinary II encoding CRC algorithm bug

### Changed
- UI enhancements to `xhfs'
- Tcl hash tables for maintaining open files and volumes
- Tcl/Tk interface compatibility improvements

## [1.7b] - 1996-03-01

### Added
- File copying from UNIX to HFS in `xhfs' (Text and Raw modes)

### Changed
- Many UI enhancements to `xhfs'

## [1.6b] - 1996-02-01

### Added
- Invisible file attribute support
- UI enhancements to `xhfs' and `hfs'

### Fixed
- Bug preventing file/directory creation at root level

### Changed
- hfs_islocked() now returns 1 for read-only volumes

## [1.5b] - 1996-01-01

### Added
- Complete B*-tree record insertion
- B*-tree record deletion
- File writing and truncation
- `hrmdir' and `hdel' commands
- Current volume concept with hfs_getvol() and hfs_setvol()
- Absolute pathname support for multiple volumes

### Changed
- Files and folders can now be created/deleted without limitations

## [1.4b] - 1995-12-01

### Added
- hfs_setcwd() function
- hfs_umountall() function

### Fixed
- Minor bug causing header node read before directory completion

### Changed
- Optional Tcl/Tk program installation
- Multiple volume mounting returns same reference
- Renamed hfs_cwdid() to hfs_getcwd()
- hfs_umount() now closes all files/directories before unmounting
- `xhfs' can handle same volume on both sides

## [1.3b] - 1995-11-01

### Added
- B*-tree file growth implementation
- New directory creation option in `xhfs'

### Changed
- New directories can be created until disk is full

## [1.2b] - 1995-10-01

### Added
- B*-tree node splitting implementation

### Fixed
- Minor bugs from version 1.1b

### Changed
- New directories can be created until catalog file is full

## [1.1b] - 1995-09-01

### Added
- Full read-only capability
- Limited write capability (directory creation until catalog node full)
- hfs_format() for creating new volumes

## Earlier Versions

See original CHANGES file for complete historical development details from the initial beta releases through version 1.0b.

---

## Legend

- **Added**: New features
- **Changed**: Changes in existing functionality  
- **Deprecated**: Soon-to-be removed features
- **Removed**: Removed features
- **Fixed**: Bug fixes
- **Security**: Vulnerability fixes