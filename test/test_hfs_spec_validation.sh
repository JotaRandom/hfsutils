#!/bin/bash
# Test script to validate HFS/HFS+ specification conformance
# Tests alternate header locations and Volume Header field values

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "========================================="
echo "  HFS/HFS+ Specification Validation Test"
echo "========================================="
echo ""

# Cleanup
cleanup() {
    rm -f test_hfs_spec.img test_hfsplus_spec.img
}
trap cleanup EXIT

# Test 1: HFS Classic Volume
echo -e "${BLUE}[TEST 1]${NC} HFS Classic Specification Conformance"
echo "Creating 10MB HFS volume..."
dd if=/dev/zero of=test_hfs_spec.img bs=1M count=10 2>/dev/null
./build/standalone/mkfs.hfs -l "TestHFS" test_hfs_spec.img >/dev/null 2>&1

FILE_SIZE=$(stat -c%s test_hfs_spec.img)
ALTERNATE_MDB_OFFSET=$((FILE_SIZE - 1024))

echo -e "${BLUE}[INFO]${NC} File size: $FILE_SIZE bytes"
echo -e "${BLUE}[INFO]${NC} Alternate MDB should be at offset: $ALTERNATE_MDB_OFFSET (file_size - 1024)"

# Check primary MDB at offset 1024
echo -e "${BLUE}[CHECK]${NC} Primary MDB signature at offset 1024..."
PRIMARY_SIG=$(xxd -s 1024 -l 2 -p test_hfs_spec.img)
if [ "$PRIMARY_SIG" = "4244" ]; then
    echo -e "${GREEN}[PASS]${NC} Primary MDB signature: 0x4244 ('BD')"
else
    echo -e "${RED}[FAIL]${NC} Primary MDB signature: 0x$PRIMARY_SIG (expected 0x4244)"
fi

# Check alternate MDB at file_size - 1024
echo -e "${BLUE}[CHECK]${NC} Alternate MDB signature at offset $ALTERNATE_MDB_OFFSET..."
ALT_SIG=$(xxd -s $ALTERNATE_MDB_OFFSET -l 2 -p test_hfs_spec.img)
if [ "$ALT_SIG" = "4244" ]; then
    echo -e "${GREEN}[PASS]${NC} Alternate MDB signature: 0x4244 ('BD') at correct offset"
else
    echo -e "${RED}[FAIL]${NC} Alternate MDB signature: 0x$ALT_SIG at offset $ALTERNATE_MDB_OFFSET (expected 0x4244)"
fi

# Check drAtrb field (offset 1024 + 4 = 1028)
echo -e "${BLUE}[CHECK]${NC} HFS drAtrb (attributes) at offset 1028..."
DRATRB=$(xxd -s 1028 -l 2 -p test_hfs_spec.img)
if [ "$DRATRB" = "0100" ]; then
    echo -e "${GREEN}[PASS]${NC} drAtrb: 0x0100 (unmounted bit set)"
else
    echo -e "${YELLOW}[WARN]${NC} drAtrb: 0x$DRATRB (expected 0x0100)"
fi

# Check drNxtCNID (offset 1024 + 84 = 1108)
echo -e "${BLUE}[CHECK]${NC} HFS drNxtCNID (next catalog ID) at offset 1108..."
DRNXTCNID=$(xxd -s 1108 -l 4 -p test_hfs_spec.img)
if [ "$DRNXTCNID" = "00000010" ]; then
    echo -e "${GREEN}[PASS]${NC} drNxtCNID: 0x00000010 (16 decimal)"
else
    echo -e "${YELLOW}[WARN]${NC} drNxtCNID: 0x$DRNXTCNID (expected 0x00000010)"
fi

echo ""

# Test 2: HFS+ Volume
echo -e "${BLUE}[TEST 2]${NC} HFS+ Specification Conformance"
echo "Creating 10MB HFS+ volume..."
dd if=/dev/zero of=test_hfsplus_spec.img bs=1M count=10 2>/dev/null
./build/standalone/mkfs.hfs+ -l "TestHFSPlus" test_hfsplus_spec.img >/dev/null 2>&1

FILE_SIZE=$(stat -c%s test_hfsplus_spec.img)
ALTERNATE_VH_OFFSET=$((FILE_SIZE - 1024))

echo -e "${BLUE}[INFO]${NC} File size: $FILE_SIZE bytes"
echo -e "${BLUE}[INFO]${NC} Alternate Volume Header should be at offset: $ALTERNATE_VH_OFFSET (file_size - 1024)"

# Check primary Volume Header at offset 1024
echo -e "${BLUE}[CHECK]${NC} Primary Volume Header signature at offset 1024..."
PRIMARY_SIG=$(xxd -s 1024 -l 2 -p test_hfsplus_spec.img)
if [ "$PRIMARY_SIG" = "482b" ]; then
    echo -e "${GREEN}[PASS]${NC} Primary VH signature: 0x482B ('H+')"
else
    echo -e "${RED}[FAIL]${NC} Primary VH signature: 0x$PRIMARY_SIG (expected 0x482B)"
fi

# Check alternate Volume Header at file_size - 1024
echo -e "${BLUE}[CHECK]${NC} Alternate Volume Header signature at offset $ALTERNATE_VH_OFFSET..."
ALT_SIG=$(xxd -s $ALTERNATE_VH_OFFSET -l 2 -p test_hfsplus_spec.img)
if [ "$ALT_SIG" = "482b" ]; then
    echo -e "${GREEN}[PASS]${NC} Alternate VH signature: 0x482B ('H+') at correct offset"
else
    echo -e "${RED}[FAIL]${NC} Alternate VH signature: 0x$ALT_SIG at offset $ALTERNATE_VH_OFFSET (expected 0x482B)"
fi

# Check attributes field (offset 1024 + 4 = 1028)
echo -e "${BLUE}[CHECK]${NC} HFS+ attributes at offset 1028..."
ATTRIBUTES=$(xxd -s 1028 -l 4 -p test_hfsplus_spec.img)
if [ "$ATTRIBUTES" = "00000100" ]; then
    echo -e "${GREEN}[PASS]${NC} attributes: 0x00000100 (unmounted bit set)"
elif [ "$ATTRIBUTES" = "00002100" ]; then
    echo -e "${GREEN}[PASS]${NC} attributes: 0x00002100 (unmounted + journaled bits set)"
else
    echo -e "${RED}[FAIL]${NC} attributes: 0x$ATTRIBUTES (expected 0x00000100 or 0x00002100)"
fi

# Check blockSize (offset 1024 + 40 = 1064)
echo -e "${BLUE}[CHECK]${NC} HFS+ blockSize at offset 1064..."
BLOCKSIZE=$(xxd -s 1064 -l 4 -p test_hfsplus_spec.img)
echo -e "${BLUE}[INFO]${NC} blockSize: 0x$BLOCKSIZE"

# Check rsrcClumpSize (offset 1024 + 56 = 1080)
echo -e "${BLUE}[CHECK]${NC} HFS+ rsrcClumpSize at offset 1080..."
RSRC_CLUMP=$(xxd -s 1080 -l 4 -p test_hfsplus_spec.img)
if [ "$RSRC_CLUMP" != "00000000" ]; then
    echo -e "${GREEN}[PASS]${NC} rsrcClumpSize: 0x$RSRC_CLUMP (non-zero)"
else
    echo -e "${RED}[FAIL]${NC} rsrcClumpSize: 0x$RSRC_CLUMP (should be non-zero)"
fi

# Check dataClumpSize (offset 1024 + 60 = 1084)
echo -e "${BLUE}[CHECK]${NC} HFS+ dataClumpSize at offset 1084..."
DATA_CLUMP=$(xxd -s 1084 -l 4 -p test_hfsplus_spec.img)
if [ "$DATA_CLUMP" != "00000000" ]; then
    echo -e "${GREEN}[PASS]${NC} dataClumpSize: 0x$DATA_CLUMP (non-zero)"
else
    echo -e "${RED}[FAIL]${NC} dataClumpSize: 0x$DATA_CLUMP (should be non-zero)"
fi

# Check nextCatalogID (offset 1024 + 64 = 1088)
echo -e "${BLUE}[CHECK]${NC} HFS+ nextCatalogID at offset 1088..."
NEXT_CNID=$(xxd -s 1088 -l 4 -p test_hfsplus_spec.img)
if [ "$NEXT_CNID" = "00000010" ]; then
    echo -e "${GREEN}[PASS]${NC} nextCatalogID: 0x00000010 (16 decimal)"
else
    echo -e "${RED}[FAIL]${NC} nextCatalogID: 0x$NEXT_CNID (expected 0x00000010)"
fi

echo ""
echo "========================================="
echo "  Validation Summary"
echo "========================================="
echo ""
echo "Per Apple TN1150 and Wikipedia documentation:"
echo "  - Alternate headers MUST be at file_size - 1024 bytes"
echo "  - HFS signature: 0x4244 ('BD')"
echo "  - HFS+ signature: 0x482B ('H+') or 0x4858 ('HX' for HFSX)"
echo "  - nextCatalogID >= 16 (kHFSFirstUserCatalogNodeID)"
echo "  - attributes bit 8 set (0x0100 = kHFSVolumeUnmountedBit)"
echo "  - rsrcClumpSize and dataClumpSize must be non-zero"
echo ""
