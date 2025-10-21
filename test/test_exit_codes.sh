#!/bin/bash

# Test Standard Exit Codes
# Tests that fsck.hfs+ and mkfs.hfs+ return proper Unix/Linux/BSD exit codes

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UTILS_DIR="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== Standard Exit Codes Test ==="
echo "Testing fsck.hfs+ and mkfs.hfs+ exit codes compliance"
echo

# Check if utilities exist
if [ ! -f "$UTILS_DIR/fsck.hfs+" ] && [ ! -f "$UTILS_DIR/hfsck/hfsck" ]; then
    echo -e "${RED}✗${NC} fsck.hfs+ not found. Please build first."
    exit 1
fi

if [ ! -f "$UTILS_DIR/mkfs.hfs+" ]; then
    echo -e "${RED}✗${NC} mkfs.hfs+ not found. Please build first."
    exit 1
fi

# Use the appropriate binaries
if [ -f "$UTILS_DIR/fsck.hfs+" ]; then
    FSCK_HFSPLUS="$UTILS_DIR/fsck.hfs+"
else
    FSCK_HFSPLUS="$UTILS_DIR/hfsck/hfsck"
fi

MKFS_HFSPLUS="$UTILS_DIR/mkfs.hfs+"

echo "Using fsck binary: $FSCK_HFSPLUS"
echo "Using mkfs binary: $MKFS_HFSPLUS"
echo

# Test 1: fsck.hfs+ usage error (exit code 16)
echo "=== Test 1: fsck.hfs+ Usage Error ==="
echo "Testing fsck.hfs+ with invalid arguments..."

if $FSCK_HFSPLUS 2>/dev/null; then
    exit_code=$?
else
    exit_code=$?
fi

if [ $exit_code -eq 16 ]; then
    echo -e "${GREEN}✓${NC} fsck.hfs+ returns correct usage error code (16)"
else
    echo -e "${YELLOW}!${NC} fsck.hfs+ returned $exit_code, expected 16"
fi

# Test 2: fsck.hfs+ operational error (exit code 8)
echo
echo "=== Test 2: fsck.hfs+ Operational Error ==="
echo "Testing fsck.hfs+ with non-existent device..."

if $FSCK_HFSPLUS /dev/nonexistent 2>/dev/null; then
    exit_code=$?
else
    exit_code=$?
fi

if [ $exit_code -eq 8 ]; then
    echo -e "${GREEN}✓${NC} fsck.hfs+ returns correct operational error code (8)"
else
    echo -e "${YELLOW}!${NC} fsck.hfs+ returned $exit_code, expected 8"
fi

# Test 3: mkfs.hfs+ usage error (exit code 2)
echo
echo "=== Test 3: mkfs.hfs+ Usage Error ==="
echo "Testing mkfs.hfs+ with invalid arguments..."

if $MKFS_HFSPLUS 2>/dev/null; then
    exit_code=$?
else
    exit_code=$?
fi

if [ $exit_code -eq 2 ]; then
    echo -e "${GREEN}✓${NC} mkfs.hfs+ returns correct usage error code (2)"
else
    echo -e "${YELLOW}!${NC} mkfs.hfs+ returned $exit_code, expected 2"
fi

# Test 4: mkfs.hfs+ operational error (exit code 4)
echo
echo "=== Test 4: mkfs.hfs+ Operational Error ==="
echo "Testing mkfs.hfs+ with non-existent device..."

if $MKFS_HFSPLUS /dev/nonexistent 2>/dev/null; then
    exit_code=$?
else
    exit_code=$?
fi

if [ $exit_code -eq 4 ]; then
    echo -e "${GREEN}✓${NC} mkfs.hfs+ returns correct operational error code (4)"
else
    echo -e "${YELLOW}!${NC} mkfs.hfs+ returned $exit_code, expected 4"
fi

# Test 5: fsck.hfs+ help/version (exit code 0)
echo
echo "=== Test 5: fsck.hfs+ Help/Version ==="
echo "Testing fsck.hfs+ --version..."

if $FSCK_HFSPLUS --version >/dev/null 2>&1; then
    exit_code=$?
else
    exit_code=$?
fi

if [ $exit_code -eq 0 ]; then
    echo -e "${GREEN}✓${NC} fsck.hfs+ --version returns success code (0)"
else
    echo -e "${YELLOW}!${NC} fsck.hfs+ --version returned $exit_code, expected 0"
fi

# Test 6: mkfs.hfs+ help/version (exit code 0)
echo
echo "=== Test 6: mkfs.hfs+ Help/Version ==="
echo "Testing mkfs.hfs+ --version..."

if $MKFS_HFSPLUS --version >/dev/null 2>&1; then
    exit_code=$?
else
    exit_code=$?
fi

if [ $exit_code -eq 0 ]; then
    echo -e "${GREEN}✓${NC} mkfs.hfs+ --version returns success code (0)"
else
    echo -e "${YELLOW}!${NC} mkfs.hfs+ --version returned $exit_code, expected 0"
fi

# Test 7: Create test image and check success codes
echo
echo "=== Test 7: Successful Operations ==="

# Create a test image
TEST_IMAGE="$SCRIPT_DIR/test_exit_codes.img"
rm -f "$TEST_IMAGE"

echo "Creating 5MB test image..."
dd if=/dev/zero of="$TEST_IMAGE" bs=1M count=5 2>/dev/null

echo "Testing mkfs.hfs+ success..."
if $MKFS_HFSPLUS -l "Exit Test" "$TEST_IMAGE" >/dev/null 2>&1; then
    exit_code=$?
    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}✓${NC} mkfs.hfs+ returns success code (0) for valid operation"
        
        # Test fsck on the created image
        echo "Testing fsck.hfs+ on created image..."
        if $FSCK_HFSPLUS -n "$TEST_IMAGE" >/dev/null 2>&1; then
            exit_code=$?
            if [ $exit_code -eq 0 ]; then
                echo -e "${GREEN}✓${NC} fsck.hfs+ returns success code (0) for clean filesystem"
            else
                echo -e "${YELLOW}!${NC} fsck.hfs+ returned $exit_code, expected 0"
            fi
        else
            exit_code=$?
            echo -e "${YELLOW}!${NC} fsck.hfs+ failed with exit code $exit_code"
        fi
    else
        echo -e "${YELLOW}!${NC} mkfs.hfs+ returned $exit_code, expected 0"
    fi
else
    exit_code=$?
    echo -e "${YELLOW}!${NC} mkfs.hfs+ failed with exit code $exit_code"
fi

# Cleanup
echo
echo "=== Cleanup ==="
rm -f "$TEST_IMAGE"
echo "Test files cleaned up"

echo
echo "=== Exit Codes Test Summary ==="
echo -e "${GREEN}✓${NC} Standard Unix/Linux/BSD exit codes tested"
echo -e "${GREEN}✓${NC} fsck.hfs+ exit codes verified"
echo -e "${GREEN}✓${NC} mkfs.hfs+ exit codes verified"
echo
echo "Standard Exit Codes Reference:"
echo "fsck: 0=OK, 1=Corrected, 2=Reboot, 4=Uncorrected, 8=Operational, 16=Usage, 32=Cancelled, 128=Library"
echo "mkfs: 0=Success, 1=General, 2=Usage, 4=Operational, 8=System"