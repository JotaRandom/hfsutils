hfsutils
========

![CI](https://github.com/JotaRandom/hfsutils/actions/workflows/ci.yml/badge.svg)

Tools for Reading and Writing Macintosh HFS Volumes
--------------------------------------------------

hfsutils is a collection of tools for manipulating Hierarchical File System (HFS) volumes, the native format used by classic Macintosh computers. This modern port works on Linux, Unix-like systems, and BSD variants, providing a lightweight, terminal-based toolset for accessing HFS media.

This version (4.1.0A.1) is based on the original hfsutils by Robert Leslie (1996-1998), building upon Brock Gunter-Smith's Apple Silicon fork (4.0.0) with extensive additional modernization work. The package has been restructured to remove legacy Tcl/Tk/X11 dependencies, focusing on command-line utilities and C libraries with enterprise-grade features including HFS+ journaling support.

**Authors & Contributors:**
- Original Author: Robert Leslie
- Apple Silicon Fork (4.0.0): Brock Gunter-Smith  
- Extended Version (4.1.0A.1): Pablo Lezaeta
- Repository: https://github.com/JotaRandom/hfsutils
- Original Project: http://www.mars.org/home/rob/proj/hfs/
- License: GNU General Public License, Version 2 (see [COPYRIGHT](COPYRIGHT))
- Changelog: See [CHANGELOG.md](CHANGELOG.md) for version history

Features
--------

**Version Evolution:**
- **Version 4.0.0**: "Apple Silicon fork" - Base modernization by Brock Gunter-Smith
- **Version 4.1.0A.1**: Extended implementation with HFS+ journaling, professional standards, enterprise-grade features, and flexible installation
- **Version 5.0.0**: Future goal - Transparent mount support with standard Unix utilities

- **Media Support**: Read and write HFS volumes on floppy disks, SCSI disks, CD-ROMs, Zip drives, and disk images
- **Partitioned Media**: Full support for partitioned media with multiple HFS volumes
- **Command-Line Tools**: Complete set of utilities for mounting, listing, copying, and formatting HFS volumes
- **Cross-Platform**: Works on modern Linux, Unix-like systems (FreeBSD, NetBSD, OpenBSD), and other POSIX systems
- **Unified Binary**: Single `hfsutil` executable with all utilities, plus optional traditional command symlinks
- **HFS+ Formatting**: Create HFS+ volumes with proper structure and metadata
- **Standard Filesystem Utilities**: mkfs.hfs, mkfs.hfs+, fsck.hfs+ for Unix integration
- **C Libraries**: libhfs and librsrc for developers wanting to integrate HFS support
- **Modern Build System**: Simplified build process without autotools dependencies
- **Comprehensive Testing**: Full test suite for both HFS and HFS+ functionality
- **Clean Code Organization**: Well-structured codebase with common utilities properly organized

**HFS+ Support:**
- ✅ **HFS+ Volume Creation**: Complete HFS+ formatting with proper structures conforming to Apple TN1150
- ✅ **Standard Unix Utilities**: mkfs.hfs, mkfs.hfs+, fsck.hfs+ commands
- ✅ **Filesystem Detection**: Automatic HFS vs HFS+ type detection
- ✅ **Program Name Detection**: Utilities behave based on invocation name
- ✅ **Specification Conformance**: Alternate headers, Volume Header fields, proper signatures
- 🔄 **Full HFS+ Operations**: B-tree initialization and mounting (planned)

**Current Limitations:**
- HFS+ volumes can be created but not yet fully mounted with hfsutil (use system tools)
- Known issue: B-tree compatibility between mkfs.hfs and hfsutil (under investigation)
- Macintosh File System (MFS) for 400K floppies is not supported
- Some 800K floppies may have hardware-related limitations (disk images work fine)

Installation
------------

**Prerequisites:**
- C compiler (gcc or clang)
- Standard Unix tools (make, bash)
- POSIX-compatible system

**Quick Build:**
```bash
git clone https://github.com/JotaRandom/hfsutils.git
cd hfsutils
./build.sh
sudo make install
```

**Manual Build:**
```bash
# Build libraries first
cd libhfs && ./configure && make && cd ..
cd librsrc && ./configure && make && cd ..

# Build main utilities
make

# Install system-wide
sudo make install
```

**Alternative Build (if autotools issues):**
```bash
# Use manual build target to avoid autotools completely
make build-manual
sudo make install
```

**Installation Options:**
```bash
# Standard installation
sudo make install

# Custom installation prefix
make install PREFIX=/opt/hfsutils

# Package building with staging directory
make install DESTDIR=/tmp/staging PREFIX=/usr

# Install with traditional command symlinks (hls, hcopy, etc.)
sudo make install-symlinks
```

**Distribution Packaging:**
```bash
# Standard distribution packaging
make install PREFIX=/usr DESTDIR=/tmp/package-root

# For distributions with merged /bin (like Arch, Fedora 17+)
make install PREFIX=/usr SBINDIR=/usr/bin DESTDIR=/tmp/package-root

# With symlinks for merged systems
make install-symlinks PREFIX=/usr SBINDIR=/usr/bin DESTDIR=/tmp/package-root
```

**Build Customization:**
```bash
# Custom compiler and flags
make CC=clang CFLAGS="-O3 -march=native"

# Environment-based build
export CC=gcc-11
export CFLAGS="-O2 -g -fstack-protector-strong"
./build.sh

# Cross-compilation
make CC=aarch64-linux-gnu-gcc CFLAGS="-O2"
```

**Modern Systems Compatibility:**

Many modern Linux distributions have merged `/sbin` into `/bin` for simplicity. hfsutils automatically handles this:

- **Traditional systems** (RHEL/CentOS 6, Debian 7, Ubuntu 14.04 and older): Use default settings
- **Merged systems** (Arch Linux, Fedora 17+, Debian 8+, Ubuntu 15.04+): Use `SBINDIR=/usr/bin`

The build system automatically detects merged systems and creates appropriate symlinks. System utilities like `hfsck` are installed to `SBINDIR`, while user utilities like `hfsutil` go to `BINDIR`. All installation paths are fully configurable.

**Installation Variables:**
- `PREFIX`: Installation prefix (default: `/usr/local`)
- `BINDIR`: User binaries directory (default: `PREFIX/bin`)
- `SBINDIR`: System binaries directory (default: `PREFIX/sbin`)
- `DESTDIR`: Staging directory for package building

**Note**: All system utilities (`hfsck`, filesystem checkers) now properly respect the `SBINDIR` variable, ensuring correct installation paths for both traditional and merged filesystem layouts.

**Build Targets:**
- `make` - Build hfsutil executable and libraries
- `make install` - Install binaries, libraries, and manual pages
- `make install-symlinks` - Install with traditional command names
- `make symlinks` - Create command symlinks (for testing)
- `make test` - Run test suite
- `make clean` - Remove built files
- `make help` - Show all available targets and variables

For detailed build system documentation, see [BUILD.md](BUILD.md).

## Testing

The project includes a comprehensive test suite for both HFS and HFS+ functionality:

```bash
# Run all tests
cd test && ./run_tests.sh

# Run specific test categories
./run_tests.sh basic      # Basic functionality tests
./run_tests.sh hfsplus    # HFS+ specific tests
./run_tests.sh integration # Integration workflow tests
./run_tests.sh errors     # Error handling tests

# Generate test data (HFS and HFS+ images)
./generate_test_data.sh

# Run complete HFS+ validation
./test_hfsplus_complete.sh
```

**Test Coverage:**
- ✅ HFS volume creation, mounting, and file operations
- ✅ HFS+ volume formatting and structure validation
- ✅ Filesystem type detection (HFS vs HFS+)
- ✅ Program name detection (mkfs.hfs, mkfs.hfs+, fsck.hfs+)
- ✅ Mixed HFS/HFS+ environment compatibility
- ✅ Error handling and edge cases
- ✅ Build system validation across platforms

## Documentation

### User Documentation
- **[BUILD.md](BUILD.md)** - Comprehensive build system documentation
- **[CHANGELOG.md](CHANGELOG.md)** - Complete version history and changes
- **[COPYRIGHT](COPYRIGHT)** - License and copyright information
- **[PACKAGING.md](PACKAGING.md)** - Distribution packaging guidelines
- **[PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md)** - Project organization

### Technical Documentation (./doc/)
- **[TN1150_HFS_PLUS_VOLUME_FORMAT.md](doc/TN1150_HFS_PLUS_VOLUME_FORMAT.md)** - Apple's official HFS+ specification
- **[HFS_CLASSIC_SPECIFICATION.md](doc/HFS_CLASSIC_SPECIFICATION.md)** - HFS Classic complete specification
- **[WIKIPEDIA_HFS_PLUS.md](doc/WIKIPEDIA_HFS_PLUS.md)** - Wikipedia reference for HFS+
- **[HFS_IMPLEMENTATION_NOTES.md](doc/HFS_IMPLEMENTATION_NOTES.md)** - Practical implementation guide
- **[DEVELOPMENT_HISTORY.md](doc/DEVELOPMENT_HISTORY.md)** - Project evolution and lessons learned
- **[README.md](doc/README.md)** - Documentation index and quick reference

Usage
-----

The tools are provided as a single `hfsutil` executable with subcommands. You can also create traditional command symlinks for backward compatibility.

**Command Structure:**
```bash
hfsutil <command> [options]
```

**Available Commands:**
| Command | Description |
|---------|-------------|
| `hmount` | Mount an HFS volume |
| `humount` | Unmount an HFS volume |
| `hls` | List files in HFS directory |
| `hcd` | Change HFS directory |
| `hpwd` | Show current HFS directory |
| `hcopy` | Copy files to/from HFS volume |
| `hdel` | Delete HFS files |
| `hmkdir` | Create HFS directory |
| `hrmdir` | Remove HFS directory |
| `hrename` | Rename HFS files |
| `hattrib` | Show/modify HFS file attributes |
| `hvol` | Display HFS volume information |
| `hformat` | Format HFS and HFS+ volumes |
| `hfsck` | Check and repair HFS and HFS+ volumes |

**Getting Help:**
```bash
# Show available commands
hfsutil

# Get help for a specific command
hfsutil <command> --help
```

## Filesystem Utilities

hfsutils includes dedicated filesystem utilities that automatically detect and handle both HFS and HFS+ filesystems:

### Filesystem Checking (hfsck)

```bash
# Check filesystem (auto-detects HFS/HFS+)
hfsutil hfsck /dev/disk1s2

# Check with verbose output
hfsutil hfsck -v /dev/disk1s2

# Check without repairs (read-only)
hfsutil hfsck -n /dev/disk1s2

# Standard names (via symlinks)
fsck.hfs /dev/disk1s2      # Forces HFS checking
fsck.hfs+ /dev/disk1s3     # Forces HFS+ checking
```

### Filesystem Formatting (hformat)

```bash
# Format as HFS (default)
hfsutil hformat -l "My Disk" /dev/disk1s2

# Format as HFS+ (fully implemented)
hfsutil hformat -t hfs+ -l "My HFS+ Disk" /dev/disk1s2

# Force format (overwrite partitions)
hfsutil hformat -f -l "New Disk" /dev/disk1

# Standard Unix filesystem utilities
mkfs.hfs -l "My Disk" /dev/disk1s2        # Format as HFS
mkfs.hfs+ -l "My HFS+ Disk" /dev/disk1s2  # Format as HFS+
mkfs.hfsplus -l "My Disk" /dev/disk1s2    # Alternative HFS+ name

# Filesystem checking
fsck.hfs+ /dev/disk1s2                    # Check HFS+ volume
hfsck /dev/disk1s2                        # Auto-detect and check
```

**Filesystem Type Detection:**
- `hfsck` automatically detects HFS, HFS+, and HFSX filesystems
- `hformat` defaults to HFS, supports HFS+ with `-t hfs+` option
- Standard names (`fsck.hfs+`, `mkfs.hfs+`) enforce specific filesystem types
- Program name detection: utilities behave based on how they're invoked
- All utilities handle the HFS date limit (February 6, 2040) gracefully

**Basic Examples:**
```bash
# Mount an HFS volume
hfsutil hmount /dev/disk2

# List files in current HFS directory
hfsutil hls

# List files with details (like ls -l)
hfsutil hls -l

# Copy a file from HFS to local system
hfsutil hcopy :filename ./filename

# Copy a file to HFS volume
hfsutil hcopy ./filename :filename

# Create and format a new HFS disk image
dd if=/dev/zero of=disk.hfs bs=1024 count=1440
hfsutil hformat -l "My Disk" disk.hfs

# Format as HFS+ (fully supported)
hfsutil hformat -t hfs+ -l "My HFS+ Disk" disk.img

# Check filesystem integrity
hfsutil hfsck -v disk.hfs

# Check volume information
hfsutil hvol
```

**Working with Disk Images:**
```bash
# Mount a disk image
hfsutil hmount disk_image.img

# Navigate and explore
hfsutil hls -la
hfsutil hcd :System:Extensions
hfsutil hpwd

# Copy files for recovery
hfsutil hcopy :important_file ./recovered_file
hfsutil hcopy :folder: ./recovered_folder -r

# Unmount when done
hfsutil humount
```

**Using Traditional Command Names:**
If you prefer the classic command names, create symlinks:
```bash
# Create symlinks (after building)
make symlinks

# Now you can use traditional commands
hls -l
hcopy :file ./file
hmount disk.img

# Or use standard filesystem utilities
fsck.hfs /dev/disk1s2
mkfs.hfs -l "My Disk" /dev/disk1s2
fsck.hfs+ /dev/disk1s3
mkfs.hfs+ -l "My HFS+ Disk" /dev/disk1s3
```

Developer Libraries
-------------------

hfsutils includes C libraries for developers who want to integrate HFS support into their applications:

**libhfs** - Core HFS filesystem library:
- Low-level HFS volume access
- File and directory operations
- B*-tree manipulation
- Volume formatting and mounting

**librsrc** - Resource fork library:
- Read Macintosh resource forks
- Resource data extraction
- Compatible with or without libhfs

**Usage Example:**
```c
#include <hfs.h>

hfsvol *vol;
hfsfile *file;

// Mount HFS volume
vol = hfs_mount("/path/to/volume", 0, HFS_MODE_RDWR);

// Open file
file = hfs_open(vol, "filename");

// Read data
char buffer[1024];
hfs_read(file, buffer, sizeof(buffer));

// Clean up
hfs_close(file);
hfs_umount(vol);
```

**Installation for Development:**
```bash
# Install libraries and headers
sudo make install

# Libraries will be installed to /usr/local/lib/
# Headers will be installed to /usr/local/include/
```

Troubleshooting
---------------

**Build Issues:**
For errors like "config.status: No such file or directory":
```bash
make clean
./build.sh

# For persistent issues, try a complete clean
make distclean
./build.sh
```

**Common Problems:**
- **"config.status: No such file or directory"**: Run `./build.sh` to configure libraries
- **Permission denied on devices**: Use `sudo` or add user to appropriate groups
- **"hfsutil: command not found"**: Ensure `/usr/local/bin` is in your PATH

**Runtime Issues:**
```bash
# Check if volume is already mounted
hfsutil hvol

# Force unmount if needed
hfsutil humount -f

# Verify HFS volume integrity
hfsutil hfsck /path/to/volume
```

**Permission Issues:**
For installation problems:
```bash
sudo make install
```

**Security Notes:**
- Do not install as setuid root; it’s unnecessary and risky.
- Use device permissions or setgid for device access if needed.
- Consider using proper group membership for controlled device access in multi-user environments

Contributing
------------

Contributions are welcome! This project aims to maintain a modern, portable HFS toolset.

**How to Contribute:**
1. Fork the repository on GitHub
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes and test on supported Unix-like systems
4. Commit your changes (`git commit -m 'Add amazing feature'`)
5. Push to the branch (`git push origin feature/amazing-feature`)
6. Open a Pull Request

**Development Guidelines:**
- Follow existing code style and conventions
- Test changes on multiple platforms when possible
- Update documentation for new features
- Run the test suite: `make test`

**Reporting Issues:**
When reporting bugs or issues, please include:
- Operating system and architecture (e.g., "Linux x86_64", "FreeBSD arm64")
- HFS volume type and source (floppy disk, CD-ROM, disk image, etc.)
- Complete command line used and full error messages
- Steps to reproduce the issue
- Expected vs. actual behavior

**Areas for Contribution:**
- HFS+ (Extended Format) support implementation
- Additional platform testing and compatibility fixes
- Performance improvements
- Documentation improvements
- Test coverage expansion

Project History & Acknowledgments
---------------------------------

**Original Development:**
- **Robert Leslie** (1996-1998): Original author and creator of hfsutils
- **Original Project**: http://www.mars.org/home/rob/proj/hfs/

**Modern Development:**
- **Brock Gunter-Smith**: Apple Silicon fork (4.0.0) and initial modernization work
- **Pablo Lezaeta**: Extended version (4.1.0A.1) with journaling, professional standards, enterprise features, and flexible installation
- **Community Contributors**: Various patches, bug reports, and improvements

**Technical Acknowledgments:**
- HFS globbing code inspired by Tcl filename globbing (John Ousterhout)
- BinHex encoding/decoding based on BinHex 4.0 specification (Peter N Lewis)
- `hls` command based on GNU `ls` implementation
- Thanks to the many beta testers and contributors listed in the CREDITS file

**Special Thanks:**
This project builds upon decades of work by the original author and contributors who created a robust, portable HFS implementation. Version 4.1.0A.1 extends the Apple Silicon fork (4.0.0) with enterprise-grade features including HFS+ journaling support, professional standards compliance, comprehensive testing, and flexible installation system for modern distributions. The ultimate goal (5.0.0) is transparent filesystem mounting integration with standard Unix utilities.

For a complete list of historical contributors, see the `CREDITS` file in the repository.
