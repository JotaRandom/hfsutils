#!/bin/bash

#
# Simple Integration Test for HFS Utilities
# Demonstrates basic inter-utility workflows
#

set -e

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "========================================="
echo "  Simple HFS Utilities Integration Test"
echo "========================================="
echo

# Change to test directory
cd "$(dirname "$0")"

# Setup paths
UTILS_DIR=".."
TEST_IMG="test_integration.hfs"

echo "1. Creating HFS volume..."
dd if=/dev/zero of="$TEST_IMG" bs=1024 count=2048 2>/dev/null
$UTILS_DIR/hformat -l "Integration Test" "$TEST_IMG"
echo -e "${GREEN}✓${NC} Volume created"

echo
echo "2. Mounting volume..."
$UTILS_DIR/hmount "$TEST_IMG"
echo -e "${GREEN}✓${NC} Volume mounted"

echo
echo "3. Creating directory structure..."
$UTILS_DIR/hmkdir :Documents
$UTILS_DIR/hmkdir :Documents:Projects
$UTILS_DIR/hmkdir :Software
echo -e "${GREEN}✓${NC} Directories created"

echo
echo "4. Creating test files..."
echo "Hello from HFS utilities!" > test_file.txt
echo "Project documentation" > project.txt
echo "#include <stdio.h>" > hello.c

echo
echo "5. Copying files to volume..."
$UTILS_DIR/hcopy test_file.txt :Documents:readme.txt
$UTILS_DIR/hcopy project.txt :Documents:Projects:project.txt
$UTILS_DIR/hcopy hello.c :Software:hello.c
echo -e "${GREEN}✓${NC} Files copied to volume"

echo
echo "6. Listing volume contents..."
echo "Root directory:"
$UTILS_DIR/hls
echo
echo "Documents directory:"
$UTILS_DIR/hls :Documents
echo
echo "Documents/Projects directory:"
$UTILS_DIR/hls :Documents:Projects
echo -e "${GREEN}✓${NC} Directory listings shown"

echo
echo "7. Testing navigation..."
$UTILS_DIR/hcd :Documents
echo "Current directory: $($UTILS_DIR/hpwd)"
$UTILS_DIR/hcd :Projects
echo "Current directory: $($UTILS_DIR/hpwd)"
echo -e "${GREEN}✓${NC} Navigation works"

echo
echo "8. Copying file back from volume..."
# We're in Documents:Projects, go back to Documents first
$UTILS_DIR/hcd ::
$UTILS_DIR/hcopy readme.txt retrieved_file.txt
if [ -f retrieved_file.txt ]; then
    echo "Retrieved content: $(cat retrieved_file.txt)"
    echo -e "${GREEN}✓${NC} File retrieved successfully"
else
    echo -e "${RED}✗${NC} Failed to retrieve file"
fi

echo
echo "9. Testing file operations..."
# Go back to root (omit argument to go to root)
$UTILS_DIR/hcd
# Now we should be at root
$UTILS_DIR/hrename :Software:hello.c :Software:main.c
echo "After rename:"
$UTILS_DIR/hls :Software
echo -e "${GREEN}✓${NC} File renamed"

echo
echo "10. Testing deletion..."
$UTILS_DIR/hdel :Software:main.c
echo "After deletion:"
$UTILS_DIR/hls :Software
$UTILS_DIR/hrmdir :Software
echo "After removing empty directory:"
$UTILS_DIR/hls
echo -e "${GREEN}✓${NC} Deletion works"

echo
echo "11. Volume information..."
$UTILS_DIR/hvol
echo -e "${GREEN}✓${NC} Volume info displayed"

echo
echo "12. Unmounting volume..."
$UTILS_DIR/humount
echo -e "${GREEN}✓${NC} Volume unmounted"

echo
echo "13. Cleanup..."
rm -f test_file.txt project.txt hello.c retrieved_file.txt "$TEST_IMG"
echo -e "${GREEN}✓${NC} Cleanup complete"

echo
echo "========================================="
echo -e "${GREEN}All integration tests passed!${NC}"
echo "========================================="