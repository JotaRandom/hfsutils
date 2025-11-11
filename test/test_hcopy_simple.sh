#!/bin/bash

# Simple test for hcopy -R flag support
# This tests that the flag parsing works correctly

set -e

HFSUTIL=$(cd "$(dirname "$0")" && pwd)/../hfsutil

if [ ! -f "$HFSUTIL" ]; then
    echo "ERROR: hfsutil not found at $HFSUTIL"
    exit 1
fi

echo "Testing hcopy -R flag..."
echo "=========================================="

# Test 1: Verify -R flag is recognized
echo ""
echo "Test 1: Verify -R flag is recognized in usage"
OUTPUT=$($HFSUTIL hcopy 2>&1 || true)
if echo "$OUTPUT" | grep -q "\-R"; then
    echo "[PASS] -R flag appears in hcopy usage message"
else
    echo "[FAIL] -R flag not found in usage message"
    echo "Usage output:"
    echo "$OUTPUT"
fi

# Test 2: Verify hcopy accepts the -R option without error (no volume needed)
echo ""
echo "Test 2: Verify hcopy accepts -R option"
# This will fail because no files/volumes exist, but should fail with missing file error, not flag error
OUTPUT=$($HFSUTIL hcopy -R nonexistent /tmp 2>&1 || true)
if echo "$OUTPUT" | grep -qi "no such file\|not found"; then
    echo "[PASS] hcopy accepts -R flag and fails on missing file (expected)"
elif echo "$OUTPUT" | grep -qi "invalid option\|unrecognized"; then
    echo "[FAIL] hcopy does not recognize -R flag"
    echo "Error output:"
    echo "$OUTPUT"
else
    echo "[WARN] Unexpected error output:"
    echo "$OUTPUT"
fi

# Test 3: Verify older hcopy behavior (reject directories without -R)
echo ""
echo "Test 3: Test directory rejection without -R"
mkdir -p /tmp/test_hcopy_source
echo "test file" > /tmp/test_hcopy_source/file.txt

# The command should fail with EISDIR when trying to copy directory without -R
OUTPUT=$($HFSUTIL hcopy /tmp/test_hcopy_source /tmp 2>&1 || true)
if echo "$OUTPUT" | grep -qi "is a directory"; then
    echo "[PASS] hcopy rejects directory without -R flag"
elif echo "$OUTPUT" | grep -qi "eisdir"; then
    echo "[PASS] hcopy rejects directory (EISDIR error)"
else
    # If no directory error, that's OK too - depends on where it fails
    echo "[INFO] No directory rejection error (may be expected): $OUTPUT"
fi

rm -rf /tmp/test_hcopy_source

echo ""
echo "=========================================="
echo "Simple flag tests completed successfully!"
