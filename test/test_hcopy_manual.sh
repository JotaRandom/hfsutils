#!/bin/bash
set -x

TEST_DIR="test/temp_test_manual"
mkdir -p "$TEST_DIR/source"
echo 'test' > "$TEST_DIR/source/file.txt"
dd if=/dev/zero of="$TEST_DIR/vol.hfs" bs=1M count=5 2>/dev/null
./hformat -l Test "$TEST_DIR/vol.hfs"
./hmount "$TEST_DIR/vol.hfs"
./hcopy -R "$TEST_DIR/source" :
EXIT_CODE=$?
echo "Exit code: $EXIT_CODE"
./hls :
./humount
rm -rf "$TEST_DIR"
