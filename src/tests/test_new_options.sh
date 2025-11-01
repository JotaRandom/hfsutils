#!/bin/bash
# Test script for new command-line options in mkfs.hfs

set -e

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

log_info() {
    echo -e "${BLUE}INFO:${NC} $1"
}

log_success() {
    echo -e "${GREEN}SUCCESS:${NC} $1"
}

log_error() {
    echo -e "${RED}ERROR:${NC} $1"
}

MKFS_HFS="./build/standalone/mkfs.hfs"

echo "=== Testing New Command-Line Options ==="
echo

# Test 1: --license option
log_info "Testing --license option"
if $MKFS_HFS --license | grep -q "Copyright"; then
    log_success "✓ --license option works"
else
    log_error "✗ --license option failed"
    exit 1
fi

echo

# Test 2: -t/--type option
log_info "Testing -t/--type option"

# Test explicit HFS type
rm -f /tmp/test_type_hfs.img
if $MKFS_HFS -t hfs -v /tmp/test_type_hfs.img | grep -q "HFS volume"; then
    log_success "✓ -t hfs works"
else
    log_error "✗ -t hfs failed"
    exit 1
fi

# Test explicit HFS+ type
rm -f /tmp/test_type_hfsplus.img
if $MKFS_HFS -t hfs+ -v /tmp/test_type_hfsplus.img | grep -q "HFS+ volume"; then
    log_success "✓ -t hfs+ works"
else
    log_error "✗ -t hfs+ failed"
    exit 1
fi

# Test hfsplus alias
rm -f /tmp/test_type_hfsplus2.img
if $MKFS_HFS -t hfsplus -v /tmp/test_type_hfsplus2.img | grep -q "HFS+ volume"; then
    log_success "✓ -t hfsplus works"
else
    log_error "✗ -t hfsplus failed"
    exit 1
fi

# Test invalid type
if $MKFS_HFS -t invalid /tmp/test_invalid_type.img 2>&1 | grep -q "invalid filesystem type"; then
    log_success "✓ Invalid type correctly rejected"
else
    log_error "✗ Invalid type not rejected"
    exit 1
fi

echo

# Test 3: -s/--size option
log_info "Testing -s/--size option"

# Test size in bytes
rm -f /tmp/test_size_bytes.img
if $MKFS_HFS -s 1048576 -v /tmp/test_size_bytes.img | grep -q "created successfully"; then
    log_success "✓ Size in bytes works"
else
    log_error "✗ Size in bytes failed"
    exit 1
fi

# Test size with K suffix
rm -f /tmp/test_size_k.img
if $MKFS_HFS -s 1024K -v /tmp/test_size_k.img | grep -q "created successfully"; then
    log_success "✓ Size with K suffix works"
else
    log_error "✗ Size with K suffix failed"
    exit 1
fi

# Test size with M suffix
rm -f /tmp/test_size_m.img
if $MKFS_HFS -s 10M -v /tmp/test_size_m.img | grep -q "created successfully"; then
    log_success "✓ Size with M suffix works"
else
    log_error "✗ Size with M suffix failed"
    exit 1
fi

# Test size with G suffix
rm -f /tmp/test_size_g.img
if $MKFS_HFS -s 1G -v /tmp/test_size_g.img | grep -q "created successfully"; then
    log_success "✓ Size with G suffix works"
else
    log_error "✗ Size with G suffix failed"
    exit 1
fi

# Test minimum size validation
if $MKFS_HFS -s 100K /tmp/test_small.img 2>&1 | grep -q "must be at least 800KB"; then
    log_success "✓ Minimum size validation works"
else
    log_error "✗ Minimum size validation failed"
    exit 1
fi

# Test invalid size
if $MKFS_HFS -s invalid /tmp/test_invalid_size.img 2>&1 | grep -q "invalid size"; then
    log_success "✓ Invalid size correctly rejected"
else
    log_error "✗ Invalid size not rejected"
    exit 1
fi

echo

# Test 4: Combined options
log_info "Testing combined options"

rm -f /tmp/test_combined.img
if $MKFS_HFS -t hfs+ -s 20M -l "Combined Test" -v /tmp/test_combined.img | grep -q "HFS+ volume 'Combined Test' created successfully"; then
    log_success "✓ Combined options work"
else
    log_error "✗ Combined options failed"
    exit 1
fi

echo

# Test 5: Program name detection with explicit type override
log_info "Testing program name detection with type override"

# Test mkfs.hfs+ with explicit HFS type (should use explicit type)
rm -f /tmp/test_override.img
if ./build/standalone/mkfs.hfs+ -t hfs -v /tmp/test_override.img | grep -q "HFS volume"; then
    log_success "✓ Explicit type overrides program name"
else
    log_error "✗ Explicit type override failed"
    exit 1
fi

echo

# Test 6: File size preservation with new options
log_info "Testing file size preservation with new options"

# Create a 5MB file and format it
rm -f /tmp/test_preserve_size.img
dd if=/dev/zero of=/tmp/test_preserve_size.img bs=1024 count=5120 2>/dev/null

size_before=$(stat -c%s /tmp/test_preserve_size.img 2>/dev/null || stat -f%z /tmp/test_preserve_size.img 2>/dev/null)
log_info "File size before formatting: $size_before bytes"

$MKFS_HFS -f -t hfs+ -l "Size Test" -v /tmp/test_preserve_size.img >/dev/null

size_after=$(stat -c%s /tmp/test_preserve_size.img 2>/dev/null || stat -f%z /tmp/test_preserve_size.img 2>/dev/null)
log_info "File size after formatting: $size_after bytes"

if [[ "$size_before" -eq "$size_after" ]]; then
    log_success "✓ File size preserved with new options"
else
    log_error "✗ File size changed! Before: $size_before, After: $size_after"
    exit 1
fi

echo

# Cleanup
log_info "Cleaning up test files"
rm -f /tmp/test_*.img

echo "=== All New Option Tests Passed! ==="
echo
echo "Summary of new features tested:"
echo "✓ --license option displays license information"
echo "✓ -t/--type option supports hfs, hfs+, hfsplus"
echo "✓ -s/--size option supports bytes and K/M/G suffixes"
echo "✓ Size validation enforces 800KB minimum"
echo "✓ Invalid options are properly rejected"
echo "✓ Combined options work together"
echo "✓ Explicit type overrides program name detection"
echo "✓ File sizes are preserved during formatting"
echo
log_success "mkfs.hfs command-line interface is fully compatible!"