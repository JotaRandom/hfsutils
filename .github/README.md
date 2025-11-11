# GitHub Workflows for hfsutils

This directory contains GitHub Actions workflows for continuous integration and testing.

## Workflows

### CI Workflow (`.github/workflows/ci.yml`)

Comprehensive continuous integration workflow that builds and tests hfsutils on multiple platforms.

#### Jobs

1. **build-and-test-ubuntu**
   - Platform: Ubuntu Latest
   - Builds complete project using `build.sh`
   - Creates symlinks for filesystem utilities
   - Runs comprehensive test suite:
     - Basic functionality tests
     - Integration workflow tests
     - HFS+ specific tests
     - Error handling tests
     - Specification conformance tests
   - Tests installation to verify package structure
   - Uploads artifacts on failure for debugging

2. **build-and-test-archlinux**
   - Platform: Arch Linux (container)
   - Same test suite as Ubuntu
   - Validates compatibility with rolling-release distribution
   - Uses latest Arch Linux packages
   - Independent artifact upload for debugging

3. **test-specification-conformance**
   - Platform: Ubuntu Latest
   - Runs after successful Ubuntu build
   - Validates HFS/HFS+ specification compliance:
     - **HFS Alternate MDB Location**: Verifies signature `0x4244` at `device_size - 1024`
     - **HFS+ Alternate Volume Header**: Verifies signature `0x482B` or `0x4244` at `device_size - 1024`
     - **HFS+ attributes Field**: Validates `kHFSVolumeUnmountedBit` (0x0100) is set
   - Uses `hexdump` to inspect binary structures
   - Ensures conformance to Apple TN1150 specification

4. **cross-platform-compatibility**
   - Platform: Ubuntu Latest
   - Runs after all builds complete
   - Summary job to verify all platforms passed
   - Provides single checkpoint for PR approval

#### Test Categories

The test suite (`test/run_tests.sh`) includes:

- **basic**: Core functionality (mount, ls, copy, etc.)
- **integration**: Real-world workflows (backup, migration, archive)
- **hfsplus**: HFS+ specific features (formatting, detection, journaling)
- **errors**: Error handling and edge cases
- **all**: Complete test suite

#### Triggers

- **Push**: Any push to `master` branch
- **Pull Request**: Any PR targeting `master` branch

#### Dependencies

**Ubuntu:**
- build-essential
- gcc
- make
- perl
- hexdump
- util-linux

**Arch Linux:**
- base-devel
- gcc
- make
- perl
- git
- util-linux

## Running Tests Locally

### Ubuntu/Debian
```bash
sudo apt-get install build-essential gcc make perl hexdump
./build.sh
cd test
./run_tests.sh all
```

### Arch Linux
```bash
sudo pacman -S base-devel gcc make perl util-linux
./build.sh
cd test
./run_tests.sh all
```

### Quick Tests
```bash
cd test
./run_tests.sh basic  # Basic functionality only
```

### Specification Tests
```bash
cd test
./test_hfs_spec_validation.sh
./test_hfsplus_complete.sh
```

## Artifacts

On test failure, the following artifacts are uploaded:

- `test/temp/` - Temporary test files and images
- `test/*.log` - Test execution logs
- `*.img` - HFS/HFS+ disk images created during tests

Artifacts are kept for 90 days and can be downloaded from the Actions tab.

## Status Badges

Add to README.md:

```markdown
![CI](https://github.com/JotaRandom/hfsutils/actions/workflows/ci.yml/badge.svg)
```

## Maintenance

### Adding New Tests

1. Add test function to `test/run_tests.sh`
2. Update workflow to include new test category if needed
3. Test locally before pushing

### Updating Dependencies

1. Update package lists in workflow file
2. Test in container locally:
   ```bash
   docker run -it --rm -v $(pwd):/workspace -w /workspace ubuntu:latest bash
   # or
   docker run -it --rm -v $(pwd):/workspace -w /workspace archlinux:latest bash
   ```

### Debugging Failures

1. Check Actions tab for logs
2. Download artifacts if available
3. Reproduce locally using same commands
4. Check specification conformance with `hexdump`

## References

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Apple TN1150 - HFS Plus Volume Format](../doc/TN1150_HFS_PLUS_VOLUME_FORMAT.md)
- [Test Suite Documentation](../test/README.md)
