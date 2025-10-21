hfsutils
========

Tools for Reading and Writing Macintosh HFS Volumes
--------------------------------------------------

hfsutils is a collection of tools for manipulating Hierarchical File System (HFS) volumes, the native format used by classic Macintosh computers. This modern port works on Linux, Unix-like systems, and BSD variants, providing a lightweight, terminal-based toolset for accessing HFS media.

This version is based on the original hfsutils by Robert Leslie (1996-1998), with contributions from Brock Gunter-Smith's Apple Silicon fork and additional modernization work. The package has been restructured to remove legacy Tcl/Tk/X11 dependencies, focusing on command-line utilities and C libraries.

**Authors & Contributors:**
- Original Author: Robert Leslie
- Apple Silicon Fork: Brock Gunter-Smith  
- Current Maintainer: Pablo Lezaeta
- Repository: https://github.com/JotaRandom/hfsutils
- Original Project: http://www.mars.org/home/rob/proj/hfs/
- License: GNU General Public License, Version 2

Features
--------

- **Media Support**: Read and write HFS volumes on floppy disks, SCSI disks, CD-ROMs, Zip drives, and disk images
- **Partitioned Media**: Full support for partitioned media with multiple HFS volumes
- **Command-Line Tools**: Complete set of utilities for mounting, listing, copying, and formatting HFS volumes
- **Cross-Platform**: Works on modern Linux, Unix-like systems (FreeBSD, NetBSD, OpenBSD), and other POSIX systems
- **Unified Binary**: Single `hfsutil` executable with all utilities, plus optional traditional command symlinks
- **C Libraries**: libhfs and librsrc for developers wanting to integrate HFS support
- **Modern Build System**: Simplified build process without autotools dependencies

**Current Limitations:**
- HFS+ (Extended Format) support is planned but not yet implemented
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

**Installation Options:**
```bash
# Install just the unified hfsutil binary
sudo make install

# Install with traditional command symlinks (hls, hcopy, etc.)
sudo make install-symlinks

# Create symlinks without installing (for testing)
make symlinks
```

**Build Targets:**
- `make` - Build hfsutil executable
- `make symlinks` - Create traditional command symlinks
- `make test` - Run test suite
- `make clean` - Remove built files
- `make help` - Show all available targets

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
| `hformat` | Format HFS volumes |
| `hfsck` | Check and repair HFS volumes |

**Getting Help:**
```bash
# Show available commands
hfsutil

# Get help for a specific command
hfsutil <command> --help
```

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
- **Brock Gunter-Smith**: Apple Silicon fork and modernization work
- **Pablo Lezaeta**: Current maintainer and additional improvements
- **Community Contributors**: Various patches, bug reports, and improvements

**Technical Acknowledgments:**
- HFS globbing code inspired by Tcl filename globbing (John Ousterhout)
- BinHex encoding/decoding based on BinHex 4.0 specification (Peter N Lewis)
- `hls` command based on GNU `ls` implementation
- Thanks to the many beta testers and contributors listed in the CREDITS file

**Special Thanks:**
This project builds upon decades of work by the original author and contributors who created a robust, portable HFS implementation. The goal of this modern fork is to preserve and extend this valuable tool for contemporary systems while maintaining compatibility with the original design principles.

For a complete list of historical contributors, see the `CREDITS` file in the repository.
