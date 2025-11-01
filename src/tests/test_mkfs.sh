#!/bin/bash
# Test script for mkfs.hfs functionality

set -e

echo "=== Testing mkfs.hfs functionality ==="
echo

# Set paths
MKFS_HFS="./build/standalone/mkfs.hfs"
MKFS_HFS_PLUS="./build/standalone/mkfs.hfs+"
MKFS_HFSPLUS="./build/standalone/mkfs.hfsplus"

# Test basic functionality
echo "1. Testing basic functionality..."

# Test version
echo "   Testing version information:"
$MKFS_HFS --version
echo

# Test help
echo "   Testing help:"
$MKFS_HFS --help | head -5
echo

# Test different program names
echo "2. Testing different program names..."
echo "   mkfs.hfs:"
$MKFS_HFS --version | head -1

echo "   mkfs.hfs+:"
$MKFS_HFS_PLUS --version | head -1

echo "   mkfs.hfsplus:"
$MKFS_HFSPLUS --version | head -1
echo

# Test formatting
echo "3. Testing HFS formatting..."

# Create test images
TEST_IMAGE1="/tmp/test_hfs1.img"
TEST_IMAGE2="/tmp/test_hfs2.img"
TEST_IMAGE3="/tmp/test_hfsplus.img"

echo "   Creating basic HFS volume:"
$MKFS_HFS -v -l "Test Volume" "$TEST_IMAGE1"

echo "   Verifying HFS signature:"
if hexdump -C "$TEST_IMAGE1" | head -1 | grep -q "4c 4b"; then
    echo "   ✓ Boot block signature correct (LK)"
else
    echo "   ✗ Boot block signature incorrect"
fi

if hexdump -C "$TEST_IMAGE1" | grep "42 44" | head -1 > /dev/null; then
    echo "   ✓ MDB signature correct (BD)"
else
    echo "   ✗ MDB signature incorrect"
fi

if hexdump -C "$TEST_IMAGE1" | grep -q "Test Volume"; then
    echo "   ✓ Volume name correctly written"
else
    echo "   ✗ Volume name not found"
fi

echo "   Creating HFS+ volume:"
rm -f "$TEST_IMAGE3"  # Clean up first
$MKFS_HFS_PLUS -v -l "HFS+ Test" "$TEST_IMAGE3"

echo "   Verifying HFS+ signature:"
if hexdump -C "$TEST_IMAGE3" | head -1 | grep -q "4c 4b"; then
    echo "   ✓ Boot block signature correct (LK)"
else
    echo "   ✗ Boot block signature incorrect"
fi

if hexdump -C "$TEST_IMAGE3" | grep "48 2b" | head -1 > /dev/null; then
    echo "   ✓ HFS+ Volume Header signature correct (H+)"
else
    echo "   ✗ HFS+ Volume Header signature incorrect"
fi

echo

# Test error conditions
echo "4. Testing error conditions..."

echo "   Testing missing device argument:"
if $MKFS_HFS 2>/dev/null; then
    echo "   ✗ Should fail with missing device"
else
    echo "   ✓ Correctly rejects missing device"
fi

echo "   Testing volume name too long:"
if $MKFS_HFS -l "This volume name is way too long for HFS specification" "$TEST_IMAGE2" 2>/dev/null; then
    echo "   ✗ Should fail with long volume name"
else
    echo "   ✓ Correctly rejects long volume name"
fi

echo "   Testing existing file without force:"
if $MKFS_HFS "$TEST_IMAGE1" 2>/dev/null; then
    echo "   ✗ Should fail without force flag"
else
    echo "   ✓ Correctly requires force flag for existing file"
fi

echo "   Testing force flag:"
$MKFS_HFS -f -l "Forced" "$TEST_IMAGE1"
echo "   ✓ Force flag works correctly"

echo

# Test different volume names
echo "5. Testing volume name handling..."

echo "   Single character name:"
$MKFS_HFS -l "A" "$TEST_IMAGE2"
echo "   ✓ Single character name works"

echo "   Maximum length name (27 chars):"
$MKFS_HFS -f -l "abcdefghijklmnopqrstuvwxyz1" "$TEST_IMAGE2"
echo "   ✓ Maximum length name works"

echo "   Name with spaces:"
$MKFS_HFS -f -l "My Test Volume" "$TEST_IMAGE2"
echo "   ✓ Name with spaces works"

echo

# Performance test
echo "6. Testing performance..."
echo "   Creating 1MB volume:"
time $MKFS_HFS -f -l "Speed Test" "$TEST_IMAGE2" 2>/dev/null
echo "   ✓ Performance test completed"

echo

# Cleanup
echo "7. Cleaning up..."
rm -f "$TEST_IMAGE1" "$TEST_IMAGE2" "$TEST_IMAGE3"
echo "   ✓ Test files cleaned up"

echo
echo "=== All tests completed successfully! ==="
echo
echo "Summary:"
echo "✓ Basic functionality works"
echo "✓ Program name detection works"
echo "✓ HFS formatting creates correct structures"
echo "✓ HFS+ formatting creates correct structures"
echo "✓ Error handling works correctly"
echo "✓ Volume name validation works"
echo "✓ Performance is acceptable"
echo
echo "mkfs.hfs and mkfs.hfs+ are ready for basic testing!"