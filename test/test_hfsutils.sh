#!/bin/bash
#
# test_hfsutils.sh - Test hfsutil commands on HFS and HFS+ volumes
# Tests mkdir, ls, hcopy (in/out) - used on systems without HFS mount drivers
#

set -e
cd "$(dirname "$0")/.."

HFSUTIL="./hfsutil"
TMP="/tmp/test_hfsutils_$$"
mkdir -p "$TMP"
trap "rm -rf $TMP" EXIT

echo "========================================="
echo "  hfsutil Commands Test Suite"
echo "  (for systems without HFS mount drivers)"
echo "========================================="
echo ""

# Create test volume (10MB - works with HFS and HFS+)
IMG="$TMP/test.img"
dd if=/dev/zero of="$IMG" bs=1M count=10 2>/dev/null

#
# TEST 1: HFS Volume Operations
#
echo "=== Test 1: HFS Volume Operations ==="
echo "[1] Format as HFS..."
$HFSUTIL hformat -l "TestHFS" "$IMG" >/dev/null 2>&1 || { echo "FAIL: hformat"; exit 1; }
echo "  + hformat created HFS volume"

echo "[2] Mount volume..."
$HFSUTIL hmount "$IMG" >/dev/null 2>&1 || { echo "FAIL: hmount"; exit 1; }
echo "  + hmount successful"

echo "[3] List root directory..."
$HFSUTIL hls >/dev/null 2>&1 || { echo "FAIL: hls"; exit 1; }
echo "  + hls works"

echo "[4] Create directory..."
$HFSUTIL hmkdir testdir >/dev/null 2>&1 || { echo "FAIL: hmkdir"; exit 1; }
echo "  + hmkdir created directory"

echo "[5] Verify directory..."
$HFSUTIL hls | grep -q testdir || { echo "FAIL: directory not found"; exit 1; }
echo "  + Directory visible in hls"

echo "[6] Copy file into volume..."
echo "Test content" > "$TMP/testfile.txt"
$HFSUTIL hcopy "$TMP/testfile.txt" :testfile.txt >/dev/null 2>&1 || { echo "FAIL: hcopy in"; exit 1; }
echo "  + hcopy (host→HFS) works"

echo "[7] Verify file..."
$HFSUTIL hls | grep -q testfile || { echo "FAIL: file not found"; exit 1; }
echo "  + File visible in hls"

echo "[8] Copy file out of volume..."
$HFSUTIL hcopy :testfile.txt "$TMP/retrieved.txt" >/dev/null 2>&1 || { echo "FAIL: hcopy out"; exit 1; }
echo "  + hcopy (HFS→host) works"

echo "[9] Verify copied content..."
diff "$TMP/testfile.txt" "$TMP/retrieved.txt" >/dev/null 2>&1 || { echo "FAIL: content mismatch"; exit 1; }
echo "  + File content intact"

echo "[10] Unmount..."
$HFSUTIL humount >/dev/null 2>&1 || { echo "FAIL: humount"; exit 1; }
echo "  + humount successful"

echo "+ HFS operations complete"
echo ""

#
# TEST 2: HFS+ Volume Operations
#
echo "=== Test 2: HFS+ Volume Operations ==="
echo "[1] Format as HFS+..."
$HFSUTIL hformat -t hfs+ -l "TestHFSPlus" "$IMG" >/dev/null 2>&1 || { echo "FAIL: hformat -t hfs+"; exit 1; }
echo "  + hformat created HFS+ volume"

echo "[2] Mount volume..."
$HFSUTIL hmount "$IMG" >/dev/null 2>&1 || { echo "FAIL: hmount"; exit 1; }
echo "  + hmount successful"

echo "[3] Create directory..."
$HFSUTIL hmkdir testdir_plus >/dev/null 2>&1 || { echo "FAIL: hmkdir"; exit 1; }
echo "  + hmkdir works on HFS+"

echo "[4] Copy file in..."
echo "HFS+ test" > "$TMP/testfile_plus.txt"
$HFSUTIL hcopy "$TMP/testfile_plus.txt" :testfile_plus.txt >/dev/null 2>&1 || { echo "FAIL: hcopy"; exit 1; }
echo "  + hcopy works on HFS+"

echo "[5] Copy file out..."
$HFSUTIL hcopy :testfile_plus.txt "$TMP/retrieved_plus.txt" >/dev/null 2>&1 || { echo "FAIL: hcopy out"; exit 1; }
diff "$TMP/testfile_plus.txt" "$TMP/retrieved_plus.txt" >/dev/null 2>&1 || { echo "FAIL: content"; exit 1; }
echo "  + File content intact on HFS+"

echo "[6] Unmount..."
$HFSUTIL humount >/dev/null 2>&1 || { echo "FAIL: humount"; exit 1; }
echo "  + humount successful"

echo "+ HFS+ operations complete"
echo ""

#
# TEST 3: Version Info
#
echo "=== Test 3: Version Info ==="
$HFSUTIL --version >/dev/null 2>&1 || { echo "FAIL: version"; exit 1; }
echo "+ Version info available"
echo ""

echo "========================================="
echo "+ All hfsutil tests passed"
echo "  - HFS volume operations"
echo "  - HFS+ volume operations"
echo "  - Directory creation (mkdir)"
echo "  - File listing (ls)"
echo "  - File copy in/out (hcopy)"
echo "  - Content integrity verified"
echo "========================================="
