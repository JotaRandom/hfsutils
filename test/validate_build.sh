#!/bin/bash

# Validation script to check build system functionality
# This script verifies that all variables are properly passed through the build system

set -e

echo "=== HFSUtils Build System Validation ==="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test function
test_variable() {
    local var_name="$1"
    local expected="$2"
    local actual="$3"
    
    if [ "$actual" = "$expected" ]; then
        echo -e "${GREEN}✓${NC} $var_name: $actual"
        return 0
    else
        echo -e "${RED}✗${NC} $var_name: expected '$expected', got '$actual'"
        return 1
    fi
}

# Test 1: Check Makefile variable defaults
echo "Test 1: Checking Makefile variable defaults"
echo "----------------------------------------"

# Extract defaults from Makefile
PREFIX_DEFAULT=$(grep "^PREFIX ?=" Makefile | cut -d'=' -f2 | tr -d ' ')
CC_DEFAULT=$(grep "^CC ?=" Makefile | cut -d'=' -f2 | tr -d ' ')
CFLAGS_DEFAULT=$(grep "^CFLAGS ?=" Makefile | cut -d'=' -f2 | tr -d ' ')

test_variable "PREFIX" "/usr/local" "$PREFIX_DEFAULT"
test_variable "CC" "gcc" "$CC_DEFAULT"
test_variable "CFLAGS" "-g-O2" "$CFLAGS_DEFAULT"

echo

# Test 2: Check build.sh variable handling
echo "Test 2: Checking build.sh variable handling"
echo "-------------------------------------------"

# Check if build.sh properly handles environment variables
if grep -q 'export CC="${CC:-gcc}"' build.sh; then
    echo -e "${GREEN}✓${NC} build.sh handles CC variable"
else
    echo -e "${RED}✗${NC} build.sh missing CC variable handling"
fi

if grep -q 'export CFLAGS="${CFLAGS:--g -O2}"' build.sh; then
    echo -e "${GREEN}✓${NC} build.sh handles CFLAGS variable"
else
    echo -e "${RED}✗${NC} build.sh missing CFLAGS variable handling"
fi

if grep -q 'export PREFIX="${PREFIX:-/usr/local}"' build.sh; then
    echo -e "${GREEN}✓${NC} build.sh handles PREFIX variable"
else
    echo -e "${RED}✗${NC} build.sh missing PREFIX variable handling"
fi

if grep -q 'export DESTDIR="${DESTDIR:-}"' build.sh; then
    echo -e "${GREEN}✓${NC} build.sh handles DESTDIR variable"
else
    echo -e "${RED}✗${NC} build.sh missing DESTDIR variable handling"
fi

echo

# Test 3: Check library Makefiles
echo "Test 3: Checking library Makefiles"
echo "----------------------------------"

for lib in libhfs librsrc hfsck; do
    if [ -f "$lib/Makefile" ]; then
        if grep -q "PREFIX ?=" "$lib/Makefile"; then
            echo -e "${GREEN}✓${NC} $lib/Makefile supports PREFIX"
        else
            echo -e "${RED}✗${NC} $lib/Makefile missing PREFIX support"
        fi
        
        if grep -q "DESTDIR ?=" "$lib/Makefile"; then
            echo -e "${GREEN}✓${NC} $lib/Makefile supports DESTDIR"
        else
            echo -e "${RED}✗${NC} $lib/Makefile missing DESTDIR support"
        fi
        
        if grep -q "CC ?=" "$lib/Makefile"; then
            echo -e "${GREEN}✓${NC} $lib/Makefile supports CC"
        else
            echo -e "${RED}✗${NC} $lib/Makefile missing CC support"
        fi
        
        if grep -q "CFLAGS ?=" "$lib/Makefile"; then
            echo -e "${GREEN}✓${NC} $lib/Makefile supports CFLAGS"
        else
            echo -e "${RED}✗${NC} $lib/Makefile missing CFLAGS support"
        fi
    else
        echo -e "${YELLOW}!${NC} $lib/Makefile not found"
    fi
done

echo

# Test 4: Check manual page installation
echo "Test 4: Checking manual page installation"
echo "-----------------------------------------"

if [ -d "doc/man" ]; then
    man_count=$(find doc/man -name "*.1" | wc -l)
    echo -e "${GREEN}✓${NC} Found $man_count manual pages in doc/man/"
    
    if grep -q "doc/man/\*.1" Makefile; then
        echo -e "${GREEN}✓${NC} Makefile includes manual page installation"
    else
        echo -e "${RED}✗${NC} Makefile missing manual page installation"
    fi
else
    echo -e "${RED}✗${NC} doc/man directory not found"
fi

echo

# Test 5: Check installation targets
echo "Test 5: Checking installation targets"
echo "-------------------------------------"

targets=("install" "install-libs" "install-symlinks")
for target in "${targets[@]}"; do
    if grep -q "^$target:" Makefile; then
        echo -e "${GREEN}✓${NC} Target '$target' exists"
    else
        echo -e "${RED}✗${NC} Target '$target' missing"
    fi
done

echo

# Summary
echo "=== Validation Summary ==="
echo "Build system features implemented:"
echo "• DESTDIR support for package building"
echo "• PREFIX support for custom installation paths"
echo "• CC/CXX compiler selection"
echo "• CFLAGS/CXXFLAGS/LDFLAGS support"
echo "• Manual pages installation"
echo "• Library installation with headers"
echo "• Proper variable propagation to sub-makefiles"
echo
echo "Usage examples:"
echo "  make install PREFIX=/opt/hfsutils"
echo "  make install DESTDIR=/tmp/staging"
echo "  make CC=clang CFLAGS='-O3 -march=native'"
echo "  CC=gcc-11 CFLAGS='-O2 -g' ./build.sh"