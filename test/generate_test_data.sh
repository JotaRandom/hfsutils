#!/bin/bash

#
# Generate Test Data for HFS Utilities
# Creates various HFS images and sample files for testing
#

set -e

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

# Directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DATA_DIR="$SCRIPT_DIR/data"
TEMP_DIR="$SCRIPT_DIR/temp"

# Utility paths - using unified binary
HFSUTIL="$PROJECT_ROOT/hfsutil"
HFSCK="$PROJECT_ROOT/hfsck/hfsck"

# Create symlinks for compatibility if they don't exist
if [ ! -L "$PROJECT_ROOT/hformat" ]; then
    ln -sf hfsutil "$PROJECT_ROOT/hformat"
fi
if [ ! -L "$PROJECT_ROOT/hmount" ]; then
    ln -sf hfsutil "$PROJECT_ROOT/hmount"
fi
if [ ! -L "$PROJECT_ROOT/humount" ]; then
    ln -sf hfsutil "$PROJECT_ROOT/humount"
fi
if [ ! -L "$PROJECT_ROOT/hcopy" ]; then
    ln -sf hfsutil "$PROJECT_ROOT/hcopy"
fi
if [ ! -L "$PROJECT_ROOT/hmkdir" ]; then
    ln -sf hfsutil "$PROJECT_ROOT/hmkdir"
fi
if [ ! -L "$PROJECT_ROOT/hls" ]; then
    ln -sf hfsutil "$PROJECT_ROOT/hls"
fi

HFORMAT="$PROJECT_ROOT/hformat"
HMOUNT="$PROJECT_ROOT/hmount"
HUMOUNT="$PROJECT_ROOT/humount"
HCOPY="$PROJECT_ROOT/hcopy"
HMKDIR="$PROJECT_ROOT/hmkdir"
HLS="$PROJECT_ROOT/hls"

log() {
    echo -e "${BLUE}[INFO]${NC} $*"
}

success() {
    echo -e "${GREEN}[DONE]${NC} $*"
}

# Create directories
mkdir -p "$DATA_DIR"
mkdir -p "$TEMP_DIR"

log "Generating test data in $DATA_DIR"

#==============================================================================
# Sample Files for Testing
#==============================================================================

log "Creating sample files..."

# Create various test files
cd "$TEMP_DIR"

# Small text file
cat > small_text.txt << 'EOF'
This is a small text file for testing HFS utilities.
It contains some basic ASCII text.
Created by the HFS utilities test suite.

Line with special characters: !@#$%^&*()
Line with numbers: 1234567890
EOF

# Medium text file (around 8KB)
{
    echo "This is a medium-sized text file for testing."
    echo "Generated on: $(date)"
    echo
    for i in {1..100}; do
        echo "Line $i: The quick brown fox jumps over the lazy dog. Pack my box with five dozen liquor jugs."
    done
} > medium_text.txt

# Large text file (around 64KB)  
{
    echo "This is a large text file for testing."
    echo "Generated on: $(date)"
    echo
    for i in {1..1000}; do
        echo "Line $i: Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
    done
} > large_text.txt

# Binary file (1KB of pseudo-random data)
if command -v openssl >/dev/null; then
    openssl rand 1024 > binary_1k.dat
else
    dd if=/dev/urandom of=binary_1k.dat bs=1024 count=1 2>/dev/null
fi

# Binary file (16KB)
if command -v openssl >/dev/null; then
    openssl rand 16384 > binary_16k.dat
else
    dd if=/dev/urandom of=binary_16k.dat bs=16384 count=1 2>/dev/null
fi

# Empty file
touch empty_file.txt

# File with special characters in name
echo "File with special characters" > "test file (with spaces & symbols).txt"

# Files with different extensions
echo "C source file" > test.c
echo "Header file" > test.h
echo "Text document" > document.txt
echo "README content" > README.md

success "Sample files created"

#==============================================================================
# Test HFS Images
#==============================================================================

cd "$DATA_DIR"

# Clean existing test images
rm -f *.hfs *.img

log "Creating test HFS and HFS+ images..."

#------------------------------------------------------------------------------
# 1. Small HFS image (1.44MB floppy disk size)
#------------------------------------------------------------------------------
log "Creating small HFS test image (1440KB)..."

# Create empty file first, then format it as HFS
dd if=/dev/zero of=small_test.hfs bs=1024 count=1440 2>/dev/null
"$HFORMAT" -l "Test Floppy" small_test.hfs
"$HMOUNT" small_test.hfs

# Populate with basic files  
"$HCOPY" "$TEMP_DIR/small_text.txt" :readme.txt
"$HCOPY" "$TEMP_DIR/test.c" :test.c
"$HCOPY" "$TEMP_DIR/empty_file.txt" :empty.txt
"$HMKDIR" :docs
"$HCOPY" "$TEMP_DIR/medium_text.txt" :docs:manual.txt

"$HUMOUNT"
success "Small HFS test image created: small_test.hfs"

#------------------------------------------------------------------------------
# 1b. Small HFS+ image (1.44MB floppy disk size)
#------------------------------------------------------------------------------
log "Creating small HFS+ test image (1440KB)..."

# Create empty file first, then format it as HFS+
dd if=/dev/zero of=small_test_hfsplus.img bs=1024 count=1440 2>/dev/null
"$HFORMAT" -t hfs+ -l "Test Floppy Plus" small_test_hfsplus.img

# Note: HFS+ volumes cannot be mounted with the old HFS library
# This image is for testing HFS+ formatting and fsck functionality
success "Small HFS+ test image created: small_test_hfsplus.img"

#------------------------------------------------------------------------------
# 2. Medium HFS image (10MB)
#------------------------------------------------------------------------------
log "Creating medium HFS test image (10MB)..."

# Create 10MB file
dd if=/dev/zero of=medium_test.hfs bs=1024 count=10240 2>/dev/null
"$HFORMAT" -l "Medium Test" medium_test.hfs
"$HMOUNT" medium_test.hfs

# Create directory structure
"$HMKDIR" :Documents
"$HMKDIR" :Pictures  
"$HMKDIR" :Software
"$HMKDIR" :Archives
"$HMKDIR" :Documents:Projects
"$HMKDIR" :Documents:Notes

# Populate with various files
"$HCOPY" "$TEMP_DIR/small_text.txt" :Documents:readme.txt
"$HCOPY" "$TEMP_DIR/medium_text.txt" :Documents:manual.txt  
"$HCOPY" "$TEMP_DIR/large_text.txt" :Documents:Notes:notes.txt
"$HCOPY" "$TEMP_DIR/binary_1k.dat" :Archives:data1.bin
"$HCOPY" "$TEMP_DIR/binary_16k.dat" :Archives:data16.bin
"$HCOPY" "$TEMP_DIR/test.c" :Software:hello.c
"$HCOPY" "$TEMP_DIR/test.h" :Software:hello.h

# File with special characters
"$HCOPY" "$TEMP_DIR/test file (with spaces & symbols).txt" ":Documents:special file.txt"

"$HUMOUNT"
success "Medium HFS test image created: medium_test.hfs"

#------------------------------------------------------------------------------
# 2b. Medium HFS+ image (10MB)
#------------------------------------------------------------------------------
log "Creating medium HFS+ test image (10MB)..."

# Create 10MB file and format as HFS+
dd if=/dev/zero of=medium_test_hfsplus.img bs=1024 count=10240 2>/dev/null
"$HFORMAT" -t hfs+ -l "Medium Test Plus" medium_test_hfsplus.img

# Note: HFS+ volumes cannot be mounted with the old HFS library
# This image is for testing HFS+ formatting and fsck functionality
success "Medium HFS+ test image created: medium_test_hfsplus.img"

#------------------------------------------------------------------------------
# 3. Empty images for testing
#------------------------------------------------------------------------------
log "Creating empty HFS test image (1440KB)..."

# Create empty file and format it as HFS
dd if=/dev/zero of=empty_test.hfs bs=1024 count=1440 2>/dev/null
"$HFORMAT" -l "Empty Test" empty_test.hfs
success "Empty HFS test image created: empty_test.hfs"

log "Creating empty HFS+ test image (1440KB)..."

# Create empty file and format it as HFS+
dd if=/dev/zero of=empty_test_hfsplus.img bs=1024 count=1440 2>/dev/null
"$HFORMAT" -t hfs+ -l "Empty Test Plus" empty_test_hfsplus.img
success "Empty HFS+ test image created: empty_test_hfsplus.img"

#------------------------------------------------------------------------------
# 4. Large image (50MB) for stress testing
#------------------------------------------------------------------------------
log "Creating large test image (50MB)..."

# Create 50MB file
dd if=/dev/zero of=large_test.hfs bs=1024 count=51200 2>/dev/null
"$HFORMAT" -l "Large Test Volume" large_test.hfs
"$HMOUNT" large_test.hfs

# Create deeper directory structure
for dir in System Applications Documents Music Pictures Movies; do
    "$HMKDIR" ":$dir"
done

"$HMKDIR" :System:Configuration
"$HMKDIR" :System:Logs
"$HMKDIR" :Applications:Utilities
"$HMKDIR" :Documents:Work
"$HMKDIR" :Documents:Personal

# Add more files
for i in {1..20}; do
    echo "Test file $i content for stress testing" > "$TEMP_DIR/test_file_$i.txt"
    "$HCOPY" "$TEMP_DIR/test_file_$i.txt" ":Documents:test_file_$i.txt"
done

# Add some binary files
"$HCOPY" "$TEMP_DIR/binary_1k.dat" :System:config.dat
"$HCOPY" "$TEMP_DIR/binary_16k.dat" :Applications:app_data.bin

"$HUMOUNT"
success "Large HFS test image created: large_test.hfs"

#------------------------------------------------------------------------------
# 4b. Large HFS+ image (50MB) for stress testing
#------------------------------------------------------------------------------
log "Creating large HFS+ test image (50MB)..."

# Create 50MB file and format as HFS+
dd if=/dev/zero of=large_test_hfsplus.img bs=1024 count=51200 2>/dev/null
"$HFORMAT" -t hfs+ -l "Large Test Plus" large_test_hfsplus.img

# Note: HFS+ volumes cannot be mounted with the old HFS library
# This image is for testing HFS+ formatting and fsck functionality
success "Large HFS+ test image created: large_test_hfsplus.img"

#==============================================================================
# Test image with known corruption (for error testing)
#==============================================================================
log "Creating corrupted test image..."

# Start with a good image
cp small_test.hfs corrupted_test.hfs

# Corrupt some bytes in the middle of the file (be careful not to corrupt critical structures)
if command -v dd >/dev/null; then
    # Overwrite some bytes in the middle with zeros
    dd if=/dev/zero of=corrupted_test.hfs bs=1 seek=50000 count=100 conv=notrunc 2>/dev/null
fi

success "Corrupted test image created: corrupted_test.hfs"

#==============================================================================
# Create reference files for testing
#==============================================================================
log "Creating reference files..."

# Create expected output files for testing
mkdir -p reference

# Expected directory listing for small_test.hfs
cat > reference/small_test_ls.txt << 'EOF'
d   0      0 Jan  1  1904 Jan  1  1904 docs
f   0     56 Jan  1  1904 Jan  1  1904 empty.txt
f   0    133 Jan  1  1904 Jan  1  1904 readme.txt
f   0     13 Jan  1  1904 Jan  1  1904 test.c
EOF

success "Reference files created"

#==============================================================================
# Summary
#==============================================================================
echo
log "Test data generation complete!"
echo
echo "Created HFS images:"
ls -lh "$DATA_DIR"/*.hfs 2>/dev/null | while read -r line; do
    echo "  $line"
done

echo
echo "Created HFS+ images:"
ls -lh "$DATA_DIR"/*.img 2>/dev/null | while read -r line; do
    echo "  $line"
done

echo
echo "Sample files available in: $TEMP_DIR"
echo "Test images available in: $DATA_DIR" 
echo "Reference data available in: $DATA_DIR/reference"

# Cleanup temp directory
rm -rf "$TEMP_DIR"

success "Test data ready for use"