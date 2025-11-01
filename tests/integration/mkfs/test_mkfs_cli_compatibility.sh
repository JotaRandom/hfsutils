#!/bin/bash
#
# test_mkfs_cli_compatibility.sh - Test command-line interface compatibility with hformat
# Copyright (C) 2025 Pablo Lezaeta
#

set -e

# Test configuration
TEST_DIR="$(dirname "$0")"
ROOT_DIR="$TEST_DIR/../../.."
BUILD_DIR="$ROOT_DIR/build/standalone"
TEST_IMAGE_DIR="/tmp/hfs_cli_test_$$"
TEST_IMAGE_SIZE="10M"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Cleanup function
cleanup() {
    log_info "Cleaning up test files..."
    rm -rf "$TEST_IMAGE_DIR"
}

# Set up cleanup trap
trap cleanup EXIT

# Test proper mkfs behavior (no -s or -t options)
test_proper_mkfs_behavior() {
    log_info "Testing proper mkfs behavior..."
    
    local test_image="$TEST_IMAGE_DIR/mkfs_proper.img"
    
    # Create test image file
    dd if=/dev/zero of="$test_image" bs=1M count=10 2>/dev/null
    
    # Test -f (force) option
    log_info "Testing -f (force) option..."
    "$BUILD_DIR/mkfs.hfs" -f -l "ForceTest" "$test_image"
    
    # Test -l (label) option
    log_info "Testing -l (label) option..."
    dd if=/dev/zero of="$test_image" bs=1M count=10 2>/dev/null
    "$BUILD_DIR/mkfs.hfs" -l "TestLabel" "$test_image"
    
    # Test -v (verbose) option
    log_info "Testing -v (verbose) option..."
    dd if=/dev/zero of="$test_image" bs=1M count=10 2>/dev/null
    "$BUILD_DIR/mkfs.hfs" -v -l "VerboseTest" "$test_image" | grep -q "formatting"
    
    # Test that mkfs.hfs+ works with HFS+ only
    log_info "Testing mkfs.hfs+ creates HFS+ only..."
    dd if=/dev/zero of="$test_image" bs=1M count=10 2>/dev/null
    "$BUILD_DIR/mkfs.hfs+" -v -l "HFSPlusTest" "$test_image" 2>&1 | grep -q "HFS+ filesystem"
    
    log_info "Proper mkfs behavior tests passed"
}

# Test that inappropriate options are rejected
test_inappropriate_options_rejected() {
    log_info "Testing that inappropriate options are rejected..."
    
    # Test that -t option is rejected by mkfs.hfs
    log_info "Testing -t option rejection by mkfs.hfs..."
    if "$BUILD_DIR/mkfs.hfs" -t hfs /dev/null 2>/dev/null; then
        log_error "mkfs.hfs should reject -t option"
        return 1
    fi
    
    # Test that -s option is rejected by mkfs.hfs
    log_info "Testing -s option rejection by mkfs.hfs..."
    if "$BUILD_DIR/mkfs.hfs" -s 1024 /dev/null 2>/dev/null; then
        log_error "mkfs.hfs should reject -s option"
        return 1
    fi
    
    # Test that -t option is rejected by mkfs.hfs+
    log_info "Testing -t option rejection by mkfs.hfs+..."
    if "$BUILD_DIR/mkfs.hfs+" -t hfs+ /dev/null 2>/dev/null; then
        log_error "mkfs.hfs+ should reject -t option"
        return 1
    fi
    
    # Test that -s option is rejected by mkfs.hfs+
    log_info "Testing -s option rejection by mkfs.hfs+..."
    if "$BUILD_DIR/mkfs.hfs+" -s 1024 /dev/null 2>/dev/null; then
        log_error "mkfs.hfs+ should reject -s option"
        return 1
    fi
    
    log_info "Inappropriate options rejection tests passed"
}

# Test separate tool behavior
test_separate_tool_behavior() {
    log_info "Testing separate tool behavior..."
    
    local test_image="$TEST_IMAGE_DIR/separate_tools.img"
    
    # Test mkfs.hfs creates HFS only
    log_info "Testing mkfs.hfs creates HFS only..."
    dd if=/dev/zero of="$test_image" bs=1M count=10 2>/dev/null
    "$BUILD_DIR/mkfs.hfs" -v "$test_image" | grep -q "HFS filesystem" && \
    ! "$BUILD_DIR/mkfs.hfs" -v "$test_image" | grep -q "HFS+ filesystem"
    
    # Test mkfs.hfs+ creates HFS+ only
    log_info "Testing mkfs.hfs+ creates HFS+ only..."
    dd if=/dev/zero of="$test_image" bs=1M count=10 2>/dev/null
    "$BUILD_DIR/mkfs.hfs+" -v "$test_image" 2>&1 | grep -q "HFS+ filesystem"
    
    log_info "Separate tool behavior tests passed"
}

# Test error handling
test_error_handling() {
    log_info "Testing error handling..."
    
    # Test missing device argument
    if "$BUILD_DIR/mkfs.hfs" 2>/dev/null; then
        log_error "Should have failed with missing device argument"
        return 1
    fi
    
    # Test invalid volume name (too long for HFS)
    local long_name="123456789012345678901234567890"  # 30 chars, too long for HFS
    if "$BUILD_DIR/mkfs.hfs" -l "$long_name" /dev/null 2>/dev/null; then
        log_error "Should have failed with volume name too long for HFS"
        return 1
    fi
    
    # Test invalid volume name (too long for HFS+)
    local very_long_name=$(printf 'A%.0s' {1..260})  # 260 chars, too long for HFS+
    if "$BUILD_DIR/mkfs.hfs+" -l "$very_long_name" /dev/null 2>/dev/null; then
        log_error "Should have failed with volume name too long for HFS+"
        return 1
    fi
    
    log_info "Error handling tests passed"
}

# Test volume label validation
test_volume_label_validation() {
    log_info "Testing volume label validation..."
    
    local test_image="$TEST_IMAGE_DIR/label_test.img"
    
    # Create test image file
    dd if=/dev/zero of="$test_image" bs=1M count=10 2>/dev/null
    
    # Test valid HFS label (27 chars max)
    "$BUILD_DIR/mkfs.hfs" -l "ValidHFSLabel" "$test_image"
    
    # Test valid HFS+ label (255 chars max)
    dd if=/dev/zero of="$test_image" bs=1M count=10 2>/dev/null
    "$BUILD_DIR/mkfs.hfs+" -l "ValidHFSPlusLabel" "$test_image"
    
    log_info "Volume label validation tests passed"
}

# Test proper mkfs behavior with partitions
test_partition_handling() {
    log_info "Testing partition handling..."
    
    local test_image="$TEST_IMAGE_DIR/partition_test.img"
    
    # Test basic formatting without partition number
    dd if=/dev/zero of="$test_image" bs=1M count=10 2>/dev/null
    "$BUILD_DIR/mkfs.hfs" -f -l "BasicTest" "$test_image"
    
    # Test with partition number 0 (whole device)
    dd if=/dev/zero of="$test_image" bs=1M count=10 2>/dev/null
    "$BUILD_DIR/mkfs.hfs" -l "PartTest" "$test_image" 0
    
    log_info "Partition handling tests passed"
}

# Check if utilities exist
check_utilities() {
    log_info "Checking for required utilities..."
    
    if [ ! -f "$BUILD_DIR/mkfs.hfs" ]; then
        log_error "mkfs.hfs not found. Please build first with 'make mkfs.hfs'"
        exit 1
    fi
    
    if [ ! -f "$BUILD_DIR/mkfs.hfs+" ]; then
        log_error "mkfs.hfs+ not found. Please build first with 'make mkfs.hfs'"
        exit 1
    fi
    
    log_info "All utilities found"
}

# Main test execution
main() {
    log_info "Starting mkfs command-line interface compatibility tests..."
    
    # Create test directory
    mkdir -p "$TEST_IMAGE_DIR"
    
    # Run tests
    check_utilities
    test_proper_mkfs_behavior
    test_inappropriate_options_rejected
    test_separate_tool_behavior
    test_error_handling
    test_volume_label_validation
    test_partition_handling
    
    log_info "All mkfs CLI compatibility tests passed!"
}

# Run main function
main "$@"