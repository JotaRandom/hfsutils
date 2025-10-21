# HFS Utilities Test Suite

Comprehensive testing framework for Apple Silicon HFS utilities.

## Quick Start

```bash
# Generate test data (HFS images and sample files)
./generate_test_data.sh

# Run all tests
./run_tests.sh

# Run specific test categories
./run_tests.sh basic        # Basic functionality tests
./run_tests.sh integration  # Integration workflow tests  
./run_tests.sh errors       # Error handling tests

# Simple integration demonstration
./simple_integration_test.sh
```

## Test Structure

### 1. Test Data Generator (`generate_test_data.sh`)
Creates various HFS and HFS+ images and sample files for testing:
- **Small images** (1.44MB): Basic testing with simple file structure (HFS and HFS+)
- **Medium images** (10MB): More complex directory hierarchies (HFS and HFS+)
- **Large images** (50MB): Stress testing with many files (HFS and HFS+)
- **Empty images**: Testing empty volume operations (HFS and HFS+)
- **Corrupted image**: Error handling tests

### 2. Main Test Runner (`run_tests.sh`)
Comprehensive test harness with multiple test categories:

#### Basic Functionality Tests
- Utility existence checks
- Help/version output
- Mount/unmount operations
- Directory listing and navigation
- File operations (copy, delete, rename)
- Directory operations (create, remove)
- Volume information display

#### Integration Workflow Tests
- **Backup/Restore**: Complete backup and restoration workflow
- **Archive Creation**: Organizing files by type
- **Volume Migration**: Moving data between volumes
- **File Management**: Complex directory structures
- **Data Recovery**: Simulated recovery scenarios

#### Error Handling Tests
- Operations without mounted volumes
- Non-existent file/directory access
- Invalid operations (e.g., deleting directories with hdel)
- Edge cases (special characters, empty files, deep paths)

#### HFS+ Specific Tests
- HFS+ volume formatting with hformat -t hfs+ and mkfs.hfs+
- Filesystem type detection (HFS vs HFS+)
- Program name detection (mkfs.hfs, mkfs.hfs+, fsck.hfs+)
- HFS+ volume information and validation
- Mixed HFS/HFS+ operations and compatibility

### 3. Simple Integration Test (`simple_integration_test.sh`)
A straightforward demonstration that:
1. Creates and formats an HFS volume
2. Creates directory structure
3. Copies files to/from the volume
4. Tests navigation and file operations
5. Verifies all utilities work together

## Test Options

```bash
./run_tests.sh [OPTIONS] [TEST_PATTERN]

Options:
  -v, --verbose     Enable verbose output
  -q, --quick       Skip stress tests
  -k, --keep        Keep test files for inspection
  -h, --help        Show help message

Test patterns:
  all              Run all tests (default)
  basic            Basic functionality only
  integration      Integration workflows only
  errors           Error handling only
  hfsplus          HFS+ specific tests only
```

## Test Results

The test suite validates:
- ✅ All utilities compile and run on Apple Silicon
- ✅ Basic HFS operations (mount, copy, delete, rename)
- ✅ HFS+ volume creation and formatting
- ✅ Filesystem type detection and program name recognition
- ✅ Directory navigation and management
- ✅ Inter-utility workflows and data pipelines
- ✅ Error handling and edge cases
- ✅ Volume information and management
- ✅ Mixed HFS/HFS+ environment compatibility

## Known Issues

1. **MacBinary Detection**: Binary files may trigger MacBinary format detection
2. **Path Navigation**: `hcd` changes affect relative paths in subsequent commands
3. **Test Isolation**: Some tests may leave mounted volumes on failure

## Future Enhancements

- [ ] Performance benchmarking
- [ ] Stress testing with large file counts
- [ ] Concurrent operation testing
- [ ] Extended attribute preservation tests
- [ ] Cross-volume operation tests

## Contributing

When adding new tests:
1. Add test data generation to `generate_test_data.sh`
2. Implement test function in `run_tests.sh`
3. Update appropriate test category in the case statement
4. Document any new test patterns in this README

## Requirements

- Bash 3.2+
- Standard Unix utilities (dd, grep, etc.)
- Compiled HFS utilities in parent directory