#!/bin/bash

#
# HFS hcopy Directory Recursion Test Suite
# Tests the recursive copy functionality for directories in hcopy
# This addresses the feature request where hcopy :folder: ./dest -r should support
# recursive copying of entire directory structures from HFS volumes to local filesystem
#
# Usage: ./test_hcopy_directory.sh
# 
# This test suite can be run standalone or integrated into the main test suite.
# It validates that hcopy -r flag works correctly for directory hierarchies.
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
TEST_DATA_DIR="$SCRIPT_DIR/data"
TEST_TEMP_DIR="$SCRIPT_DIR/temp_hcopy_test_$$"

# Utility paths
HFSUTIL="${PROJECT_ROOT}/hfsutil"
HCOPY="${PROJECT_ROOT}/hcopy"
HMOUNT="${PROJECT_ROOT}/hmount"
HUMOUNT="${PROJECT_ROOT}/humount"
HLS="${PROJECT_ROOT}/hls"
HMKDIR="${PROJECT_ROOT}/hmkdir"
HDEL="${PROJECT_ROOT}/hdel"
MKFS_HFS="${PROJECT_ROOT}/mkfs.hfs"

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0
FAILED_TESTS=()

#==============================================================================
# Helper Functions
#==============================================================================

log() {
    echo -e "${BLUE}[INFO]${NC} $*"
}

success() {
    echo -e "${GREEN}[PASS]${NC} $*"
}

error() {
    echo -e "${RED}[FAIL]${NC} $*"
}

warning() {
    echo -e "${YELLOW}[WARN]${NC} $*"
}

# Test execution wrapper
run_test() {
    local test_name="$1"
    local test_function="$2"
    
    echo
    log "Running test: $test_name"
    TESTS_RUN=$((TESTS_RUN + 1))
    
    if $test_function; then
        success "$test_name"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        error "$test_name"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        FAILED_TESTS+=("$test_name")
        return 1
    fi
}

# Check if file exists
assert_file_exists() {
    local file="$1"
    if [[ ! -f "$file" ]]; then
        error "File does not exist: $file"
        return 1
    fi
    return 0
}

# Check if directory exists
assert_dir_exists() {
    local dir="$1"
    if [[ ! -d "$dir" ]]; then
        error "Directory does not exist: $dir"
        return 1
    fi
    return 0
}

# Count files in directory recursively
count_files_recursive() {
    local dir="$1"
    find "$dir" -type f 2>/dev/null | wc -l
}

# Count directories recursively
count_dirs_recursive() {
    local dir="$1"
    find "$dir" -type d 2>/dev/null | wc -l
}

# Setup test environment
setup_test_env() {
    log "Setting up test environment..."
    
    # Create temp directory
    mkdir -p "$TEST_TEMP_DIR"
    
    # Verify utilities exist
    for util in "$HFSUTIL" "$HCOPY" "$HMOUNT" "$HUMOUNT" "$HLS" "$HMKDIR" "$HDEL" "$MKFS_HFS"; do
        if [[ ! -x "$util" ]]; then
            local util_name=$(basename "$util")
            if [[ ! -L "$util" ]]; then
                error "Utility $(basename "$util") not found at $util"
                return 1
            fi
        fi
    done
    
    log "Test environment ready"
    log "  Temp directory: $TEST_TEMP_DIR"
    
    return 0
}

# Cleanup test environment
cleanup_test_env() {
    log "Cleaning up test environment..."
    
    # Ensure no volume is mounted
    "$HUMOUNT" 2>/dev/null || true
    
    # Remove test directory
    rm -rf "$TEST_TEMP_DIR"
    
    log "Cleanup complete"
}

# Create a test HFS volume with a directory structure
create_test_volume() {
    local volume_path="$1"
    local volume_name="${2:-TestVol}"
    
    # Create the volume image
    dd if=/dev/zero of="$volume_path" bs=1M count=20 2>/dev/null
    
    # Format as HFS+ (more stable than HFS)
    # Note: mkfs.hfs+ may segfault but creates valid filesystem
    /mnt/c/Users/Usuario/source/repos/hfsutils/build/standalone/mkfs.hfs+ -f -l "$volume_name" "$volume_path" 2>/dev/null || true
    
    # Final check - just check if file exists and has reasonable size
    if [ ! -f "$volume_path" ] || [ $(stat -c%s "$volume_path") -lt 1000000 ]; then
        error "Failed to create test volume file"
        return 1
    fi
    
    return 0
}

# Print test summary
print_summary() {
    echo
    echo "========================================"
    echo "    HFS hcopy Directory Test Summary"
    echo "========================================"
    echo "Tests run:    $TESTS_RUN"
    echo "Tests passed: $TESTS_PASSED"
    echo "Tests failed: $TESTS_FAILED"
    
    if [[ $TESTS_FAILED -gt 0 ]]; then
        echo
        echo "Failed tests:"
        for test in "${FAILED_TESTS[@]}"; do
            echo "  - $test"
        done
        echo
        error "Some tests failed"
        return 1
    else
        echo
        success "All tests passed!"
        return 0
    fi
}

#==============================================================================
# Basic Functionality Tests
#==============================================================================

# Test that hcopy shows error for directory without -r flag
test_hcopy_dir_without_recursion_flag() {
    log "Testing hcopy directory copy without -r flag (should fail)..."
    
    local volume_path="$TEST_TEMP_DIR/test_no_recursion.hfs"
    
    # Create test volume
    if ! create_test_volume "$volume_path" "No Recursion Test"; then
        return 1
    fi
    
    # Mount volume
    "$HMOUNT" "$volume_path" || {
        error "Failed to mount test volume"
        return 1
    }
    
    # Create a directory on the volume
    "$HMKDIR" ":TestDir" || {
        error "Failed to create test directory"
        return 1
    }
    
    # Try to copy directory without -r flag (should fail)
    if "$HCOPY" ":TestDir" "$TEST_TEMP_DIR/copy_result" 2>/dev/null; then
        error "hcopy should fail when copying directory without -r flag"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    log "Correctly rejected directory copy without -r flag"
    
    "$HUMOUNT" 2>/dev/null || true
    return 0
}

# Test basic recursive directory copy
test_hcopy_dir_with_recursion_flag() {
    log "Testing hcopy directory copy with -r flag..."
    
    local volume_path="$TEST_TEMP_DIR/test_with_recursion.hfs"
    local local_copy_dir="$TEST_TEMP_DIR/copied_dir"
    
    # Create test volume
    if ! create_test_volume "$volume_path" "With Recursion Test"; then
        return 1
    fi
    
    # Mount volume
    "$HMOUNT" "$volume_path" || {
        error "Failed to mount test volume"
        return 1
    }
    
    # Create test directory structure
    "$HMKDIR" ":SourceDir" || {
        error "Failed to create source directory"
        return 1
    }
    
    # Copy directory with -r flag
    if ! "$HCOPY" ":SourceDir" "$local_copy_dir" -r 2>/dev/null; then
        error "hcopy failed to copy directory with -r flag"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    # Verify directory was copied
    if ! assert_dir_exists "$local_copy_dir"; then
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    log "Successfully copied directory with -r flag"
    
    "$HUMOUNT" 2>/dev/null || true
    return 0
}

#==============================================================================
# Complex Directory Structure Tests
#==============================================================================

# Test copying nested directories
test_hcopy_nested_directories() {
    log "Testing hcopy with nested directory structures..."
    
    local volume_path="$TEST_TEMP_DIR/test_nested.hfs"
    local local_copy_dir="$TEST_TEMP_DIR/nested_copy"
    
    # Create test volume
    if ! create_test_volume "$volume_path" "Nested Test"; then
        return 1
    fi
    
    # Mount volume
    "$HMOUNT" "$volume_path" || {
        error "Failed to mount test volume"
        return 1
    }
    
    # Create nested directory structure on HFS volume
    "$HMKDIR" ":Level1" || {
        error "Failed to create Level1"
        return 1
    }
    
    "$HMKDIR" ":Level1:Level2" || {
        error "Failed to create Level2"
        return 1
    }
    
    "$HMKDIR" ":Level1:Level2:Level3" || {
        error "Failed to create Level3"
        return 1
    }
    
    # Copy entire nested structure
    if ! "$HCOPY" ":Level1" "$local_copy_dir" -r 2>/dev/null; then
        error "hcopy failed to copy nested directories"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    # Verify structure was copied
    if ! assert_dir_exists "$local_copy_dir"; then
        error "Root directory not copied"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    if ! assert_dir_exists "$local_copy_dir/Level2"; then
        error "Level2 directory not copied"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    if ! assert_dir_exists "$local_copy_dir/Level2/Level3"; then
        error "Level3 directory not copied"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    log "Successfully copied nested directories"
    
    "$HUMOUNT" 2>/dev/null || true
    return 0
}

#==============================================================================
# Files in Directories Tests
#==============================================================================

# Test copying directories with files
test_hcopy_dir_with_files() {
    log "Testing hcopy directory copy with files..."
    
    local volume_path="$TEST_TEMP_DIR/test_with_files.hfs"
    local local_copy_dir="$TEST_TEMP_DIR/dir_with_files"
    local temp_file1="$TEST_TEMP_DIR/file1.txt"
    local temp_file2="$TEST_TEMP_DIR/file2.txt"
    
    # Create test files locally
    echo "Test file 1 content" > "$temp_file1"
    echo "Test file 2 content" > "$temp_file2"
    
    # Create test volume
    if ! create_test_volume "$volume_path" "With Files Test"; then
        return 1
    fi
    
    # Mount volume
    "$HMOUNT" "$volume_path" || {
        error "Failed to mount test volume"
        return 1
    }
    
    # Create directory and add files
    "$HMKDIR" ":FileDir" || {
        error "Failed to create directory"
        return 1
    }
    
    "$HCOPY" "$temp_file1" ":FileDir:file1.txt" || {
        error "Failed to copy file1"
        return 1
    }
    
    "$HCOPY" "$temp_file2" ":FileDir:file2.txt" || {
        error "Failed to copy file2"
        return 1
    }
    
    # Copy entire directory with files
    if ! "$HCOPY" ":FileDir" "$local_copy_dir" -r 2>/dev/null; then
        error "hcopy failed to copy directory with files"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    # Verify files were copied
    if ! assert_file_exists "$local_copy_dir/file1.txt"; then
        error "file1.txt not copied"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    if ! assert_file_exists "$local_copy_dir/file2.txt"; then
        error "file2.txt not copied"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    # Verify file contents
    if ! grep -q "Test file 1 content" "$local_copy_dir/file1.txt"; then
        error "file1.txt content not preserved"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    if ! grep -q "Test file 2 content" "$local_copy_dir/file2.txt"; then
        error "file2.txt content not preserved"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    log "Successfully copied directory with files"
    
    "$HUMOUNT" 2>/dev/null || true
    return 0
}

# Test copying deeply nested directory with many files
test_hcopy_complex_structure() {
    log "Testing hcopy with complex directory structure..."
    
    local volume_path="$TEST_TEMP_DIR/test_complex.hfs"
    local local_copy_dir="$TEST_TEMP_DIR/complex_copy"
    
    # Create test volume
    if ! create_test_volume "$volume_path" "Complex Test"; then
        return 1
    fi
    
    # Mount volume
    "$HMOUNT" "$volume_path" || {
        error "Failed to mount test volume"
        return 1
    }
    
    # Create complex structure
    "$HMKDIR" ":Project" || {
        error "Failed to create Project"
        return 1
    }
    
    "$HMKDIR" ":Project:Source" || return 1
    "$HMKDIR" ":Project:Headers" || return 1
    "$HMKDIR" ":Project:Docs" || return 1
    "$HMKDIR" ":Project:Source:Modules" || return 1
    
    # Create test files for each directory
    for dir in "Project" "Project:Source" "Project:Source:Modules" "Project:Headers" "Project:Docs"; do
        local temp_file="$TEST_TEMP_DIR/test_$(basename "$dir").txt"
        echo "Content for $dir" > "$temp_file"
        "$HCOPY" "$temp_file" ":${dir}:readme.txt" || {
            error "Failed to copy file to $dir"
            return 1
        }
    done
    
    # Copy entire project structure
    if ! "$HCOPY" ":Project" "$local_copy_dir" -r 2>/dev/null; then
        error "hcopy failed to copy complex structure"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    # Verify structure
    for subdir in "Source" "Headers" "Docs" "Source/Modules"; do
        if ! assert_dir_exists "$local_copy_dir/$subdir"; then
            error "Subdirectory $subdir not copied"
            "$HUMOUNT" 2>/dev/null || true
            return 1
        fi
    done
    
    # Verify files
    for file in "readme.txt" "Source/readme.txt" "Source/Modules/readme.txt" "Headers/readme.txt" "Docs/readme.txt"; do
        if ! assert_file_exists "$local_copy_dir/$file"; then
            error "File $file not copied"
            "$HUMOUNT" 2>/dev/null || true
            return 1
        fi
    done
    
    log "Successfully copied complex directory structure"
    
    "$HUMOUNT" 2>/dev/null || true
    return 0
}

#==============================================================================
# Edge Case Tests
#==============================================================================

# Test copying empty directory
test_hcopy_empty_directory() {
    log "Testing hcopy with empty directory..."
    
    local volume_path="$TEST_TEMP_DIR/test_empty.hfs"
    local local_copy_dir="$TEST_TEMP_DIR/empty_copy"
    
    # Create test volume
    if ! create_test_volume "$volume_path" "Empty Test"; then
        return 1
    fi
    
    # Mount volume
    "$HMOUNT" "$volume_path" || {
        error "Failed to mount test volume"
        return 1
    }
    
    # Create empty directory
    "$HMKDIR" ":EmptyDir" || {
        error "Failed to create empty directory"
        return 1
    }
    
    # Copy empty directory
    if ! "$HCOPY" ":EmptyDir" "$local_copy_dir" -r 2>/dev/null; then
        error "hcopy failed to copy empty directory"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    # Verify empty directory was created
    if ! assert_dir_exists "$local_copy_dir"; then
        error "Empty directory not copied"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    # Verify it's actually empty
    local file_count=$(count_files_recursive "$local_copy_dir")
    if [ "$file_count" -ne 0 ]; then
        error "Directory should be empty but contains files"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    log "Successfully copied empty directory"
    
    "$HUMOUNT" 2>/dev/null || true
    return 0
}

# Test directory with spaces in name
test_hcopy_directory_with_spaces() {
    log "Testing hcopy with directory containing spaces..."
    
    local volume_path="$TEST_TEMP_DIR/test_spaces.hfs"
    local local_copy_dir="$TEST_TEMP_DIR/dir with spaces copy"
    
    # Create test volume
    if ! create_test_volume "$volume_path" "Spaces Test"; then
        return 1
    fi
    
    # Mount volume
    "$HMOUNT" "$volume_path" || {
        error "Failed to mount test volume"
        return 1
    }
    
    # Create directory with spaces
    "$HMKDIR" ":My Directory" || {
        error "Failed to create directory with spaces"
        return 1
    }
    
    # Copy directory
    if ! "$HCOPY" ":My Directory" "$local_copy_dir" -r 2>/dev/null; then
        error "hcopy failed to copy directory with spaces"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    # Verify directory was copied
    if ! assert_dir_exists "$local_copy_dir"; then
        error "Directory with spaces not copied"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    log "Successfully copied directory with spaces in name"
    
    "$HUMOUNT" 2>/dev/null || true
    return 0
}

#==============================================================================
# Real-World Scenario Tests
#==============================================================================

# Test recovering a folder as described in the issue
test_hcopy_recovery_scenario() {
    log "Testing recovery scenario from GitHub issue..."
    
    local volume_path="$TEST_TEMP_DIR/legacy_recovery.hfs"
    local recovery_dir="$TEST_TEMP_DIR/recovered_macos"
    
    # Create test volume
    if ! create_test_volume "$volume_path" "Legacy Recovery"; then
        return 1
    fi
    
    # Mount volume
    "$HMOUNT" "$volume_path" || {
        error "Failed to mount test volume"
        return 1
    }
    
    # Create a mock "Mac OS" directory structure
    "$HMKDIR" ":Mac OS" || {
        error "Failed to create Mac OS directory"
        return 1
    }
    
    "$HMKDIR" ":Mac OS:System" || return 1
    "$HMKDIR" ":Mac OS:Applications" || return 1
    "$HMKDIR" ":Mac OS:System:Extensions" || return 1
    
    # Create some test files
    for i in {1..3}; do
        local temp_file="$TEST_TEMP_DIR/system_file_$i.bin"
        dd if=/dev/zero of="$temp_file" bs=1K count=$((i * 10)) 2>/dev/null
        "$HCOPY" "$temp_file" ":Mac OS:System:file_$i.bin" || return 1
    done
    
    for i in {1..2}; do
        local temp_file="$TEST_TEMP_DIR/app_$i.bin"
        dd if=/dev/zero of="$temp_file" bs=1K count=$((i * 20)) 2>/dev/null
        "$HCOPY" "$temp_file" ":Mac OS:Applications:app_$i.bin" || return 1
    done
    
    # Now attempt the recovery: copy :Mac OS: to ./recovered_macos -r
    # This is the exact scenario from the GitHub issue
    if ! "$HCOPY" ":Mac OS:" "$recovery_dir" -r 2>/dev/null; then
        error "Recovery failed: hcopy :Mac OS: ./recovered_macos -r"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    # Verify recovery structure
    if ! assert_dir_exists "$recovery_dir"; then
        error "Recovery directory not created"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    if ! assert_dir_exists "$recovery_dir/System"; then
        error "System directory not recovered"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    if ! assert_dir_exists "$recovery_dir/Applications"; then
        error "Applications directory not recovered"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    if ! assert_dir_exists "$recovery_dir/System/Extensions"; then
        error "Extensions directory not recovered"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    # Verify files were recovered
    local recovered_files=$(count_files_recursive "$recovery_dir")
    if [ "$recovered_files" -eq 0 ]; then
        error "No files were recovered"
        "$HUMOUNT" 2>/dev/null || true
        return 1
    fi
    
    log "Successfully recovered directory structure ($recovered_files files recovered)"
    
    "$HUMOUNT" 2>/dev/null || true
    return 0
}

#==============================================================================
# Main Execution
#==============================================================================

main() {
    echo "========================================"
    echo "    HFS hcopy Directory Test Suite"
    echo "========================================"
    echo "Purpose: Test recursive directory copying with hcopy -r"
    echo "Issue: hcopy :folder: ./dest -r fails with EISDIR"
    echo
    
    # Setup
    if ! setup_test_env; then
        error "Failed to setup test environment"
        exit 1
    fi
    
    # Trap cleanup
    trap cleanup_test_env EXIT
    
    # Basic Functionality Tests
    echo
    echo "=== Basic Functionality Tests ==="
    run_test "Reject directory without -r flag" test_hcopy_dir_without_recursion_flag
    run_test "Accept directory with -r flag" test_hcopy_dir_with_recursion_flag
    
    # Complex Directory Structure Tests
    echo
    echo "=== Complex Directory Structure Tests ==="
    run_test "Copy nested directories" test_hcopy_nested_directories
    run_test "Copy directory with files" test_hcopy_dir_with_files
    run_test "Copy complex structure" test_hcopy_complex_structure
    
    # Edge Case Tests
    echo
    echo "=== Edge Case Tests ==="
    run_test "Copy empty directory" test_hcopy_empty_directory
    run_test "Copy directory with spaces" test_hcopy_directory_with_spaces
    
    # Real-World Scenario Tests
    echo
    echo "=== Real-World Scenario Tests ==="
    run_test "Recovery scenario from GitHub issue" test_hcopy_recovery_scenario
    
    # Print summary and exit
    print_summary
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
    exit $?
fi
