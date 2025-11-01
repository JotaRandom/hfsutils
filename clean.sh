#!/bin/bash
# Complete cleanup script for HFS utilities

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
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

echo "=== HFS Utilities Cleanup Script ==="
echo

# Parse arguments
CLEAN_ALL=false
CLEAN_CONFIG=false
CLEAN_BUILD=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --all)
            CLEAN_ALL=true
            shift
            ;;
        --config)
            CLEAN_CONFIG=true
            shift
            ;;
        --build)
            CLEAN_BUILD=true
            shift
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo
            echo "Options:"
            echo "  --all       Clean everything (build + config)"
            echo "  --build     Clean only build artifacts"
            echo "  --config    Clean only configuration files"
            echo "  --help      Show this help"
            echo
            echo "Examples:"
            echo "  $0                # Clean build artifacts only"
            echo "  $0 --all          # Clean everything"
            echo "  $0 --config       # Clean configuration only"
            exit 0
            ;;
        *)
            log_warning "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Default to cleaning build artifacts
if [[ "$CLEAN_ALL" == "false" && "$CLEAN_CONFIG" == "false" && "$CLEAN_BUILD" == "false" ]]; then
    CLEAN_BUILD=true
fi

if [[ "$CLEAN_ALL" == "true" ]]; then
    CLEAN_BUILD=true
    CLEAN_CONFIG=true
fi

# Clean build artifacts
if [[ "$CLEAN_BUILD" == "true" ]]; then
    log_info "Cleaning build artifacts..."
    
    # Standalone build artifacts
    if [[ -d build/standalone ]]; then
        rm -rf build/standalone
        log_success "Removed build/standalone/"
    fi
    
    # Main build artifacts
    if [[ -d build ]]; then
        rm -rf build
        log_success "Removed build/"
    fi
    
    # Object files
    find . -name "*.o" -type f -delete 2>/dev/null || true
    find . -name "*.a" -type f -delete 2>/dev/null || true
    
    # Temporary files
    rm -f conftest conftest.c conftest.o 2>/dev/null || true
    rm -f /tmp/test_hfs*.img 2>/dev/null || true
    
    # Makefile targets
    if [[ -f Makefile.standalone ]]; then
        make -f Makefile.standalone clean 2>/dev/null || true
    fi
    
    if [[ -f Makefile ]] && ! grep -q "HFS Utilities for Apple Silicon" Makefile 2>/dev/null; then
        make clean 2>/dev/null || true
    fi
    
    log_success "Build artifacts cleaned"
fi

# Clean configuration files
if [[ "$CLEAN_CONFIG" == "true" ]]; then
    log_info "Cleaning configuration files..."
    
    # Configuration files
    rm -f config.mk config.log config.status 2>/dev/null || true
    rm -rf autom4te.cache 2>/dev/null || true
    
    # Backup files
    find . -name "*~" -type f -delete 2>/dev/null || true
    find . -name "*.bak" -type f -delete 2>/dev/null || true
    find . -name ".#*" -type f -delete 2>/dev/null || true
    
    log_success "Configuration files cleaned"
fi

# Show what remains
echo
log_info "Cleanup completed!"

if [[ -d build ]]; then
    log_warning "Some build artifacts remain:"
    ls -la build/ 2>/dev/null || true
fi

echo
echo "To rebuild:"
echo "  ./configure.standalone && make -f Makefile.standalone"
echo "  # or"
echo "  ./build.standalone.sh"
echo
echo "To clean everything:"
echo "  $0 --all"