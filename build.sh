#!/bin/bash

# HFSUtils - Simple Build Script
# This script builds hfsutils without needing autoconf/configure

echo "Building HFSUtils..."

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
    ./configure
fi
make || { echo "Failed to build libhfs"; exit 1; }
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
    ./configure
fi
make || { echo "Failed to build librsrc"; exit 1; }
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
    ./configure
fi
cd ..


# Build main utilities
echo "Building hfsutil..."
make || { echo "Failed to build hfsutil"; exit 1; }

if [ -f hfsutil ]; then
    echo "Build complete! The hfsutil binary is ready."
    echo ""
    echo "To install system-wide, run:"
    echo "  sudo make install"
    echo ""
    echo "To create traditional command symlinks, run:"
    echo "  sudo make install-symlinks"
else
    echo "Build failed - hfsutil binary not created"
    exit 1
fi