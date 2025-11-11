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

# Build hfsck with journaling support
echo "Building hfsck..."
cd hfsck

# Try autotools first, but don't fail if it doesn't work
if [ -f configure ] && [ -f config.status ]; then
    echo "Trying autotools build..."
    if make CC="$CC" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" 2>/dev/null; then
        echo "hfsck built successfully with autotools"
        cd ..
        # Continue to main utilities
    else
        echo "Autotools build failed, falling back to manual compilation..."
    fi
else
    echo "Autotools not properly configured, using manual compilation..."
fi

# Manual compilation (either as fallback or primary method)
if [ ! -f hfsck ]; then
    echo "Building hfsck manually with journaling support..."
    
    # Compile all source files
    $CC $CFLAGS -I./../include -I./../include/common -I./../libhfs -I./../src/common -DHAVE_CONFIG_H -c *.c || { echo "Failed to compile hfsck sources"; exit 1; }
    
    # Compile common sources
    $CC $CFLAGS -I./../include -I./../include/common -I./../libhfs -I./../src/common -DHAVE_CONFIG_H -c ../src/common/suid.c -o suid.o || { echo "Failed to compile suid.c"; exit 1; }
    $CC $CFLAGS -I./../include -I./../include/common -I./../libhfs -I./../src/common -DHAVE_CONFIG_H -c ../src/common/version.c -o version.o || { echo "Failed to compile version.c"; exit 1; }
    $CC $CFLAGS -I./../include -I./../include/common -I./../libhfs -I./../src/common -DHAVE_CONFIG_H -c ../src/common/hfs_detect.c -o hfs_detect.o || { echo "Failed to compile hfs_detect.c"; exit 1; }
    
    # Link hfsck with journaling support
    $CC $CFLAGS -o hfsck ck_btree.o ck_mdb.o ck_volume.o hfsck.o main.o util.o journal.o suid.o version.o hfs_detect.o ./../libhfs/libhfs.a || { echo "Failed to link hfsck"; exit 1; }
    
    echo "hfsck built successfully with manual compilation"
fi

cd ..


# Build main utilities
echo "Building hfsutil..."
make CC="$CC" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" PREFIX="$PREFIX" || { echo "Failed to build hfsutil"; exit 1; }

# Build standalone mkfs utilities
echo "Building standalone mkfs.hfs and mkfs.hfs+ utilities..."
make -C src/mkfs all CC="$CC" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" || { echo "Failed to build mkfs utilities"; exit 1; }

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