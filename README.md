# hfsutils

## Tools for Reading and Writing Macintosh HFS Volumes

**hfsutils** is a software package designed to manipulate Hierarchical File System (HFS) volumes, the native volume format used by Macintosh computers, on modern Linux and Unix-like systems, including BSD variants. This fork updates the original hfsutils project to ensure compatibility with contemporary operating systems while maintaining its core functionality as a terminal-based toolset.

Originally developed by Robert Leslie in 1996–1998, this fork integrates improvements from various other forks and aims to provide a robust, lightweight solution for working with HFS volumes. The project is licensed under the GNU General Public License, Version 2.

---

## Features

- Read and write HFS volumes on various media (e.g., floppy disks, SCSI disks, CD-ROMs, Zip drives, image files).
- Support for partitioned media.
- Command-line utilities for mounting, listing, copying, and formatting HFS volumes.
- Compatibility with modern Linux distributions and Unix-like systems (e.g., FreeBSD, NetBSD, OpenBSD).
- Removed deprecated Tcl/Tk/X11 interfaces to focus on terminal-based functionality.
- Planned support for Apple’s HFS+ (Extended Format) is under consideration but not yet implemented.

**Note**: The obsolete Macintosh File System (MFS) used on 400K floppies is not supported. Additionally, 800K Macintosh floppies may not be readable on some systems due to hardware limitations, though disk images of these volumes are supported.

---

## Installation

This project uses GNU Autoconf for configuration, enabling straightforward compilation and installation on modern systems. Follow these steps to install hfsutils:

1. **Prerequisites**:
   - A C compiler (e.g., `gcc` or `clang`).
   - GNU Autoconf and Automake (for building from source).
   - Standard Unix development tools (`make`, etc.).
   - No additional dependencies are required, as Tcl/Tk/X11 support has been removed.

2. **Building and Installing**:
   ```bash
   ./configure
   make
   sudo make install
