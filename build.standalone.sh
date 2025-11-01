#!/bin/bash
# Quick build script for standalone HFS utilities

set -e

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() {
    echo -e "${BLUE}INFO:${NC} $1"
}

log_success() {
    echo -e "${GREEN}SUCCESS:${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}WARNING:${NC} $1"
}

echo "=== Standalone HFS Utilities Build Script ==="
echo

# Check if we need to configure
if [[ ! -f config.mk ]]; then
    log_info "No configuration found, running configure..."
    ./configure.standalone
    echo
fi

# Load configuration if it exists
if [[ -f config.mk ]]; then
    log_info "Configuration found"
fi

# Use Makefile.standalone if regular Makefile doesn't exist or is the original
if [[ ! -f Makefile ]] || grep -q "HFS Utilities for Apple Silicon" Makefile 2>/dev/null; then
    log_info "Using standalone Makefile..."
    MAKEFILE="Makefile.standalone"
else
    MAKEFILE="Makefile"
fi

# Build
log_info "Building standalone utilities..."
make -f "$MAKEFILE" clean
if [[ "$1" == "fast" ]] || [[ "$1" == "--fast" ]]; then
    log_info "Using fast build (no optimization)..."
    make -f "$MAKEFILE" fast
else
    make -f "$MAKEFILE" simple
fi

echo
log_success "Build completed!"

# Run tests if requested
if [[ "$1" == "test" ]] || [[ "$1" == "--test" ]]; then
    echo
    log_info "Running tests..."
    make -f "$MAKEFILE" test
fi

# Show what was built
echo
echo "Built utilities:"
ls -la build/standalone/mkfs.hfs* 2>/dev/null || log_warning "No utilities found in build/standalone/"

echo
echo "Usage:"
echo "  ./build/standalone/mkfs.hfs --help"
echo "  ./build/standalone/mkfs.hfs -v -l 'Test' /tmp/test.img"
echo
echo "To install:"
echo "  sudo make -f $MAKEFILE install"
echo
echo "To clean:"
echo "  make -f $MAKEFILE clean"