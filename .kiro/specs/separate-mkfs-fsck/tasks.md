# Implementation Plan

- [x] 1. Set up project structure for standalone utilities
  - Create new directory structure for mkfs, fsck, and mount components
  - Set up build directories and organization for embedded libraries
  - Create initial Makefile targets for standalone builds
  - _Requirements: 3.1, 3.2, 3.3, 6.1_

- [ ] 2. Extract and prepare libhfs subset for embedding
  - [x] 2.1 Identify minimal libhfs dependencies for mkfs.hfs
    - Analyze hformat.c dependencies and create dependency map
    - Extract required libhfs source files for filesystem creation
    - Create embedded libhfs subset with only formatting functions
    - _Requirements: 1.2, 1.3_

  - [x] 2.2 Identify minimal libhfs dependencies for fsck.hfs
    - Analyze hfsck dependencies and create dependency map
    - Extract required libhfs source files for filesystem checking
    - Create embedded libhfs subset with checking and repair functions
    - _Requirements: 2.2, 2.3_

  - [x] 2.3 Identify minimal libhfs dependencies for mount.hfs
    - Analyze hmount dependencies and create dependency map
    - Extract required libhfs source files for filesystem mounting
    - Create embedded libhfs subset with mounting and volume access functions
    - _Requirements: 5.2, 5.3_

  - [x] 2.4 Create common embedded utilities
    - Extract suid privilege management code
    - Create device detection and partitioning utilities
    - Implement error handling and version information modules
    - _Requirements: 1.3, 2.3, 5.3, 7.4_

- [ ] 3. Implement mkfs.hfs standalone utility
  - [x] 3.1 Create mkfs.hfs main entry point
    - Implement main() function with standard mkfs command-line parsing
    - Add program name detection for mkfs.hfs, mkfs.hfs+, mkfs.hfsplus
    - Implement standard mkfs exit codes and error handling
    - _Requirements: 1.1, 1.4, 1.5, 4.1_

  - [x] 3.2 Implement HFS formatting functionality
    - Port hformat HFS creation logic to standalone function
    - Integrate embedded libhfs for HFS volume creation
    - Implement device validation and partition handling
    - _Requirements: 1.2, 4.2, 5.4_

  - [x] 3.3 Implement HFS+ formatting functionality
    - Port hformat HFS+ creation logic to standalone function
    - Add HFS+ volume header creation and journaling setup
    - Implement block size optimization and volume sizing
    - _Requirements: 1.2, 4.2, 5.4_

  - [x] 3.4 Add command-line interface compatibility
    - Implement all original hformat command-line options
    - Add standard mkfs options (-f, -l, -v)
    - Ensure backward compatibility with existing scripts
    - _Requirements: 4.1, 4.4, 5.3_

- [ ] 4. Implement fsck.hfs standalone utility
  - [x] 4.1 Create fsck.hfs main entry point
    - Implement main() function with standard fsck command-line parsing
    - Add program name detection for fsck.hfs, fsck.hfs+, fsck.hfsplus
    - Implement standard fsck exit codes and error handling
    - _Requirements: 2.1, 2.4, 2.5, 4.1_

  - [x] 4.2 Enhance HFS checking functionality
    - Complete the hfsck() function implementation with full volume validation
    - Add comprehensive B-tree checking and repair capabilities
    - Implement allocation bitmap validation and repair
    - Add catalog file consistency checking
    - _Requirements: 2.2, 4.2, 5.4_

  - [x] 4.3 Complete HFS+ checking and journal support (linux's default hfsplus module doesn't support journaling, so is up to us what to do)
    - Enhance journal replay functionality with better error handling
    - Add comprehensive HFS+ volume header validation
    - Implement HFS+ catalog file checking with Unicode support
    - Add attributes file checking for extended attributes
    - _Requirements: 2.2, 4.2, 5.4_

  - [x] 4.4 Add command-line interface compatibility


    - Implement all original hfsck command-line options
    - Add standard fsck options (-v, -n, -a, -f, -y)
    - Ensure backward compatibility with existing scripts
    - Test if building both tools for foth filesystem work and fix any build issue
    - _Requirements: 4.1, 4.4, 7.3_

- [ ] 5. Implement mount.hfs standalone utility
  - [ ] 5.1 Create mount.hfs main entry point
    - Implement main() function with standard mount command-line parsing
    - Add program name detection for mount.hfs, mount.hfs+, mount.hfsplus
    - Implement standard mount exit codes and error handling
    - _Requirements: 5.1, 5.4, 5.5, 4.1_

  - [ ] 5.2 Enhance HFS mounting functionality
    - Complete the hfs_mount_volume() function with proper mount point handling
    - Add system mount integration (fstab, /proc/mounts)
    - Implement proper mount/unmount lifecycle management
    - Add mount option parsing and validation
    - _Requirements: 5.2, 4.2, 7.4_

  - [ ] 5.3 Implement HFS+ mounting functionality
    - Add HFS+ specific mounting logic with journal awareness (Linux's default hfsplus module doesn't support journaling, so is up to us what to do)
    - Implement HFS+ volume access with proper Unicode handling
    - Add read-only and read-write mount options with journal considerations
    - Implement case-sensitive vs case-insensitive mount detection (check HFS+ documentation)
    - _Requirements: 5.2, 4.2, 7.4_

  - [ ] 5.4 Add command-line interface compatibility
    - Implement all original hmount command-line options
    - Add standard mount options (-r, -v, -o)
    - Ensure backward compatibility with existing scripts
    - _Requirements: 4.1, 4.4, 7.3_

- [ ] 6. Complete build system for standalone utilities
  - [x] 6.1 Complete Makefile targets for mkfs.hfs
    - Replace placeholder mkfs.hfs target with actual compilation
    - Link mkfs.hfs with embedded libraries and libhfs
    - Add proper dependency management for mkfs.hfs build
    - _Requirements: 3.1, 3.2, 3.4_

  - [x] 6.2 Implement Makefile targets for fsck.hfs
    - Create build target that compiles fsck.hfs with embedded dependencies
    - Set up proper include paths and library linking
    - Implement clean and install targets for fsck.hfs
    - _Requirements: 3.1, 3.2, 3.4_

  - [ ] 6.3 Implement Makefile targets for mount.hfs, mount.hfsplus and mount.hfs+
    - Create build target that compiles mount.hfs, mount.hfsplus and mount.hfs+ with embedded dependencies
    - Set up proper include paths and library linking
    - Implement clean and install targets for mount.hfs, mount.hfsplus and mount.hfs+
    - _Requirements: 3.1, 3.2, 3.4_

  - [ ] 6.4 Complete flexible installation targets
    - Replace placeholder install-linux target with actual installation
    - Replace placeholder install-other target with actual installation
    - Replace placeholder install-complete target with actual installation
    - Replace placeholder install target with actuall installation
    - Create symbolic links for all utility variants if the proper install-* is summoned
    - _Requirements: 6.1, 6.2, 6.3, 6.4, 7.5_

- [ ] 7. Implement filesystem type detection and validation
  - [ ] 7.1 Create HFS and HFS+ detection utilities
    - Implement filesystem signature detection
    - Add volume header validation for both HFS and HFS+
    - Create device and partition validation functions
    - _Requirements: 1.2, 2.2, 5.2, 7.4_

  - [ ] 7.2 Add program name-based type enforcement
    - Implement automatic filesystem type selection based on program name
    - Add validation to ensure correct filesystem type for specific utilities
    - Implement error reporting for type mismatches
    - _Requirements: 1.4, 2.4, 5.4, 4.1_

- [ ] 8. Add comprehensive error handling and reporting
  - [ ] 8.1 Implement standard exit codes
    - Add proper exit code handling for all error conditions in mkfs.hfs
    - Add proper exit code handling for all error conditions in fsck.hfs
    - Add proper exit code handling for all error conditions in mount.hfs
    - Ensure compliance with Unix/Linux filesystem utility standards
    - Ensure somehow compatibility with FreeBSD
    - _Requirements: 1.5, 2.5, 5.5, 7.3_

  - [ ] 8.2 Add verbose mode and user feedback
    - Implement detailed progress reporting for formatting operations
    - Add comprehensive diagnostic output for checking operations
    - Add informative output for mounting operations
    - Create user-friendly error messages and suggestions
    - _Requirements: 4.1, 4.2, 7.3_

- [ ] 9. Create comprehensive test suite
  - [x] 9.1 Write unit tests for mkfs.hfs functionality
    - Test HFS and HFS+ formatting with various parameters
    - Test command-line parsing and error handling
    - Test device validation and partition handling
    - _Requirements: 1.1, 1.2, 4.2_

  - [ ] 9.2 Write unit tests for fsck.hfs functionality
    - Test HFS and HFS+ checking and repair operations
    - Test journal replay and corruption handling
    - Test command-line parsing and error handling
    - _Requirements: 2.1, 2.2, 4.2_

  - [ ] 9.3 Write unit tests for mount.hfs functionality
    - Test HFS mounting with various options
    - Test command-line parsing and error handling
    - Test mount point validation and permissions
    - _Requirements: 5.1, 5.2, 4.2_

  - [ ] 9.3.1 Write unit tests for mount.hfs+ functionality
    - Test HFS+ mounting with various options
    - Test command-line parsing and error handling
    - Test mount point validation and permissions
    - _Requirements: 5.1, 5.2, 4.2_

  - [ ] 9.4 Create integration tests for cross-platform compatibility
    - Test utilities on different Linux distributions
    - Test utilities on BSD systems
    - Verify compatibility with system mount/fsck tools
    - Test flexible installation targets
    - _Requirements: 6.1, 6.2, 6.3, 7.4_

- [ ] 10. Update build system and configuration files
  - [ ] 10.1 Update main Makefile with new targets
    - Add build targets for mkfs.hfs, fsck.hfs, and mount.hfs
    - Implement install-linux, install-other, and install targets
    - Update clean and distclean targets to handle new utilities
    - Add proper dependency management for standalone utilities
    - _Requirements: 6.1, 6.2, 6.3, 6.4_

  - [ ] 10.2 Update configure script and autotools configuration
    - Modify configure.ac to detect dependencies for standalone utilities
    - Update config.h.in with new feature flags
    - Add checks for mount-specific system calls and headers
    - Ensure compatibility with different Unix/Linux distributions
    - Ensure somehow compatibility with FreeBSD
    - _Requirements: 7.4, 6.4_

  - [ ] 10.3 Update .gitignore files
    - Add new build artifacts for mkfs.hfs, fsck.hfs, mount.hfs
    - Include new object files and intermediate build products
    - Add patterns for new test outputs and temporary files
    - _Requirements: 3.4_

- [ ] 11. Update test infrastructure
  - [ ] 11.1 Extend test suite in test/ directory
    - Create test scripts for mkfs.hfs functionality
    - Create test scripts for fsck.hfs functionality  
    - Create test scripts for mount.hfs functionality
    - Add integration tests for flexible installation targets
    - _Requirements: 9.1, 9.2, 9.3, 9.3.1, 9.4_

  - [ ] 11.2 Update test data generation
    - Modify generate_test_data.sh to create test images for all utilities
    - Add test cases for different filesystem sizes and configurations
    - Create corrupted filesystem images for fsck testing
    - Add mount/unmount test scenarios
    - _Requirements: 9.1, 9.2, 9.3, 9.3.1_

  - [ ] 11.3 Update test execution framework
    - Modify run_tests.sh to execute tests for all standalone utilities
    - Add performance benchmarks for formatting and checking operations
    - Implement cross-platform test execution (Linux distributions)
    - Add test result reporting and validation
    - _Requirements: 9.4_

- [ ] 12. Update GitHub Actions and CI/CD
  - [ ] 12.1 Update GitHub workflow files
    - Modify .github/workflows/ to test standalone utilities
    - Add matrix builds for different Linux distributions
    - Include tests for all installation targets (install-linux, install-other, install, etc.)
    - Add artifact publishing for standalone binaries
    - _Requirements: 9.4, 6.4_

  - [ ] 12.2 Add automated testing for WSL compatibility
    - Create specific test jobs for Windows Subsystem for Linux
    - Test utilities on Ubuntu LTS, Debian old-stable, Arch, and other WSL distributions
    - Validate filesystem operations on WSL-mounted drives
    - Add regression testing for WSL-specific issues
    - _Requirements: 9.4_

- [ ] 13. Create comprehensive documentation
  - [ ] 13.1 Create manual pages for standalone utilities
    - Write comprehensive man pages for mkfs.hfs (section 8)
    - Write comprehensive man pages for mkfs.hfs+ (section 8)
    - Update symlinks for mkfs.hfs+ and mkfs.hfsplus manpages
    - Write comprehensive man pages for fsck.hfs (section 8)
    - Write comprehensive man pages for fsck.hfs+ (section 8)
    - Update symlinks for fsck.hfs+ and fsck.hfsplus manpages
    - Write comprehensive man pages for mount.hfs (section 8)
    - Write comprehensive man pages for mount.hfs+ (section 8)
    - Update symlinks for mount.hfs+ and mount.hfsplus manpages
    - Include examples, exit codes, and compatibility information
    - _Requirements: 7.2, 4.4_

  - [ ] 13.2 Update project documentation
    - Update README.md with standalone utility information
    - Document flexible installation options and use cases
    - Add platform-specific installation instructions
    - Include troubleshooting guide for common issues
    - _Requirements: 3.4, 6.4, 7.5_

  - [ ] 13.3 Update CHANGELOG.md
    - Document addition of standalone utilities (mkfs.hfs, fsck.hfs, mount.hfs)
    - Record new flexible installation system
    - Note breaking changes and migration path from hfsutils
    - Add version information and release notes
    - _Requirements: 6.4, 7.5_

  - [ ] 13.4 Update packaging documentation
    - Update PACKAGING.md with new installation targets
    - Update BUILD.md with new instructions
    - Update CHANGELOG
    - Add distribution-specific packaging examples
    - Document WSL-specific considerations and testing
    - Include dependency information for standalone utilities
    - _Requirements: 6.4, 7.5_

  - [ ] 13.5 Clear and organize
    - Check the entire project foldering for problems
    - Remove redundancy files and update ALL building systems
    - Check version inside programs and if needed update them to 4.1.0B
    - Update .gitignore a last time
    - Update all remaining files
    - Remove unneded, redundant, obsolete files and directories
    - Run a last make test
    - Run a last full test for all applications
    - Run a last make clean and check all is clean
    - Upload to github with a new checkout as 4.1.0B
    - _Requierements: 13.4_
