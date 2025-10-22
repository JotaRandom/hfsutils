# Distribution Packaging Guide

This guide provides instructions for packaging hfsutils for different Linux distributions.

## Quick Reference

### Standard Distribution Packaging
```bash
# Most distributions
make install PREFIX=/usr DESTDIR="$pkgdir"

# Distributions with merged /bin (Arch, Fedora 17+, etc.)
make install PREFIX=/usr SBINDIR=/usr/bin DESTDIR="$pkgdir"

# With traditional command symlinks
make install-symlinks PREFIX=/usr SBINDIR=/usr/bin DESTDIR="$pkgdir"
```

## Distribution-Specific Examples

### Arch Linux (PKGBUILD)
```bash
# In build() function
make

# In package() function
make install PREFIX=/usr SBINDIR=/usr/bin DESTDIR="$pkgdir"
make install-symlinks PREFIX=/usr SBINDIR=/usr/bin DESTDIR="$pkgdir"
```

### Debian/Ubuntu (.deb)
```bash
# Traditional Debian (separate /sbin)
make install PREFIX=/usr DESTDIR=debian/tmp

# Modern Debian/Ubuntu (merged /bin since Debian 8, Ubuntu 15.04)
make install PREFIX=/usr SBINDIR=/usr/bin DESTDIR=debian/tmp
```

### Fedora/RHEL (.rpm)
```bash
# Fedora 17+ (merged /bin)
make install PREFIX=/usr SBINDIR=/usr/bin DESTDIR=%{buildroot}

# RHEL/CentOS 6 and older (separate /sbin)
make install PREFIX=/usr DESTDIR=%{buildroot}
```

### openSUSE (.rpm)
```bash
# openSUSE (merged /bin since 12.3)
make install PREFIX=/usr SBINDIR=/usr/bin DESTDIR=%{buildroot}
```

## Installation Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `PREFIX` | `/usr/local` | Installation prefix |
| `DESTDIR` | (empty) | Staging directory for package building |
| `BINDIR` | `$(PREFIX)/bin` | User binaries directory |
| `SBINDIR` | `$(PREFIX)/sbin` | System binaries directory |
| `LIBDIR` | `$(PREFIX)/lib` | Libraries directory |
| `INCLUDEDIR` | `$(PREFIX)/include` | Header files directory |
| `MANDIR` | `$(PREFIX)/share/man` | Manual pages directory |

## Files Installed

### Binaries
- `$(BINDIR)/hfsutil` - Main unified binary
- `$(SBINDIR)/hfsck` - Filesystem checker

### Libraries (with install-libs)
- `$(LIBDIR)/libhfs.a` - HFS filesystem library
- `$(LIBDIR)/librsrc.a` - Resource fork library

### Headers (with install-libs)
- `$(INCLUDEDIR)/hfs.h` - HFS library header
- `$(INCLUDEDIR)/rsrc.h` - Resource library header

### Manual Pages
- `$(MANDIR)/man1/hfsutil.1` - Main utility manual
- `$(MANDIR)/man1/h*.1` - Individual command manuals
- `$(MANDIR)/man8/hfsck.8` - Filesystem checker manual
- `$(MANDIR)/man8/mkfs.hfs.8` - Filesystem creation manual

### Optional Symlinks (with install-symlinks)
- `$(BINDIR)/hls`, `$(BINDIR)/hcopy`, etc. - Traditional command names
- `$(BINDIR)/fsck.hfs`, `$(BINDIR)/fsck.hfs+` - Standard filesystem utilities
- `$(BINDIR)/mkfs.hfs`, `$(BINDIR)/mkfs.hfs+` - Standard creation utilities

## Modern Systems Compatibility

### Merged /bin Systems
Many modern distributions have merged `/sbin` into `/bin`:
- **Arch Linux** (all versions)
- **Fedora 17+**
- **Debian 8+**
- **Ubuntu 15.04+**
- **openSUSE 12.3+**

For these systems, use `SBINDIR=/usr/bin` to install system utilities in the correct location.

### Traditional Systems
Older distributions maintain separate `/sbin`:
- **RHEL/CentOS 6 and older**
- **Debian 7 and older**
- **Ubuntu 14.04 and older**

For these systems, use the default `SBINDIR=/usr/sbin`.

## Build Dependencies

### Required
- C compiler (gcc or clang)
- make
- Standard POSIX utilities

### Optional (for autotools)
- autoconf
- autoheader

The build system automatically falls back to manual compilation if autotools are not available or fail.

## Testing Installation

After packaging, verify the installation:

```bash
# Check main binary
/usr/bin/hfsutil --version

# Check filesystem utilities
/usr/bin/hfsck --version  # or /usr/sbin/hfsck
/usr/bin/fsck.hfs --version

# Check symlinks (if installed)
/usr/bin/hls --version
/usr/bin/mkfs.hfs --version
```

## Package Metadata

### Description
Tools for reading and writing Macintosh HFS volumes. Provides utilities for mounting, checking, and formatting HFS and HFS+ filesystems.

### Categories
- System utilities
- Filesystem tools
- File management

### Dependencies
- libc
- No additional runtime dependencies

## Version Information

Current version: **4.1.0A**
- Extended implementation with HFS+ journaling support
- Professional standards compliance
- Enterprise-grade features
- Modern build system with distribution packaging support