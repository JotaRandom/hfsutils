#!/bin/bash

# HFSUtils - Simple Build Script
# This script builds hfsutils without needing autoconf/configure

# Pass through environment variables
export CC="${CC:-gcc}"
export CXX="${CXX:-g++}"
export CFLAGS="${CFLAGS:--g -O2}"
export CXXFLAGS="${CXXFLAGS:--g -O2}"
export LDFLAGS="${LDFLAGS:-}"
export PREFIX="${PREFIX:-/usr/local}"
export DESTDIR="${DESTDIR:-}"

echo "Building HFSUtils..."
echo "CC=$CC"
echo "CFLAGS=$CFLAGS"
echo "PREFIX=$PREFIX"
if [ -n "$DESTDIR" ]; then
    echo "DESTDIR=$DESTDIR"
fi

# Create build directory if it doesn't exist
mkdir -p build/obj

# Create .stamp if it doesn't exit too
mkdir -p libhfs/.stamp
mkdir -p librsrc/.stamp
mkdir -p hfsck/.stamp

# Build libhfs
echo "Building libhfs..."
cd libhfs
if [ ! -f configure ]; then
    echo "Error: libhfs/configure not found!"
    exit 1
fi
if [ ! -f config.status ]; then
    echo "Configuring libhfs..."
    CC="$CC" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" ./configure --prefix="$PREFIX"
fi
make CC="$CC" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" || { echo "Failed to build libhfs"; exit 1; }
cd ..

# Build librsrc
echo "Building librsrc..."
cd librsrc
if [ ! -f configure ]; then
    echo "Error: librsrc/configure not found!"
    exit 1
fi
if [ ! -f config.status ]; then
    echo "Configuring librsrc..."
    CC="$CC" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" ./configure --prefix="$PREFIX"
fi
make CC="$CC" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" || { echo "Failed to build librsrc"; exit 1; }
cd ..

# Build hfsck
echo "Building hfsck..."
cd hfsck
if [ ! -f configure ]; then
    echo "Error: hfsck/configure not found!"
    exit 1
fi
if [ ! -f config.status ]; then
    echo "Configuring hfsck..."
    CC="$CC" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" ./configure --prefix="$PREFIX"
fi
cd ..


# Build main utilities
echo "Building hfsutil..."
make CC="$CC" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" PREFIX="$PREFIX" || { echo "Failed to build hfsutil"; exit 1; }

if [ -f hfsutil ]; then
    echo "Build complete! The hfsutil binary is ready."
    echo ""
    echo "To install system-wide, run:"
    echo "  sudo make install"
    echo ""
    echo "To install to a custom location, run:"
    echo "  make install PREFIX=/custom/path"
    echo "  make install DESTDIR=/staging/area"
    echo ""
    echo "To create traditional command symlinks, run:"
    echo "  sudo make install-symlinks"
else
    echo "Build failed - hfsutil binary not created"
    exit 1
fi