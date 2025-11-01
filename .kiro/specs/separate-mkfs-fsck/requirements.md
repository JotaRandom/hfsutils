# Requirements Document

## Introduction

This specification defines the separation of `hformat` and `hfsck` utilities from the hfsutils suite into independent, standards-compliant `mkfs.hfs` and `fsck.hfs` tools. These tools will follow Unix/Linux filesystem utility conventions and operate independently without requiring the full hfsutils installation.

## Glossary

- **mkfs.hfs**: A standalone filesystem creation utility for HFS/HFS+ filesystems, derived from hformat
- **fsck.hfs**: A standalone filesystem checking and repair utility for HFS/HFS+ filesystems, derived from hfsck
- **mount.hfs**: A standalone filesystem mounting utility for HFS/HFS+ filesystems, derived from hmount
- **hfsutils**: The original suite of HFS utilities that currently contains hformat, hfsck, and hmount
- **libhfs**: The core HFS library providing filesystem operations
- **System**: The combined mkfs.hfs, fsck.hfs, and mount.hfs utilities and their supporting infrastructure

## Requirements

### Requirement 1

**User Story:** As a system administrator, I want to install mkfs.hfs independently, so that I can create HFS filesystems without installing the entire hfsutils suite.

#### Acceptance Criteria

1. THE System SHALL provide mkfs.hfs as a standalone executable that does not require hfsutils installation
2. WHEN mkfs.hfs is invoked, THE System SHALL create HFS/HFS+ filesystems using only its embedded dependencies
3. THE System SHALL include all necessary library code within mkfs.hfs to eliminate external hfsutils dependencies
4. THE System SHALL follow standard mkfs naming conventions and exit codes as defined by Unix/Linux systems
5. WHERE mkfs.hfs encounters errors, THE System SHALL return appropriate exit codes (0=success, 1=general error, 2=usage error, 4=operational error, 8=system error)

### Requirement 2

**User Story:** As a system administrator, I want to install fsck.hfs independently, so that I can check and repair HFS filesystems without installing the entire hfsutils suite.

#### Acceptance Criteria

1. THE System SHALL provide fsck.hfs as a standalone executable that does not require hfsutils installation
2. WHEN fsck.hfs is invoked, THE System SHALL check and repair HFS/HFS+ filesystems using only its embedded dependencies
3. THE System SHALL include all necessary library code within fsck.hfs to eliminate external hfsutils dependencies
4. THE System SHALL follow standard fsck naming conventions and exit codes as defined by Unix/Linux systems
5. WHERE fsck.hfs encounters different conditions, THE System SHALL return appropriate exit codes (0=no errors, 1=corrected errors, 2=reboot required, 4=uncorrected errors, 8=operational error, 16=usage error, 32=cancelled, 128=library error)

### Requirement 3

**User Story:** As a package maintainer, I want separate build targets for mkfs.hfs and fsck.hfs, so that I can package them independently from hfsutils.

#### Acceptance Criteria

1. THE System SHALL provide independent Makefile targets for building mkfs.hfs and fsck.hfs
2. WHEN building mkfs.hfs, THE System SHALL compile only the necessary source files and dependencies
3. WHEN building fsck.hfs, THE System SHALL compile only the necessary source files and dependencies
4. THE System SHALL allow building mkfs.hfs and fsck.hfs without building the complete hfsutils suite
5. THE System SHALL maintain compatibility with existing autotools configuration while adding new build targets

### Requirement 4

**User Story:** As a developer, I want the separated utilities to maintain API compatibility, so that existing scripts and applications continue to work.

#### Acceptance Criteria

1. THE System SHALL preserve all command-line options and arguments from the original hformat and hfsck utilities
2. WHEN mkfs.hfs is invoked with hformat-compatible arguments, THE System SHALL produce identical filesystem output
3. WHEN fsck.hfs is invoked with hfsck-compatible arguments, THE System SHALL produce identical checking and repair behavior
4. THE System SHALL maintain backward compatibility with existing shell scripts and automation tools
5. THE System SHALL provide symbolic links or aliases to support legacy command names if requested

### Requirement 5

**User Story:** As a system administrator, I want to install mount.hfs independently, so that I can mount HFS filesystems without installing the entire hfsutils suite.

#### Acceptance Criteria

1. THE System SHALL provide mount.hfs as a standalone executable that does not require hfsutils installation
2. WHEN mount.hfs is invoked, THE System SHALL mount HFS/HFS+ filesystems using only its embedded dependencies
3. THE System SHALL include all necessary library code within mount.hfs to eliminate external hfsutils dependencies
4. THE System SHALL follow standard mount utility naming conventions and behavior
5. WHERE mount.hfs encounters errors, THE System SHALL return appropriate exit codes and error messages

### Requirement 6

**User Story:** As a package maintainer, I want flexible installation targets, so that I can create different packages for different use cases (Linux vs other systems).

#### Acceptance Criteria

1. THE System SHALL provide install-linux target that installs only mkfs.hfs, fsck.hfs, and mount.hfs
2. THE System SHALL provide install-other target that installs mkfs.hfs, fsck.hfs, and complete hfsutils suite
3. THE System SHALL provide install target that installs all utilities (mkfs.hfs, fsck.hfs, mount.hfs, and hfsutils)
4. THE System SHALL allow selective installation based on target platform requirements
5. WHERE different installation targets are used, THE System SHALL maintain proper dependencies and avoid conflicts

### Requirement 7

**User Story:** As a system integrator, I want the separated utilities to follow filesystem utility conventions, so that they integrate properly with system tools and package managers.

#### Acceptance Criteria

1. THE System SHALL install mkfs.hfs, fsck.hfs, and mount.hfs in the standard system location (/sbin or /usr/sbin)
2. THE System SHALL provide manual pages for mkfs.hfs, fsck.hfs, and mount.hfs
3. THE System SHALL follow standard Unix filesystem utility naming and behavior conventions
4. THE System SHALL ensure utilities are discoverable by mount, fsck, and other system tools
5. WHERE the utilities are installed, THE System SHALL create appropriate symbolic links for variant names