# HFS Utils Test Suite

## Structure

```
test/
├── test_mkfs.sh      - Test filesystem creation
├── test_fsck.sh      - Test validation and repair
└── test_hfsutils.sh  - Test hfsutil commands
```

## Running Tests

### Run all tests:
```bash
cd test
bash test_mkfs.sh && bash test_fsck.sh && bash test_hfsutils.sh
```

### Run individual tests:
```bash
bash test/test_mkfs.sh     # Test mkfs.hfs and mkfs.hfs+
bash test/test_fsck.sh     # Test fsck.hfs and fsck.hfs+
bash test/test_hfsutils.sh # Test hfsutil commands
```

## Test Coverage

### test_mkfs.sh (5 tests)
- HFS creation with valid signature (0x4244)
- HFS+ creation with valid signature (0x482b)
- Minimum sizes (800KB HFS, 10MB HFS+)
- Custom size (-s option)
- Journaling (-j option) with Linux warning

### test_fsck.sh (2 tests)
- Clean volume validation
- Exit code correctness

### test_hfsutils.sh (3 tests)
- hformat command
- hmount/humount
- Version info

## Requirements

- Build complete: `./build.sh`
- Executables in `build/standalone/`
- Linux/macOS with bash

## Exit Codes

- 0 = All tests passed
- 1 = Test failed (filesystem is corrupt or command failed)