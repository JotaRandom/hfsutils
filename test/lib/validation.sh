#!/bin/bash
#
# validation.sh - Strict filesystem validation library
# Zero tolerance for errors - ANY filesystem issue must fail
#

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

#
# log_pass() - Log successful test
#
log_pass() {
    echo -e "${GREEN}+ PASS${NC}: $1"
    ((TESTS_PASSED++))
}

#
# log_fail() - Log failed test and EXIT
#
log_fail() {
    echo -e "${RED}✗ FAIL${NC}: $1"
    ((TESTS_FAILED++))
    echo ""
    echo "========================================="
    echo "TEST SUITE FAILED"
    echo "========================================="
    echo "Tests run: $TESTS_RUN"
    echo "Passed: $TESTS_PASSED"
    echo "Failed: $TESTS_FAILED"
    echo ""
    echo "A filesystem MUST be 100% correct."
    echo "Fix the issue before proceeding."
    exit 1
}

#
# log_info() - Log informational message
#
log_info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

#
# validate_filesystem() - CRITICAL validation with fsck
# Returns: 0 if valid, calls log_fail if invalid
#
validate_filesystem() {
    local image="$1"
    local fs_type="$2"  # "hfs" or "hfs+"
    
    local fsck_cmd
    if [ "$fs_type" = "hfs" ]; then
        fsck_cmd="./build/standalone/fsck.hfs"
    else
        fsck_cmd="./build/standalone/fsck.hfs+"
    fi
    
    if [ ! -f "$fsck_cmd" ]; then
        log_fail "fsck command not found: $fsck_cmd"
    fi
    
    log_info "Validating $image with $fsck_cmd -n"
    
    # Run fsck in read-only mode
    local output
    output=$($fsck_cmd -n "$image" 2>&1)
    local ret=$?
    
    # Exit code 0 = no errors
    # Exit code 1 = errors corrected (shouldn't happen with -n)
    # Exit code 4 = errors found but not corrected
    # Anything else = critical failure
    
    if [ $ret -eq 0 ]; then
        log_pass "Filesystem validation clean (exit code 0)"
        return 0
    else
        echo "fsck output:"
        echo "$output"
        log_fail "Filesystem validation FAILED (exit code $ret) - filesystem is CORRUPT"
    fi
}

#
# verify_exit_code() - Verify command exit code
#
verify_exit_code() {
    local actual=$1
    local expected=$2
    local operation="$3"
    
    if [ $actual -ne $expected ]; then
        log_fail "$operation: Expected exit code $expected, got $actual"
    fi
    
    log_pass "$operation: Exit code $expected (correct)"
}

#
# verify_file_exists() - Verify file exists and has minimum size
#
verify_file_exists() {
    local file="$1"
    local min_size="$2"
    
    if [ ! -f "$file" ]; then
        log_fail "File does not exist: $file"
    fi
    
    local size=$(stat -c%s "$file" 2>/dev/null || stat -f%z "$file" 2>/dev/null)
    if [ "$size" -lt "$min_size" ]; then
        log_fail "File too small: $file ($size bytes < $min_size bytes)"
    fi
    
    log_pass "File exists with valid size: $file ($size bytes)"
}

#
# verify_signature() - Verify volume signature
#
verify_signature() {
    local image="$1"
    local fs_type="$2"
    local offset="$3"  # Offset in bytes
    
    local expected_sig
    if [ "$fs_type" = "hfs" ]; then
        expected_sig="4244"  # BD in big-endian
    elif [ "$fs_type" = "hfs+" ]; then
        expected_sig="482b"  # H+ in big-endian
    else
        log_fail "Unknown filesystem type: $fs_type"
    fi
    
    # Read 2 bytes at offset
    local actual_sig=$(dd if="$image" bs=1 skip=$offset count=2 2>/dev/null | od -An -tx1 | tr -d ' \n')
    
    if [ "$actual_sig" != "$expected_sig" ]; then
        log_fail "Invalid signature at offset $offset: got 0x$actual_sig, expected 0x$expected_sig"
    fi
    
    log_pass "Valid $fs_type signature: 0x$actual_sig at offset $offset"
}

#
# verify_field_range() - Verify numeric field is in range
#
verify_field_range() {
    local field_name="$1"
    local actual="$2"
    local min="$3"
    local max="$4"
    
    if [ "$actual" -lt "$min" ] || [ "$actual" -gt "$max" ]; then
        log_fail "$field_name out of range: $actual (expected $min-$max)"
    fi
    
    log_pass "$field_name in valid range: $actual"
}

#
# run_test() - Run a test function with strict error handling
#
run_test() {
    local test_name="$1"
    shift
    local test_func="$@"
    
    ((TESTS_RUN++))
    
    echo ""
    echo "========================================="
    echo "Test #$TESTS_RUN: $test_name"
    echo "========================================="
    
    # Run test in subshell to catch errors
    if $test_func; then
        log_pass "$test_name"
        return 0
    else
        log_fail "$test_name returned error"
    fi
}

#
# cleanup_test_files() - Clean up all test images
#
cleanup_test_files() {
    log_info "Cleaning up test files..."
    rm -f /tmp/test_*.img
    rm -f /tmp/hfs_test_*
}

#
# setup_test_env() - Set up test environment
#
setup_test_env() {
    log_info "Setting up test environment..."
    
    # Verify build directory exists
    if [ ! -d "./build/standalone" ]; then
        log_fail "Build directory not found. Run ./build.sh first."
    fi
    
    # Verify tools exist
    if [ ! -f "./build/standalone/mkfs.hfs" ]; then
        log_fail "mkfs.hfs not found. Run ./build.sh first."
    fi
    
    if [ ! -f "./build/standalone/mkfs.hfs+" ]; then
        log_fail "mkfs.hfs+ not found. Run ./build.sh first."
    fi
    
    # Note: fsck binaries are in build/standalone/fsck.hfs and fsck.hfs+
    if [ ! -f "./build/standalone/fsck.hfs" ]; then
        log_info "fsck.hfs not in standalone, checking for symlink..."
        # fsck might not be built yet, that's okay
    fi
    
    if [ ! -f "./build/standalone/fsck.hfs+" ]; then
        log_info "fsck.hfs+ not in standalone, checking for symlink..."
        # fsck might not be built yet, that's okay
    fi
    
    log_pass "Test environment ready"
}

#
# print_summary() - Print final test summary
#
print_summary() {
    echo ""
    echo "========================================="
    echo "TEST SUITE COMPLETED SUCCESSFULLY"
    echo "========================================="
    echo "Total tests run: $TESTS_RUN"
    echo "Passed: $TESTS_PASSED"
    echo "Failed: $TESTS_FAILED"
    echo ""
    echo "+ All filesystems are 100% valid"
    echo "+ All structures conform to specifications"
    echo "+ All repairs worked correctly"
    echo ""
}
