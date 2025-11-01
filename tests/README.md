# HFS/HFS+ Utilities Test Suite

This directory contains tests for the HFS/HFS+ filesystem utilities.

## Test Structure

```
tests/
├── unit/           # Unit tests for individual components
├── integration/    # Integration tests for complete workflows
├── fixtures/       # Test data and sample filesystems
├── scripts/        # Test automation scripts
└── README.md       # This file
```

## Running Tests

### Unit Tests
```bash
make test-unit
```

### Integration Tests
```bash
make test-integration
```

### All Tests
```bash
make test
```

## Test Categories

### Unit Tests
- `unit/mkfs/` - Tests for mkfs.hfs and mkfs.hfs+ functionality
- `unit/fsck/` - Tests for fsck.hfs and fsck.hfs+ functionality
- `unit/shared/` - Tests for shared utility functions

### Integration Tests
- `integration/mkfs/` - End-to-end mkfs testing
- `integration/fsck/` - End-to-end fsck testing
- `integration/workflows/` - Complete filesystem workflows

### Test Fixtures
- `fixtures/images/` - Sample HFS/HFS+ filesystem images
- `fixtures/data/` - Test data files
- `fixtures/configs/` - Test configuration files

## Test Requirements

- Linux system with loop device support
- Root privileges for some tests (filesystem creation/mounting)
- At least 100MB free disk space for test images

## Writing Tests

Tests should follow these conventions:
- Use descriptive test names
- Include both positive and negative test cases
- Clean up resources after test completion
- Document expected behavior and edge cases