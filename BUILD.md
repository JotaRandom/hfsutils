# Building Standalone HFS Utilities

This document describes how to build the standalone HFS utilities (mkfs.hfs, fsck.hfs, mount.hfs) independently from the main hfsutils suite.

## Quick Start

```bash
# Configure and build in one step
./build.standalone.sh

# Or step by step
./configure.standalone
make -f Makefile.standalone
make -f Makefile.standalone test
```

## Prerequisites

- C compiler (gcc or clang)
- Standard C library and headers
- make (optional but recommended)
- bash (for scripts)

### On Ubuntu/Debian:
```bash
sudo apt install build-essential
```

### On CentOS/RHEL:
```bash
sudo yum groupinstall "Development Tools"
```

## Configuration Options

The `configure.standalone` script supports various options:

```bash
./configure.standalone --help
```

### Common Configurations

**Default installation:**
```bash
./configure.standalone
```

**System installation:**
```bash
./configure.standalone --prefix=/usr --sbindir=/usr/bin
```

**Debug build:**
```bash
./configure.standalone --enable-debug
```

**Static linking:**
```bash
./configure.standalone --enable-static
```

**Custom compiler:**
```bash
./configure.standalone --cc=clang
```

## Build Targets

### Main Targets

- `make all` or `make simple` - Build simple standalone version (default)
- `make full` - Build full version with libhfs integration (requires more dependencies)
- `make test` - Run test suite
- `make clean` - Clean build artifacts
- `make install` - Install utilities to system

### Development Targets

- `make debug` - Build with debug symbols and no optimization
- `make release` - Build optimized release version
- `make check` - Check build environment
- `make info` - Show build configuration

### Utility Targets

- `make links` - Create symbolic links for different program names
- `make distclean` - Clean all generated files including configuration

## Installation

### Local Installation (recommended for testing)
```bash
./configure.standalone --prefix=$HOME/local
make -f Makefile.standalone install
```

### System Installation
```bash
./configure.standalone --prefix=/usr --sbindir=/usr/bin
make -f Makefile.standalone
sudo make -f Makefile.standalone install
```

### Package Installation
```bash
./configure.standalone --prefix=/usr --sbindir=/usr/bin
make -f Makefile.standalone
make -f Makefile.standalone install DESTDIR=/tmp/package-root
```

## Testing

### Run All Tests
```bash
make -f Makefile.standalone test
```

### Manual Testing
```bash
# Create a test volume
./build/standalone/mkfs.hfs -v -l "Test Volume" /tmp/test.img

# Verify the volume
hexdump -C /tmp/test.img | head -10
```

### Test Different Program Names
```bash
./build/standalone/mkfs.hfs --version
./build/standalone/mkfs.hfs+ --version
./build/standalone/mkfs.hfsplus --version
```

## Build Variants

### Simple Version (Recommended)

The simple version is a standalone implementation with minimal dependencies:

- No libhfs dependency
- Fast compilation
- Basic HFS formatting
- Suitable for most use cases

```bash
make -f Makefile.standalone simple
```

### Full Version (Advanced)

The full version integrates with the complete libhfs library:

- Full libhfs integration
- Advanced HFS features
- More dependencies required
- Complete compatibility with original hformat

```bash
make -f Makefile.standalone full
```

## Troubleshooting

### Common Issues

**Compiler not found:**
```bash
./configure.standalone --cc=/path/to/gcc
```

**Missing headers:**
```bash
# Install development packages
sudo apt install libc6-dev  # Ubuntu/Debian
sudo yum install glibc-devel # CentOS/RHEL
```

**Permission denied during install:**
```bash
sudo make -f Makefile.standalone install
```

**WSL line ending issues:**
```bash
sed -i 's/\r$//' configure.standalone build.standalone.sh
```

### Build Environment Check

```bash
make -f Makefile.standalone check
```

### Verbose Build

```bash
make -f Makefile.standalone V=1
```

## File Structure

```
├── configure.standalone      # Configuration script
├── build.standalone.sh      # Quick build script
├── Makefile.standalone      # Standalone build system
├── config.mk               # Generated configuration
├── src/mkfs/               # mkfs.hfs source code
│   ├── mkfs_hfs_simple.c   # Simple standalone version
│   ├── mkfs_hfs_main.c     # Full version main
│   ├── mkfs_hfs_format.c   # Full version formatting
│   └── test_mkfs.sh        # Test suite
├── src/embedded/shared/    # Common utilities
└── build/standalone/       # Build output directory
    ├── mkfs.hfs           # Main executable
    ├── mkfs.hfs+          # HFS+ variant
    └── mkfs.hfsplus       # HFS+ variant
```

## Integration with Main Build System

The standalone build system is designed to work alongside the main hfsutils build system:

- Uses separate build directories
- Independent configuration
- Can be built without affecting main build
- Shares source code where appropriate

## Performance

Build times on typical systems:

- Simple version: < 5 seconds
- Full version: < 30 seconds
- Test suite: < 10 seconds

## Contributing

When adding new features:

1. Update both simple and full versions if applicable
2. Add tests to `src/mkfs/test_mkfs.sh`
3. Update this documentation
4. Test on multiple platforms

## Platform Support

Tested on:
- ✅ Ubuntu 20.04+ (WSL and native)
- ✅ Debian 10+
- ✅ CentOS 7+
- ✅ macOS (with Xcode tools)
- ✅ Alpine Linux

Should work on any POSIX-compliant system with a C99 compiler.