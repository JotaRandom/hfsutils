# Project Structure

This document describes the organization of the hfsutils project, including source code, documentation, tests, and build artifacts.

## Directory Structure

```
hfsutils/
├── .github/                 # GitHub configuration and workflows
├── build/                   # Build artifacts (generated)
│   ├── obj/                # Object files
│   └── standalone/         # Standalone binaries (mkfs.hfs, fsck.hfs+, etc.)
├── config/                  # Build configuration files
├── doc/                     # Technical documentation
│   ├── TN1150_HFS_PLUS_VOLUME_FORMAT.md    # Apple HFS+ specification
│   ├── HFS_CLASSIC_SPECIFICATION.md        # HFS Classic specification
│   ├── WIKIPEDIA_HFS_PLUS.md               # Wikipedia HFS+ reference
│   ├── HFS_IMPLEMENTATION_NOTES.md         # Implementation guide
│   ├── DEVELOPMENT_HISTORY.md              # Project evolution
│   ├── README.md                           # Documentation index
│   ├── man/                # Manual pages
│   │   ├── man1/          # User command man pages
│   │   └── man8/          # System admin man pages
│   └── *.txt              # Legacy documentation files
├── hfsck/                   # HFS/HFS+ filesystem checker
│   ├── ck_*.c             # Check modules (btree, mdb, volume)
│   ├── hfsck.c            # Main checker
│   ├── journal.c          # Journal handling
│   └── util.c             # Checker utilities
├── include/                 # Public header files
│   ├── common/            # Common utilities headers
│   ├── hfsutil/           # hfsutil headers
│   └── config.h           # Build configuration
├── libhfs/                  # Core HFS library
│   ├── *.c, *.h           # HFS implementation
│   └── os/                # OS-specific code
├── librsrc/                 # Resource fork library
│   ├── *.c, *.h           # Resource handling
├── linux/                   # Linux-specific utilities
├── scripts/                 # Build and utility scripts
├── src/                     # Source code
│   ├── binhex/            # BinHex encoding/decoding
│   ├── common/            # Common utilities and version info
│   ├── embedded/          # Embedded minimal libhfs for standalone tools
│   ├── fsck/              # Standalone fsck utilities
│   ├── hfsutil/           # Main hfsutil command utilities
│   ├── mkfs/              # Standalone mkfs utilities
│   ├── mount/             # Mount/unmount utilities
│   └── tests/             # Unit tests
├── test/                    # Integration tests and test data
│   ├── data/              # Test data files
│   ├── temp/              # Temporary test files
│   ├── run_tests.sh       # Main test runner
│   ├── test_*.sh          # Individual test scripts
│   └── generate_test_data.sh  # Test data generator
├── tests/                   # Additional test suite
├── BUILD.md                 # Build system documentation
├── build.sh                 # Quick build script
├── build.standalone.sh      # Standalone utilities build
├── CHANGELOG.md             # Version history
├── clean.sh                 # Cleanup script
├── configure                # Main configure script
├── configure.standalone     # Standalone configure
├── COPYRIGHT                # License information
├── CREDITS                  # Contributors
├── Makefile                 # Main makefile
├── Makefile.standalone      # Standalone makefile
├── PACKAGING.md             # Distribution packaging guide
├── PROJECT_STRUCTURE.md     # This file
├── README.md                # Main project documentation
└── TODO                     # Development tasks
```

## Key Directories Explained

### Source Code (`src/`)

**Organized by component:**
- **`binhex/`** - BinHex encoding/decoding for Macintosh files
- **`common/`** - Shared utilities (version info, error handling, common functions)
- **`embedded/`** - Minimal libhfs subset for standalone tools (reduces dependencies)
- **`fsck/`** - Standalone filesystem checker utilities (fsck.hfs, fsck.hfs+)
- **`hfsutil/`** - Main hfsutil command and traditional utilities (hls, hcopy, hmount, etc.)
- **`mkfs/`** - Standalone filesystem creation utilities (mkfs.hfs, mkfs.hfs+)
- **`mount/`** - Mount/unmount utilities (mount.hfs, umount.hfs)
- **`tests/`** - Unit tests for individual components

### Documentation (`doc/`)

**Complete offline reference:**
- **Specifications:** Apple TN1150, HFS Classic spec, Wikipedia references
- **Implementation:** Practical guide with code examples and validation
- **History:** Project evolution, lessons learned, development timeline
- **Man Pages:** User and system administrator documentation

### Tests (`test/` and `tests/`)

**Comprehensive test coverage:**
- **`test/`** - Integration tests, test data, and main test runner
- **`tests/`** - Additional test suite
- **Test scripts:** Specification conformance, integrity checks, cross-compatibility

### Libraries

**Core HFS implementation:**
- **`libhfs/`** - Full-featured HFS library (for hfsutil)
- **`librsrc/`** - Resource fork handling
- **`src/embedded/`** - Minimal subset for standalone tools

### Build System

**Multiple build options:**
- **`Makefile`** - Main build system (full hfsutils)
- **`Makefile.standalone`** - Standalone utilities only
- **`build.sh`** - Quick one-step build
- **`configure`** - Auto-configuration for libraries
- **`config/`** - Build configuration templates

## Build Targets

### Main Targets
- `make` or `make all` - Build hfsutil executable and libraries
- `make install` - Install binaries, libraries, and man pages
- `make install-symlinks` - Install with traditional command names
- `make clean` - Remove build artifacts
- `make distclean` - Complete cleanup including configuration
- `make test` - Run test suite
- `make help` - Show all available targets

### Standalone Utilities
- `make standalone` - Build all standalone utilities (mkfs, fsck, etc.)
- `make build/standalone/mkfs.hfs` - Build HFS mkfs
- `make build/standalone/mkfs.hfs+` - Build HFS+ mkfs
- `make build/standalone/fsck.hfs+` - Build HFS+ fsck
- `make hfsck/hfsck` - Build filesystem checker

### Utility Targets
- `make symlinks` - Create command symlinks for testing
- `make clean-symlinks` - Remove symlinks

## Installation Paths

**Default installation (PREFIX=/usr/local):**
```
/usr/local/
├── bin/
│   ├── hfsutil              # Main unified binary
│   ├── hls                  # Symlink to hfsutil (optional)
│   ├── hcopy                # Symlink to hfsutil (optional)
│   └── ...                  # Other command symlinks
├── sbin/
│   ├── hfsck                # Filesystem checker
│   ├── mkfs.hfs             # HFS formatter
│   ├── mkfs.hfs+            # HFS+ formatter
│   ├── fsck.hfs+            # HFS+ checker
│   └── ...                  # Symlinks (mkfs.hfsplus, fsck.hfsplus)
├── lib/
│   ├── libhfs.a             # HFS library
│   └── librsrc.a            # Resource fork library
├── include/
│   ├── hfs.h                # HFS library header
│   └── rsrc.h               # Resource library header
└── share/man/
    ├── man1/                # User commands
    └── man8/                # System commands
```

**Merged /bin systems (Arch, Fedora 17+, Debian 8+):**
```bash
make install PREFIX=/usr SBINDIR=/usr/bin
```

## File Organization

### Root Directory Files

**Build System:**
- `Makefile`, `Makefile.standalone` - Build definitions
- `build.sh`, `build.standalone.sh` - Build scripts
- `clean.sh` - Cleanup script
- `configure`, `configure.standalone` - Configuration scripts
- `config.mk`, `config.h.in` - Generated configuration

**Documentation:**
- `README.md` - Main project documentation
- `BUILD.md` - Build system guide
- `CHANGELOG.md` - Version history
- `PACKAGING.md` - Distribution packaging
- `PROJECT_STRUCTURE.md` - This file
- `COPYRIGHT` - License (GPL v2)
- `CREDITS` - Contributors
- `INSTALL` - Installation guide
- `TODO` - Development tasks

### Generated Files (not in git)

**Build artifacts:**
- `build/obj/` - Object files
- `build/standalone/` - Standalone binaries
- `config.mk` - Generated configuration
- `*.o` - Object files in source directories

**Test artifacts:**
- `test/temp/` - Temporary test files
- `test/data/` - Generated test data
- `*.img`, `*.hfs`, `*.hfsplus` - Test disk images

## Development Workflow

### Adding New Features

1. **Update source code** in appropriate `src/` subdirectory
2. **Add tests** in `test/` directory
3. **Update documentation:**
   - User-facing: Update `README.md`
   - Technical: Add to `doc/HFS_IMPLEMENTATION_NOTES.md`
   - Build changes: Update `BUILD.md`
4. **Update CHANGELOG.md** with changes
5. **Test:** Run `make test` and validate changes

### Documentation Updates

- **Specifications:** `doc/TN1150_HFS_PLUS_VOLUME_FORMAT.md`, `doc/HFS_CLASSIC_SPECIFICATION.md`
- **Implementation:** `doc/HFS_IMPLEMENTATION_NOTES.md`
- **History:** `doc/DEVELOPMENT_HISTORY.md`
- **Quick reference:** `doc/README.md`

### Testing

```bash
# Run all tests
cd test && ./run_tests.sh

# Run specific test category
./run_tests.sh basic
./run_tests.sh hfsplus

# Run individual test
bash test_hfs_spec_validation.sh
```

## Clean Directory Structure

**Eliminated scattered files:**
- ❌ Removed: 9 summary .md files from root (consolidated in `doc/DEVELOPMENT_HISTORY.md`)
- ❌ Removed: 7 test scripts from root (moved to `test/`)
- ❌ Removed: Temporary log files (test_output.txt, build_output.log, etc.)
- ✅ Organized: All documentation in `doc/`
- ✅ Organized: All tests in `test/`
- ✅ Organized: All source code in `src/`

**Result:** Clean, navigable root directory with clear organization.
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