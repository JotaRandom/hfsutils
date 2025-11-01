#!/bin/bash
#
# test_mkfs_basic.sh - Basic integration tests for mkfs.hfs and mkfs.hfs+
# Copyright (C) 2025 Pablo Lezaeta
#

set -e

# Test configuration
TEST_DIR="$(dirname "$0")"
ROOT_DIR="$TEST_DIR/../../.."
BUILD_DIR="$ROOT_DIR/build/standalone"
TEST_IMAGE_DIR="/tmp/hfs_test_$$"
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

# Test mkfs.hfs basic functionality
test_mkfs_hfs() {
    log_info "Testing mkfs.hfs basic functionality..."
    
    local test_image="$TEST_IMAGE_DIR/test_hfs.img"
    
    # Create test image file
    dd if=/dev/zero of="$test_image" bs=1M count=10 2>/dev/null
    
    # Test basic mkfs.hfs
    log_info "Creating HFS filesystem..."
    "$BUILD_DIR/mkfs.hfs" -v -l "TestHFS" "$test_image"
    
    # Verify the image was created and has reasonable size
    if [ ! -f "$test_image" ]; then
        log_error "Test image was not created"
        return 1
    fi
    
    local size=$(stat -c%s "$test_image")
    if [ "$size" -lt 1000000 ]; then
        log_error "Test image seems too small: $size bytes"
        return 1
    fi
    
    log_info "mkfs.hfs basic test passed"
}

# Test mkfs.hfs+ basic functionality
test_mkfs_hfsplus() {
    log_info "Testing mkfs.hfs+ basic functionality..."
    
    local test_image="$TEST_IMAGE_DIR/test_hfsplus.img"
    
    # Create test image file
    dd if=/dev/zero of="$test_image" bs=1M count=10 2>/dev/null
    
    # Test basic mkfs.hfs+
    log_info "Creating HFS+ filesystem..."
    "$BUILD_DIR/mkfs.hfs+" -v -l "TestHFSPlus" "$test_image"
    
    # Verify the image was created and has reasonable size
    if [ ! -f "$test_image" ]; then
        log_error "Test image was not created"
        return 1
    fi
    
    local size=$(stat -c%s "$test_image")
    if [ "$size" -lt 1000000 ]; then
        log_error "Test image seems too small: $size bytes"
        return 1
    fi
    
    log_info "mkfs.hfs+ basic test passed"
}

# Test command-line options
test_command_line_options() {
    log_info "Testing command-line options..."
    
    # Test version option
    "$BUILD_DIR/mkfs.hfs" -V > /dev/null
    "$BUILD_DIR/mkfs.hfs+" -V > /dev/null
    
    # Test help option
    "$BUILD_DIR/mkfs.hfs" -h > /dev/null
    "$BUILD_DIR/mkfs.hfs+" -h > /dev/null
    
    # Test license option
    "$BUILD_DIR/mkfs.hfs" --license > /dev/null
    "$BUILD_DIR/mkfs.hfs+" --license > /dev/null
    
    log_info "Command-line options test passed"
}

# Main test execution
main() {
    log_info "Starting mkfs integration tests..."
    
    # Create test directory
    mkdir -p "$TEST_IMAGE_DIR"
    
    # Run tests
    check_utilities
    test_command_line_options
    test_mkfs_hfs
    test_mkfs_hfsplus
    
    log_info "All mkfs integration tests passed!"
}

# Run main function
main "$@"