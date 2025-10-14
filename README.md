hfsutils
========

Tools for Reading and Writing Macintosh HFS Volumes
--------------------------------------------------

hfsutils is a software package for manipulating Hierarchical File System (HFS) volumes, the native format used by Macintosh computers, on modern Linux and Unix-like systems, including BSD variants. This is a manual re-port of Brock Gunter-Smith's Apple Silicon fork, which builds on the original hfsutils by Robert Leslie (1996–1998) and includes patches from an earlier fork by the current maintainer. This version ensures compatibility with modern Unix systems while keeping a lightweight, terminal-based toolset.

Original Author: Robert Leslie
Original Copyright: Copyright (C) 1996-1998 Robert Leslie
Apple Silicon Fork: Brock Gunter-Smith
Repository: https://github.com/JotaRandom/hfsutils
Original Project: http://www.mars.org/home/rob/proj/hfs/
License: GNU General Public License, Version 2

Features
--------

- Read and write HFS volumes on media like floppy disks, SCSI disks, CD-ROMs, Zip drives, and disk images.
- Support for partitioned media.
- Command-line tools for mounting, listing, copying, and formatting HFS volumes.
- Works with modern Linux and Unix-like systems (e.g., FreeBSD, NetBSD, OpenBSD).
- Removed old Tcl/Tk/X11 interfaces for a terminal-only focus.
- HFS+ (Extended Format) support is planned but not yet available.

Note: The obsolete Macintosh File System (MFS) for 400K floppies is not supported. Some 800K floppies may not work Prospectively due to hardware limitations, but disk images are supported.

Installation
------------

Prerequisites
- A C compiler (e.g., gcc or clang).
- Standard Unix tools (make, etc.).
- No extra dependencies, as Tcl/Tk/X11 support is removed.

Quick Build
  git clone https://github.com/JotaRandom/hfsutils.git
  cd hfsutils
  ./build.sh
  sudo make install

Manual Build
  # Build libraries
  cd libhfs && ./configure && make
  cd ../librsrc && ./configure && make
  # Build main utility
  cd .. && make
  sudo make install

Configuration Options
- --disable-cli: Skip building command-line tools.
- --enable-devlibs: Install developer libraries.
- --enable-debug: Add diagnostic debugging support.

Usage
-----

The tools are provided as a single `hfsutil` executable with subcommands.

Command Structure
  hfsutil <command> [options]

Available Commands
  hmount   Mount an HFS volume
  humount  Unmount an HFS volume
  hls      List files in HFS directory
  hcd      Change HFS directory
  hpwd     Show current HFS directory
  hcopy    Copy files to/from HFS volume
  hdel     Delete HFS files
  hmkdir   Create HFS directory
  hrmdir   Remove HFS directory
  hrename  Rename HFS files
  hattrib  Show/modify HFS file attributes
  hvol     Display HFS volume information
  hformat  Format HFS volumes

Basic Examples
  # Mount an HFS volume
  hfsutil hmount /dev/disk2

  # List files
  hfsutil hls

  # Copy a file from HFS to local system
  hfsutil hcopy :filename ./filename

  # Copy a file to HFS volume
  hfsutil hcopy ./filename :filename

  # Format a new HFS volume
  dd if=/dev/zero of=disk.hfs bs=1024 count=1440
  hfsutil hformat -l "My Disk" disk.hfs

Working with Disk Images
  # Mount a disk image
  hfsutil hmount disk_image.img myimage

  # List files and copy
  hfsutil hls
  hfsutil hcopy :important_file ./recovered_file

  # Unmount
  hfsutil humount

Creating Symlinks (Optional)
  To use traditional command names (e.g., hcopy, hls):
  make symlinks

Troubleshooting
---------------

Build Issues
  For errors like "config.status: No such file or directory":
  make clean
  cd libhfs && make clean
  cd ../librsrc && make clean
  cd .. && ./build.sh

Permission Errors
  For installation issues:
  sudo make install

Security Notes
- Do not install as setuid root; it’s unnecessary and risky.
- Use device permissions or setgid for device access if needed.

Contributing
------------

Contributions are welcome! To contribute:
1. Fork the repository.
2. Create a feature branch.
3. Make and test changes on supported Unix-like systems.
4. Submit a pull request.

When reporting issues, include:
- Operating system and architecture.
- HFS volume type and source (e.g., floppy, disk image).
- Full command line and error messages.
- Steps to reproduce the issue.

Acknowledgments
---------------

- Robert Leslie: Original author of hfsutils.
- Brock Gunter-Smith: Apple Silicon fork and contributions.
- Contributors: Community members for patches and improvements.
