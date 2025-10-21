# Build System Documentation

## Overview

The hfsutils build system has been modernized to support standard Unix build conventions including `DESTDIR`, custom installation paths, and compiler/flag customization.

## Build Variables

### Installation Directories
- `PREFIX` - Installation prefix (default: `/usr/local`)
- `DESTDIR` - Staging directory for package building (default: empty)
- `BINDIR` - Binary installation directory (default: `$(PREFIX)/bin`)
- `LIBDIR` - Library installation directory (default: `$(PREFIX)/lib`)
- `INCLUDEDIR` - Header installation directory (default: `$(PREFIX)/include`)
- `MANDIR` - Manual page directory (default: `$(PREFIX)/share/man`)

### Build Tools and Flags
- `CC` - C compiler (default: `gcc`)
- `CXX` - C++ compiler (default: `g++`)
- `CFLAGS` - C compiler flags (default: `-g -O2`)
- `CXXFLAGS` - C++ compiler flags (default: `-g -O2`)
- `LDFLAGS` - Linker flags (default: empty)

## Build Methods

### Quick Build (Recommended)
```bash
./build.sh
sudo make install
```

### Manual Build
```bash
make clean
make
sudo make install
```

### Custom Configuration
```bash
# Custom compiler and optimization
make CC=clang CFLAGS="-O3 -march=native"

# Custom installation prefix
make install PREFIX=/opt/hfsutils

# Package building with staging directory
make install DESTDIR=/tmp/staging PREFIX=/usr

# Environment-based configuration
export CC=gcc-11
export CFLAGS="-O2 -g -fstack-protector-strong"
export PREFIX=/usr/local
./build.sh
```

## Installation Layout

When installed, hfsutils creates the following directory structure:

```
$PREFIX/
├── bin/
│   ├── hfsutil                 # Main unified binary
│   ├── hattrib -> hfsutil      # Traditional command symlinks (optional)
│   ├── hcd -> hfsutil
│   ├── hcopy -> hfsutil
│   └── ...
├── sbin/
│   └── hfsck                   # Filesystem checker
├── lib/
│   ├── libhfs.a               # HFS library
│   └── librsrc.a              # Resource library
├── include/
│   ├── hfs.h                  # HFS library header
│   └── rsrc.h                 # Resource library header
└── share/man/man1/
    ├── hfsutils.1             # Manual pages
    ├── hattrib.1
    ├── hcd.1
    └── ...
```

## Package Building

For distribution packaging, use `DESTDIR` to stage files:

```bash
# Build and stage installation
make install DESTDIR="$PWD/debian/tmp" PREFIX=/usr

# Files will be installed to:
# ./debian/tmp/usr/bin/hfsutil
# ./debian/tmp/usr/lib/libhfs.a
# ./debian/tmp/usr/share/man/man1/hfsutils.1
# etc.
```

## Cross-Compilation

The build system supports cross-compilation by setting appropriate variables:

```bash
# ARM64 cross-compilation example
make CC=aarch64-linux-gnu-gcc \
     CFLAGS="-O2 -g" \
     LDFLAGS="-static"
```

## Build Targets

- `make` or `make all` - Build all components
- `make hfsutil` - Build main utility only
- `make libhfs` - Build HFS library only
- `make librsrc` - Build resource library only
- `make hfsck` - Build filesystem checker only
- `make symlinks` - Create traditional command symlinks
- `make install` - Install all components
- `make install-libs` - Install libraries and headers only
- `make install-symlinks` - Install with traditional command names
- `make clean` - Remove built files
- `make distclean` - Remove all generated files
- `make test` - Run test suite
- `make help` - Show help information

## Troubleshooting

### Build Issues
```bash
# Clean and rebuild
make distclean
./build.sh

# Force reconfiguration of libraries
rm -f libhfs/config.status librsrc/config.status hfsck/config.status
./build.sh
```

### Installation Issues
```bash
# Check permissions
ls -la $(PREFIX)

# Install to custom location
make install PREFIX=$HOME/local

# Use DESTDIR for staging
make install DESTDIR=/tmp/test-install
```

### Compiler Issues
```bash
# Use different compiler
make CC=clang

# Add debug information
make CFLAGS="-g -O0 -DDEBUG"

# Static linking
make LDFLAGS="-static"
```

## Integration with Package Managers

### Debian/Ubuntu
```bash
# In debian/rules
override_dh_auto_build:
	./build.sh

override_dh_auto_install:
	$(MAKE) install DESTDIR=$(CURDIR)/debian/tmp PREFIX=/usr
```

### RPM-based Systems
```bash
# In .spec file
%build
./build.sh

%install
make install DESTDIR=%{buildroot} PREFIX=/usr
```

### Homebrew
```bash
# In formula
def install
  system "./build.sh"
  system "make", "install", "PREFIX=#{prefix}"
end
```