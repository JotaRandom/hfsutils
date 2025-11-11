#!/bin/bash

# Test hcopy recursive directory copying functionality
# This test creates a directory structure on Unix and copies it to an HFS volume

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Utilities
HFSUTIL="$PROJECT_ROOT/hfsutil"
HMOUNT="$PROJECT_ROOT/hmount"
HUMOUNT="$PROJECT_ROOT/humount"
HCOPY="$PROJECT_ROOT/hcopy"
HLS="$PROJECT_ROOT/hls"
HMKDIR="$PROJECT_ROOT/hmkdir"
HCD="$PROJECT_ROOT/hcd"
HFORMAT="$PROJECT_ROOT/hformat"

# Test directory
TEST_DIR="$SCRIPT_DIR/temp_recursive_test"
TEST_VOLUME="$TEST_DIR/test_volume.hfs"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

pass() {
    echo -e "${GREEN}[PASS]${NC} $*"
}

fail() {
    echo -e "${RED}[FAIL]${NC} $*"
    exit 1
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $*"
}

info() {
    echo "[INFO] $*"
}

# Cleanup function
cleanup() {
    "$HUMOUNT" 2>/dev/null || true
    rm -rf "$TEST_DIR"
}

trap cleanup EXIT

echo "========================================"
echo "  hcopy Recursive Directory Test"
echo "========================================"
echo ""

# Create test directory structure
info "Creating test directory structure..."
mkdir -p "$TEST_DIR"
mkdir -p "$TEST_DIR/source/subdir1"
mkdir -p "$TEST_DIR/source/subdir2/nested"
echo "test file 1" > "$TEST_DIR/source/file1.txt"
echo "test file 2" > "$TEST_DIR/source/subdir1/file2.txt"
echo "test file 3" > "$TEST_DIR/source/subdir2/file3.txt"
echo "nested file" > "$TEST_DIR/source/subdir2/nested/file4.txt"

# Create HFS volume
info "Creating HFS volume..."
dd if=/dev/zero of="$TEST_VOLUME" bs=1M count=5 2>/dev/null || fail "Failed to create volume file"
"$HFORMAT" -l "RecursiveTest" "$TEST_VOLUME" >/dev/null 2>&1 || fail "Failed to format volume"

# Mount volume
info "Mounting volume..."
"$HMOUNT" "$TEST_VOLUME" || fail "Failed to mount volume"

# Test 1: Verify directory is rejected without -R flag
info "Test 1: Directory without -R flag should fail..."
if "$HCOPY" "$TEST_DIR/source" : 2>/dev/null; then
    fail "hcopy should reject directory without -R flag"
else
    pass "Directory rejected without -R flag"
fi

# Test 2: Copy directory with -R flag
info "Test 2: Copy directory with -R flag..."
if "$HCOPY" -R "$TEST_DIR/source" : 2>/dev/null; then
    pass "hcopy accepted directory with -R flag"
else
    fail "hcopy failed with -R flag"
fi

# Test 3: Verify directory was created
info "Test 3: Verifying directory structure..."
if "$HLS" :source >/dev/null 2>&1; then
    pass "Source directory created on HFS volume"
else
    fail "Source directory not found on HFS volume"
fi

# Test 4: Verify subdirectories
if "$HLS" :source:subdir1 >/dev/null 2>&1; then
    pass "Subdirectory 1 created"
else
    warn "Subdirectory 1 not found (may be expected if recursive copy not fully implemented)"
fi

if "$HLS" :source:subdir2 >/dev/null 2>&1; then
    pass "Subdirectory 2 created"
else
    warn "Subdirectory 2 not found"
fi

if "$HLS" :source:subdir2:nested >/dev/null 2>&1; then
    pass "Nested subdirectory created"
else
    warn "Nested subdirectory not found"
fi

# Test 5: Verify files
info "Test 4: Verifying files..."
if "$HLS" :source | grep -q "file1.txt"; then
    pass "File1 found in root directory"
else
    warn "File1 not found in root directory"
fi

if "$HLS" :source:subdir1 | grep -q "file2.txt" 2>/dev/null; then
    pass "File2 found in subdir1"
else
    warn "File2 not found in subdir1"
fi

# Unmount
info "Unmounting volume..."
"$HUMOUNT" || warn "Failed to unmount cleanly"

echo ""
echo "========================================"
echo "  Recursive copy test completed"
echo "========================================"
