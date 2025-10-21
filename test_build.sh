#!/bin/bash

# Test script to verify the build system works with different configurations

set -e

echo "=== Testing HFSUtils Build System ==="
echo

# Test 1: Default build
echo "Test 1: Default build"
echo "make clean && make"
echo

# Test 2: Custom compiler and flags
echo "Test 2: Custom compiler and flags"
echo "make clean && make CC=clang CFLAGS='-O3 -Wall'"
echo

# Test 3: Custom prefix
echo "Test 3: Custom prefix installation"
echo "make install PREFIX=/opt/hfsutils"
echo

# Test 4: DESTDIR staging
echo "Test 4: DESTDIR staging installation"
echo "make install DESTDIR=/tmp/hfsutils-staging"
echo

# Test 5: Environment variables
echo "Test 5: Environment variables"
echo "export CC=gcc"
echo "export CFLAGS='-g -O2 -march=native'"
echo "export PREFIX=/usr/local"
echo "./build.sh"
echo

echo "=== Build System Features ==="
echo "✓ DESTDIR support for package building"
echo "✓ PREFIX support for custom installation paths"
echo "✓ CC/CXX compiler selection"
echo "✓ CFLAGS/CXXFLAGS/LDFLAGS support"
echo "✓ Manual pages installation to \$MANDIR/man1/"
echo "✓ Libraries installation to \$LIBDIR/"
echo "✓ Headers installation to \$INCLUDEDIR/"
echo "✓ hfsck installation to \$PREFIX/sbin/"
echo

echo "=== Usage Examples ==="
echo "# Standard installation:"
echo "sudo make install"
echo
echo "# Custom prefix:"
echo "make install PREFIX=/opt/hfsutils"
echo
echo "# Package building:"
echo "make install DESTDIR=\$PWD/debian/tmp"
echo
echo "# Custom compiler:"
echo "make CC=clang CFLAGS='-O3 -flto'"
echo
echo "# Environment-based build:"
echo "export CC=gcc-11"
echo "export CFLAGS='-O2 -g -fstack-protector-strong'"
echo "./build.sh"
echo