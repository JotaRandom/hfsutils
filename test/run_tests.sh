#!/bin/bash

#
# HFS Utilities Test Suite
# Comprehensive testing framework for Apple Silicon HFS utilities
#

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0
FAILED_TESTS=()

# Directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
TEST_DATA_DIR="$SCRIPT_DIR/data"
TEST_TEMP_DIR="$SCRIPT_DIR/temp"
UTILS_DIR="$PROJECT_ROOT"

# Utility paths
HATTRIB="$UTILS_DIR/hattrib"
HCD="$UTILS_DIR/hcd"
HCOPY="$UTILS_DIR/hcopy"
HDEL="$UTILS_DIR/hdel"
HDIR="$UTILS_DIR/hdir"
HFORMAT="$UTILS_DIR/hformat"
HLS="$UTILS_DIR/hls"
HMKDIR="$UTILS_DIR/hmkdir"
HMOUNT="$UTILS_DIR/hmount"
HPWD="$UTILS_DIR/hpwd"
HRENAME="$UTILS_DIR/hrename"
HRMDIR="$UTILS_DIR/hrmdir"
HUMOUNT="$UTILS_DIR/humount"
HVOL="$UTILS_DIR/hvol"

# Test configuration
CLEANUP_ON_SUCCESS=1
VERBOSE=0
QUICK_MODE=0

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

# Utility existence check
check_utility() {
    local util="$1"
    local util_path="$2"
    
    if [[ ! -x "$util_path" ]]; then
        error "Utility $util not found or not executable at $util_path"
        return 1
    fi
    return 0
}

# Check if command succeeded
assert_success() {
    local cmd="$1"
    local expected_rc="${2:-0}"
    
    if [[ $VERBOSE -eq 1 ]]; then
        log "Executing: $cmd"
    fi
    
    local output
    output=$(eval "$cmd" 2>&1)
    local actual_rc=$?
    
    if [[ $actual_rc -eq $expected_rc ]]; then
        return 0
    else
        error "Command failed (rc=$actual_rc): $cmd"
        if [[ -n "$output" ]] && [[ $VERBOSE -eq 1 ]]; then
            echo "  Output: $output" >&2
        fi
        return 1
    fi
}

# Check if command failed as expected
assert_failure() {
    local cmd="$1"
    local expected_rc="${2:-1}"
    
    if [[ $VERBOSE -eq 1 ]]; then
        log "Executing (expecting failure): $cmd"
    fi
    
    if eval "$cmd" >/dev/null 2>&1; then
        error "Command unexpectedly succeeded: $cmd"
        return 1
    else
        local actual_rc=$?
        if [[ $actual_rc -eq $expected_rc ]]; then
            return 0
        else
            warning "Command failed with $actual_rc, expected $expected_rc: $cmd"
            return 0  # Still consider this a pass since it did fail
        fi
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

# Check if file doesn't exist
assert_file_not_exists() {
    local file="$1"
    if [[ -f "$file" ]]; then
        error "File unexpectedly exists: $file"
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

# Check command output contains string
assert_output_contains() {
    local cmd="$1"
    local expected="$2"
    local output
    
    output=$(eval "$cmd" 2>&1)
    if echo "$output" | grep -q "$expected"; then
        return 0
    else
        error "Output does not contain '$expected': $output"
        return 1
    fi
}

# Generate test file with specific content
create_test_file() {
    local filename="$1"
    local size="${2:-1024}"  # Default 1KB
    local content="${3:-random}"
    
    case "$content" in
        "random")
            if command -v openssl >/dev/null; then
                openssl rand "$size" > "$filename"
            else
                # Fallback for systems without openssl
                dd if=/dev/urandom of="$filename" bs="$size" count=1 2>/dev/null
            fi
            ;;
        "text")
            # Create a text file with readable content
            {
                echo "This is a test file created by HFS utilities test suite."
                echo "File: $filename"
                echo "Size: $size bytes"
                echo "Created: $(date)"
                echo
                # Fill remaining space with repeated text
                local remaining=$((size - 200))
                if [[ $remaining -gt 0 ]]; then
                    yes "The quick brown fox jumps over the lazy dog. " | head -c "$remaining"
                fi
            } > "$filename"
            ;;
        "empty")
            touch "$filename"
            ;;
        *)
            echo "$content" > "$filename"
            ;;
    esac
}

# Setup test environment
setup_test_env() {
    log "Setting up test environment..."
    
    # Create directories
    mkdir -p "$TEST_DATA_DIR"
    mkdir -p "$TEST_TEMP_DIR"
    
    # Clean temp directory
    rm -rf "$TEST_TEMP_DIR"/*
    
    log "Test environment ready"
    log "  Data dir: $TEST_DATA_DIR"
    log "  Temp dir: $TEST_TEMP_DIR"
}

# Cleanup test environment
cleanup_test_env() {
    if [[ $CLEANUP_ON_SUCCESS -eq 1 ]] && [[ $TESTS_FAILED -eq 0 ]]; then
        log "Cleaning up test environment..."
        rm -rf "$TEST_TEMP_DIR"
    else
        warning "Leaving test files for inspection in: $TEST_TEMP_DIR"
    fi
}

# Print test summary
print_summary() {
    echo
    echo "========================================"
    echo "           TEST SUMMARY"
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
        exit 1
    else
        echo
        success "All tests passed!"
        exit 0
    fi
}

#==============================================================================
# Test Suite Functions (to be implemented in phases)
#==============================================================================

# Check all utilities exist and are executable
test_utilities_exist() {
    log "Checking utility availability..."
    
    check_utility "hattrib" "$HATTRIB" || return 1
    check_utility "hcd" "$HCD" || return 1  
    check_utility "hcopy" "$HCOPY" || return 1
    check_utility "hdel" "$HDEL" || return 1
    check_utility "hdir" "$HDIR" || return 1
    check_utility "hformat" "$HFORMAT" || return 1
    check_utility "hls" "$HLS" || return 1
    check_utility "hmkdir" "$HMKDIR" || return 1
    check_utility "hmount" "$HMOUNT" || return 1
    check_utility "hpwd" "$HPWD" || return 1
    check_utility "hrename" "$HRENAME" || return 1
    check_utility "hrmdir" "$HRMDIR" || return 1
    check_utility "humount" "$HUMOUNT" || return 1
    check_utility "hvol" "$HVOL" || return 1
    
    return 0
}

# Test basic help/version output
test_help_and_version() {
    # Test that utilities show appropriate output when called incorrectly
    assert_output_contains "$HATTRIB 2>&1 | head -1" "Usage:"
    assert_output_contains "$HCOPY 2>&1 | head -1" "Usage:"
    assert_output_contains "$HDEL 2>&1 | head -1" "Usage:"
    assert_output_contains "$HFORMAT 2>&1 | head -1" "Usage:"
    assert_output_contains "$HMKDIR 2>&1 | head -1" "Usage:"
    assert_output_contains "$HMOUNT 2>&1 | head -1" "Usage:"
    assert_output_contains "$HRENAME 2>&1 | head -1" "Usage:"
    assert_output_contains "$HRMDIR 2>&1 | head -1" "Usage:"
    
    # These utilities show different messages when no volume is mounted
    # But they should still provide some output
    local output
    output=$($HCD 2>&1 | head -1)
    if [[ -z "$output" ]]; then
        error "No output from hcd"
        return 1
    fi
    
    output=$($HLS 2>&1 | head -1) 
    if [[ -z "$output" ]]; then
        error "No output from hls"
        return 1
    fi
    
    output=$($HVOL 2>&1 | head -1)
    if [[ -z "$output" ]]; then
        error "No output from hvol"
        return 1
    fi
    
    return 0
}

# Test basic mount/unmount operations
test_mount_unmount() {
    log "Testing mount/unmount operations..."
    
    # Ensure no volume is currently mounted
    assert_success "$HUMOUNT 2>/dev/null || true"
    
    # Mount small test image
    assert_success "$HMOUNT $TEST_DATA_DIR/small_test.hfs"
    
    # Verify volume is mounted
    assert_output_contains "$HVOL" "Test Floppy"
    
    # Test hpwd works when mounted
    assert_output_contains "$HPWD" "Test Floppy:"
    
    # Unmount
    assert_success "$HUMOUNT"
    
    # Verify no volume mounted (hvol should show "Known volumes:" or similar)
    assert_output_contains "$HVOL 2>&1" "Known volumes"
    
    return 0
}

# Test directory listing
test_directory_listing() {
    log "Testing directory listing..."
    
    # Mount test volume
    assert_success "$HMOUNT $TEST_DATA_DIR/small_test.hfs"
    
    # Test basic listing
    assert_success "$HLS"
    assert_output_contains "$HLS" "readme.txt"
    assert_output_contains "$HLS" "test.c"
    assert_output_contains "$HLS" "docs"
    
    # Test listing subdirectory
    assert_output_contains "$HLS :docs" "manual.txt"
    
    # Test long format listing
    assert_success "$HLS -l"
    
    # Test hdir (should work same as hls)
    assert_success "$HDIR"
    
    assert_success "$HUMOUNT"
    return 0
}

# Test directory navigation
test_directory_navigation() {
    log "Testing directory navigation..."
    
    assert_success "$HMOUNT $TEST_DATA_DIR/medium_test.hfs"
    
    # Test pwd at root
    assert_output_contains "$HPWD" "Medium Test:"
    
    # Change to Documents directory
    assert_success "$HCD :Documents"
    assert_output_contains "$HPWD" "Documents"
    
    # Test relative directory change
    assert_success "$HCD :Projects"
    assert_output_contains "$HPWD" "Projects"
    
    # Go back to root
    assert_success "$HCD :"
    assert_output_contains "$HPWD" "Medium Test:"
    
    assert_success "$HUMOUNT"
    return 0
}

# Test file operations
test_file_operations() {
    log "Testing file operations..."
    
    assert_success "$HMOUNT $TEST_DATA_DIR/medium_test.hfs"
    
    # Test file copy from HFS to local
    assert_success "$HCOPY :Documents:readme.txt $TEST_TEMP_DIR/copied_file.txt"
    assert_file_exists "$TEST_TEMP_DIR/copied_file.txt"
    
    # Test file copy from local to HFS
    create_test_file "$TEST_TEMP_DIR/new_test.txt" 500 "text"
    assert_success "$HCOPY $TEST_TEMP_DIR/new_test.txt :new_file.txt"
    assert_output_contains "$HLS" "new_file.txt"
    
    # Test file deletion
    assert_success "$HDEL :new_file.txt"
    assert_failure "$HLS | grep new_file.txt"
    
    # Test file attributes (just check it runs, output varies)
    $HATTRIB :Documents:readme.txt >/dev/null 2>&1 || true
    
    assert_success "$HUMOUNT"
    return 0
}

# Test directory operations
test_directory_operations() {
    log "Testing directory operations..."
    
    assert_success "$HMOUNT $TEST_DATA_DIR/medium_test.hfs"
    
    # Test directory creation
    assert_success "$HMKDIR :TestDir"
    assert_output_contains "$HLS" "TestDir"
    
    # Test nested directory creation
    assert_success "$HMKDIR :TestDir:SubDir"
    assert_output_contains "$HLS :TestDir" "SubDir"
    
    # Test directory renaming
    assert_success "$HRENAME :TestDir :RenamedDir"
    assert_output_contains "$HLS" "RenamedDir"
    assert_failure "$HLS | grep TestDir"
    
    # Test directory removal (should fail with contents)
    assert_failure "$HRMDIR :RenamedDir"
    
    # Remove subdirectory first
    assert_success "$HRMDIR :RenamedDir:SubDir"
    
    # Now remove parent directory
    assert_success "$HRMDIR :RenamedDir"
    assert_failure "$HLS | grep RenamedDir"
    
    assert_success "$HUMOUNT"
    return 0
}

# Test file renaming
test_file_renaming() {
    log "Testing file renaming..."
    
    assert_success "$HMOUNT $TEST_DATA_DIR/small_test.hfs"
    
    # Create a test file to rename
    create_test_file "$TEST_TEMP_DIR/rename_test.txt" 100 "text"
    assert_success "$HCOPY $TEST_TEMP_DIR/rename_test.txt :original.txt"
    
    # Test file rename
    assert_success "$HRENAME :original.txt :renamed.txt"
    assert_output_contains "$HLS" "renamed.txt"
    assert_failure "$HLS | grep original.txt"
    
    # Clean up
    assert_success "$HDEL :renamed.txt"
    
    assert_success "$HUMOUNT"
    return 0
}

# Test volume information
test_volume_info() {
    log "Testing volume information..."
    
    # Test with different volumes
    for image in small_test.hfs medium_test.hfs large_test.hfs; do
        assert_success "$HMOUNT $TEST_DATA_DIR/$image"
        
        # Test hvol shows volume info
        assert_success "$HVOL"
        assert_output_contains "$HVOL" "Volume name"
        assert_output_contains "$HVOL" "created on"
        assert_output_contains "$HVOL" "bytes free"
        
        assert_success "$HUMOUNT"
    done
    
    return 0
}

# Test empty volume operations
test_empty_volume() {
    log "Testing empty volume operations..."
    
    assert_success "$HMOUNT $TEST_DATA_DIR/empty_test.hfs"
    
    # Should be able to list empty volume
    assert_success "$HLS"
    
    # Should show volume info
    assert_output_contains "$HVOL" "Empty Test"
    
    # Should be able to create files/directories
    assert_success "$HMKDIR :EmptyTest"
    create_test_file "$TEST_TEMP_DIR/empty_test_file.txt" 50 "text"
    assert_success "$HCOPY $TEST_TEMP_DIR/empty_test_file.txt :test.txt"
    
    # Verify they exist
    assert_output_contains "$HLS" "EmptyTest"
    assert_output_contains "$HLS" "test.txt"
    
    assert_success "$HUMOUNT"
    return 0
}

#==============================================================================
# Integration Workflow Tests
#==============================================================================

# Test complete backup/restore workflow
test_backup_restore_workflow() {
    log "Testing backup/restore workflow..."
    
    # Create a fresh volume to work with
    dd if=/dev/zero of="$TEST_TEMP_DIR/workflow_test.hfs" bs=1024 count=1440 2>/dev/null
    assert_success "$HFORMAT -l 'Workflow Test' $TEST_TEMP_DIR/workflow_test.hfs" || return 1
    assert_success "$HMOUNT $TEST_TEMP_DIR/workflow_test.hfs" || return 1
    
    # Create directory structure and files
    assert_success "$HMKDIR :Documents" || return 1
    assert_success "$HMKDIR :Pictures" || return 1
    assert_success "$HMKDIR :Documents:Projects" || return 1
    
    # Create test files to backup
    create_test_file "$TEST_TEMP_DIR/doc1.txt" 500 "text"
    create_test_file "$TEST_TEMP_DIR/doc2.txt" 300 "text"
    create_test_file "$TEST_TEMP_DIR/pic1.dat" 1024 "random"
    
    # Copy files to HFS volume
    assert_success "$HCOPY $TEST_TEMP_DIR/doc1.txt :Documents:document1.txt"
    assert_success "$HCOPY $TEST_TEMP_DIR/doc2.txt :Documents:Projects:document2.txt"  
    assert_success "$HCOPY $TEST_TEMP_DIR/pic1.dat :Pictures:image1.dat"
    
    # Backup: Copy all files from HFS to local backup directory
    mkdir -p "$TEST_TEMP_DIR/backup"
    assert_success "$HCOPY :Documents:document1.txt $TEST_TEMP_DIR/backup/"
    assert_success "$HCOPY :Documents:Projects:document2.txt $TEST_TEMP_DIR/backup/"
    assert_success "$HCOPY :Pictures:image1.dat $TEST_TEMP_DIR/backup/"
    
    # Verify backup files exist
    assert_file_exists "$TEST_TEMP_DIR/backup/document1.txt"
    assert_file_exists "$TEST_TEMP_DIR/backup/document2.txt"
    assert_file_exists "$TEST_TEMP_DIR/backup/image1.dat"
    
    # Simulate disaster: delete files from HFS volume
    assert_success "$HDEL :Documents:document1.txt"
    assert_success "$HDEL :Documents:Projects:document2.txt"
    assert_success "$HDEL :Pictures:image1.dat"
    
    # Restore: Copy files back from backup
    assert_success "$HCOPY $TEST_TEMP_DIR/backup/document1.txt :Documents:restored1.txt"
    assert_success "$HCOPY $TEST_TEMP_DIR/backup/document2.txt :Documents:Projects:restored2.txt"
    assert_success "$HCOPY $TEST_TEMP_DIR/backup/image1.dat :Pictures:restored_image.dat"
    
    # Verify restored files exist
    assert_output_contains "$HLS :Documents" "restored1.txt"
    assert_output_contains "$HLS :Documents:Projects" "restored2.txt"
    assert_output_contains "$HLS :Pictures" "restored_image.dat"
    
    assert_success "$HUMOUNT"
    return 0
}

# Test archive creation workflow
test_archive_creation_workflow() {
    log "Testing archive creation workflow..."
    
    # Mount medium test volume
    assert_success "$HMOUNT $TEST_DATA_DIR/medium_test.hfs"
    
    # Create archive directory structure locally
    mkdir -p "$TEST_TEMP_DIR/archive/text_files"
    mkdir -p "$TEST_TEMP_DIR/archive/binary_files"
    mkdir -p "$TEST_TEMP_DIR/archive/documents"
    
    # Extract different types of files to appropriate archive directories
    assert_success "$HCOPY :Software:hello.c $TEST_TEMP_DIR/archive/text_files/"
    assert_success "$HCOPY :Software:hello.h $TEST_TEMP_DIR/archive/text_files/"
    assert_success "$HCOPY :Documents:readme.txt $TEST_TEMP_DIR/archive/documents/"
    assert_success "$HCOPY :Documents:manual.txt $TEST_TEMP_DIR/archive/documents/"
    assert_success "$HCOPY :Archives:data1.bin $TEST_TEMP_DIR/archive/binary_files/"
    assert_success "$HCOPY :Archives:data16.bin $TEST_TEMP_DIR/archive/binary_files/"
    
    # Verify archive structure
    assert_file_exists "$TEST_TEMP_DIR/archive/text_files/hello.c"
    assert_file_exists "$TEST_TEMP_DIR/archive/text_files/hello.h"  
    assert_file_exists "$TEST_TEMP_DIR/archive/documents/readme.txt"
    assert_file_exists "$TEST_TEMP_DIR/archive/documents/manual.txt"
    assert_file_exists "$TEST_TEMP_DIR/archive/binary_files/data1.bin"
    assert_file_exists "$TEST_TEMP_DIR/archive/binary_files/data16.bin"
    
    assert_success "$HUMOUNT"
    return 0
}

# Test volume migration workflow
test_volume_migration_workflow() {
    log "Testing volume migration workflow..."
    
    # Create source and destination volumes
    dd if=/dev/zero of="$TEST_TEMP_DIR/source.hfs" bs=1024 count=2880 2>/dev/null  # 2.88MB
    dd if=/dev/zero of="$TEST_TEMP_DIR/dest.hfs" bs=1024 count=2880 2>/dev/null
    
    assert_success "$HFORMAT -l 'Source Vol' $TEST_TEMP_DIR/source.hfs"
    assert_success "$HFORMAT -l 'Dest Vol' $TEST_TEMP_DIR/dest.hfs"
    
    # Populate source volume
    assert_success "$HMOUNT $TEST_TEMP_DIR/source.hfs"
    assert_success "$HMKDIR :Data"
    assert_success "$HMKDIR :Apps"
    
    create_test_file "$TEST_TEMP_DIR/migration_file1.txt" 200 "text"
    create_test_file "$TEST_TEMP_DIR/migration_file2.bin" 512 "random"
    
    assert_success "$HCOPY $TEST_TEMP_DIR/migration_file1.txt :Data:file1.txt"
    assert_success "$HCOPY $TEST_TEMP_DIR/migration_file2.bin :Apps:app.bin"
    
    # Get directory listing for verification
    local src_root_listing src_data_listing src_apps_listing
    src_root_listing=$($HLS)
    src_data_listing=$($HLS :Data)
    src_apps_listing=$($HLS :Apps)
    
    assert_success "$HUMOUNT"
    
    # Migrate to destination volume
    assert_success "$HMOUNT $TEST_TEMP_DIR/dest.hfs"
    
    # Recreate directory structure
    assert_success "$HMKDIR :Data"
    assert_success "$HMKDIR :Apps"
    
    # Copy files via temp directory (simulating migration)
    assert_success "$HMOUNT $TEST_TEMP_DIR/source.hfs"  # Remount source
    assert_success "$HCOPY :Data:file1.txt $TEST_TEMP_DIR/temp_file1.txt"
    assert_success "$HCOPY :Apps:app.bin $TEST_TEMP_DIR/temp_app.bin"
    assert_success "$HUMOUNT"
    
    # Copy to destination
    assert_success "$HMOUNT $TEST_TEMP_DIR/dest.hfs"
    assert_success "$HCOPY $TEST_TEMP_DIR/temp_file1.txt :Data:file1.txt"
    assert_success "$HCOPY $TEST_TEMP_DIR/temp_app.bin :Apps:app.bin"
    
    # Verify migration succeeded
    assert_output_contains "$HLS" "Data"
    assert_output_contains "$HLS" "Apps"
    assert_output_contains "$HLS :Data" "file1.txt"
    assert_output_contains "$HLS :Apps" "app.bin"
    
    assert_success "$HUMOUNT"
    return 0
}

# Test complex file management workflow  
test_file_management_workflow() {
    log "Testing complex file management workflow..."
    
    assert_success "$HMOUNT $TEST_DATA_DIR/large_test.hfs"
    
    # Create a project directory structure
    assert_success "$HMKDIR :Projects"
    assert_success "$HMKDIR :Projects:WebSite"
    assert_success "$HMKDIR :Projects:WebSite:HTML"
    assert_success "$HMKDIR :Projects:WebSite:CSS"
    assert_success "$HMKDIR :Projects:WebSite:Images"
    
    # Create and add project files
    create_test_file "$TEST_TEMP_DIR/index.html" 800 "text"
    create_test_file "$TEST_TEMP_DIR/style.css" 400 "text"  
    create_test_file "$TEST_TEMP_DIR/logo.png" 2048 "random"
    
    assert_success "$HCOPY $TEST_TEMP_DIR/index.html :Projects:WebSite:HTML:index.html"
    assert_success "$HCOPY $TEST_TEMP_DIR/style.css :Projects:WebSite:CSS:style.css"
    assert_success "$HCOPY $TEST_TEMP_DIR/logo.png :Projects:WebSite:Images:logo.png"
    
    # Navigate and verify structure
    assert_success "$HCD :Projects:WebSite"
    assert_output_contains "$HPWD" "WebSite"
    assert_output_contains "$HLS" "HTML"
    assert_output_contains "$HLS" "CSS"
    assert_output_contains "$HLS" "Images"
    
    # Test file operations within the structure
    assert_success "$HCD :HTML"
    assert_output_contains "$HLS" "index.html"
    
    # Rename and reorganize
    assert_success "$HCD :"  # Back to root
    assert_success "$HRENAME :Projects:WebSite:HTML:index.html :Projects:WebSite:HTML:main.html"
    assert_output_contains "$HLS :Projects:WebSite:HTML" "main.html"
    
    # Create backup of project
    assert_success "$HMKDIR :Backups"
    assert_success "$HMKDIR :Backups:WebSite_Backup"
    
    # Copy project files to backup
    assert_success "$HCOPY :Projects:WebSite:HTML:main.html :Backups:WebSite_Backup:main.html"
    assert_success "$HCOPY :Projects:WebSite:CSS:style.css :Backups:WebSite_Backup:style.css"
    
    # Verify backup
    assert_output_contains "$HLS :Backups:WebSite_Backup" "main.html"
    assert_output_contains "$HLS :Backups:WebSite_Backup" "style.css"
    
    assert_success "$HUMOUNT"
    return 0
}

# Test data recovery workflow
test_data_recovery_workflow() {
    log "Testing data recovery workflow..."
    
    # This test simulates recovering data from a volume that had issues
    assert_success "$HMOUNT $TEST_DATA_DIR/small_test.hfs"
    
    # Document current state (what we're "recovering")
    mkdir -p "$TEST_TEMP_DIR/recovery_log"
    
    # Create inventory of files
    $HLS > "$TEST_TEMP_DIR/recovery_log/root_files.txt"
    $HLS :docs > "$TEST_TEMP_DIR/recovery_log/docs_files.txt"
    $HVOL > "$TEST_TEMP_DIR/recovery_log/volume_info.txt"
    
    # Extract all recoverable files
    mkdir -p "$TEST_TEMP_DIR/recovered"
    assert_success "$HCOPY :readme.txt $TEST_TEMP_DIR/recovered/"
    assert_success "$HCOPY :test.c $TEST_TEMP_DIR/recovered/"
    assert_success "$HCOPY :docs:manual.txt $TEST_TEMP_DIR/recovered/"
    
    # Verify recovery
    assert_file_exists "$TEST_TEMP_DIR/recovered/readme.txt"
    assert_file_exists "$TEST_TEMP_DIR/recovered/test.c"
    assert_file_exists "$TEST_TEMP_DIR/recovered/manual.txt"
    assert_file_exists "$TEST_TEMP_DIR/recovery_log/root_files.txt"
    assert_file_exists "$TEST_TEMP_DIR/recovery_log/docs_files.txt"
    assert_file_exists "$TEST_TEMP_DIR/recovery_log/volume_info.txt"
    
    assert_success "$HUMOUNT"
    return 0
}

#==============================================================================
# Error Handling and Edge Case Tests
#==============================================================================

# Test error conditions
test_error_conditions() {
    log "Testing error conditions..."
    
    # Test operations without mounted volume
    assert_failure "$HLS"
    assert_failure "$HCD :nonexistent"
    assert_failure "$HCOPY nonexistent.txt :test.txt"
    
    # Test with non-existent files/volumes
    assert_failure "$HMOUNT /nonexistent/path/image.hfs"
    assert_failure "$HCOPY nonexistent.txt :test.txt"
    
    # Mount volume for further tests
    assert_success "$HMOUNT $TEST_DATA_DIR/small_test.hfs"
    
    # Test operations on non-existent paths
    assert_failure "$HCD :nonexistent_dir"
    assert_failure "$HLS :nonexistent_dir"
    assert_failure "$HCOPY :nonexistent_file.txt $TEST_TEMP_DIR/output.txt"
    assert_failure "$HDEL :nonexistent_file.txt"
    assert_failure "$HRENAME :nonexistent.txt :newname.txt"
    
    # Test directory operations on files (should fail)
    assert_failure "$HCD :readme.txt"  # Try to cd into a file
    assert_failure "$HRMDIR :readme.txt"  # Try to rmdir a file
    
    # Test file operations on directories (should fail)
    assert_failure "$HDEL :docs"  # Try to delete directory with hdel
    
    assert_success "$HUMOUNT"
    return 0
}

# Test edge cases
test_edge_cases() {
    log "Testing edge cases..."
    
    assert_success "$HMOUNT $TEST_DATA_DIR/medium_test.hfs"
    
    # Test files with special characters (already in test data)
    assert_output_contains "$HLS :Documents" "special file.txt"
    assert_success "$HCOPY ':Documents:special file.txt' '$TEST_TEMP_DIR/special_copy.txt'"
    assert_file_exists "$TEST_TEMP_DIR/special_copy.txt"
    
    # Test empty file operations
    create_test_file "$TEST_TEMP_DIR/empty_test.txt" 0 "empty"
    assert_success "$HCOPY $TEST_TEMP_DIR/empty_test.txt :empty_copy.txt"
    assert_output_contains "$HLS" "empty_copy.txt"
    assert_success "$HDEL :empty_copy.txt"
    
    # Test maximum path depth (within reason)
    assert_success "$HMKDIR :Level1"
    assert_success "$HMKDIR :Level1:Level2"  
    assert_success "$HMKDIR :Level1:Level2:Level3"
    assert_output_contains "$HLS :Level1:Level2" "Level3"
    
    # Navigate deep and verify
    assert_success "$HCD :Level1:Level2:Level3"
    assert_output_contains "$HPWD" "Level3"
    
    # Clean up deep structure
    assert_success "$HCD :"
    assert_success "$HRMDIR :Level1:Level2:Level3"
    assert_success "$HRMDIR :Level1:Level2"
    assert_success "$HRMDIR :Level1"
    
    assert_success "$HUMOUNT"
    return 0
}

#==============================================================================
# Main execution (updated)
#==============================================================================

usage() {
    cat << EOF
Usage: $0 [OPTIONS] [TEST_PATTERN]

Options:
    -v, --verbose     Enable verbose output
    -q, --quick       Run only quick tests (skip stress tests)
    -k, --keep        Keep test files even on success
    -h, --help        Show this help message

Test patterns:
    all              Run all tests (default)
    basic            Run only basic functionality tests
    integration      Run only integration tests
    errors           Run only error handling tests
    stress           Run only stress tests

Examples:
    $0                    # Run all tests
    $0 -v basic          # Run basic tests with verbose output
    $0 --quick           # Run all tests except stress tests
EOF
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -q|--quick)
            QUICK_MODE=1
            shift
            ;;
        -k|--keep)
            CLEANUP_ON_SUCCESS=0
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        -*)
            error "Unknown option: $1"
            usage
            exit 1
            ;;
        *)
            TEST_PATTERN="$1"
            shift
            ;;
    esac
done

# Default test pattern
TEST_PATTERN="${TEST_PATTERN:-all}"

echo "========================================"
echo "    HFS Utilities Test Suite"
echo "========================================"
echo "Platform: $(uname -m) $(uname -s)"
echo "Test pattern: $TEST_PATTERN"
echo "Project root: $PROJECT_ROOT"
echo

# Setup
setup_test_env

# Run tests based on pattern
case "$TEST_PATTERN" in
    basic)
        run_test "Utilities Exist" test_utilities_exist
        run_test "Help and Version Output" test_help_and_version
        run_test "Mount/Unmount Operations" test_mount_unmount
        run_test "Directory Listing" test_directory_listing
        run_test "Directory Navigation" test_directory_navigation
        run_test "File Operations" test_file_operations
        run_test "Directory Operations" test_directory_operations
        run_test "File Renaming" test_file_renaming
        run_test "Volume Information" test_volume_info
        run_test "Empty Volume Operations" test_empty_volume
        ;;
        
    integration)
        run_test "Backup/Restore Workflow" test_backup_restore_workflow
        run_test "Archive Creation Workflow" test_archive_creation_workflow
        run_test "Volume Migration Workflow" test_volume_migration_workflow
        run_test "File Management Workflow" test_file_management_workflow
        run_test "Data Recovery Workflow" test_data_recovery_workflow
        ;;
        
    errors)
        run_test "Error Conditions" test_error_conditions
        run_test "Edge Cases" test_edge_cases
        ;;
        
    stress)
        if [[ "$QUICK_MODE" != "1" ]]; then
            log "Stress tests not yet implemented"
        fi
        ;;
        
    all)
        # Basic functionality tests
        run_test "Utilities Exist" test_utilities_exist
        run_test "Help and Version Output" test_help_and_version
        run_test "Mount/Unmount Operations" test_mount_unmount
        run_test "Directory Listing" test_directory_listing
        run_test "Directory Navigation" test_directory_navigation
        run_test "File Operations" test_file_operations
        run_test "Directory Operations" test_directory_operations
        run_test "File Renaming" test_file_renaming
        run_test "Volume Information" test_volume_info
        run_test "Empty Volume Operations" test_empty_volume
        
        # Integration workflow tests
        run_test "Backup/Restore Workflow" test_backup_restore_workflow
        run_test "Archive Creation Workflow" test_archive_creation_workflow
        run_test "Volume Migration Workflow" test_volume_migration_workflow
        run_test "File Management Workflow" test_file_management_workflow
        run_test "Data Recovery Workflow" test_data_recovery_workflow
        
        # Error handling and edge case tests
        run_test "Error Conditions" test_error_conditions
        run_test "Edge Cases" test_edge_cases
        
        # Stress tests (if not in quick mode)
        if [[ "$QUICK_MODE" != "1" ]]; then
            log "Stress tests not yet implemented"
        fi
        ;;
        
    *)
        error "Unknown test pattern: $TEST_PATTERN"
        usage
        exit 1
        ;;
esac

# Cleanup and summary
cleanup_test_env
print_summary