# HFS Utilities for Apple Silicon
# Builds unified hfsutil binary with all utilities

# Installation directories
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
SBINDIR ?= $(PREFIX)/sbin
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include
MANDIR ?= $(PREFIX)/share/man
DESTDIR ?=

# Modern systems compatibility
# Many distributions have merged /sbin into /bin, use SBINDIR to control this
# For packaging: make install PREFIX=/usr SBINDIR=/usr/bin
# For merged systems: make install SBINDIR=$(BINDIR)

# Build tools and flags
CC ?= gcc
CXX ?= g++
CFLAGS ?= -g -O2
CXXFLAGS ?= -g -O2

# Internal flags (always applied)
INTERNAL_CFLAGS = -Wall -Werror -I. -I./include -I./include/common -I./include/hfsutil -I./include/binhex -I./libhfs -I./librsrc
INTERNAL_LDFLAGS = -L./libhfs -L./librsrc
LIBS = -lhfs -lrsrc

# Combined flags
ALL_CFLAGS = $(CFLAGS) $(INTERNAL_CFLAGS)
ALL_LDFLAGS = $(LDFLAGS) $(INTERNAL_LDFLAGS)

# Build directory for object files
BUILDDIR = build
OBJDIR = $(BUILDDIR)/obj

# Ensure build directories exist
$(shell mkdir -p $(OBJDIR))

# Executables (symlinks to hfsutil)
EXECUTABLES = hattrib hcd hcopy hdel hformat hls hmkdir hmount hpwd hrename hrmdir humount hvol

# Filesystem utility symlinks (only .hfsplus variants are symlinks)
# mkfs.hfs and mkfs.hfs+ are separate binaries
# fsck.hfs and fsck.hfs+ are separate binaries
FSCK_LINKS = fsck.hfsplus
MKFS_LINKS = mkfs.hfsplus
MOUNT_LINKS = mount.hfsplus

# Standalone utilities (separate binaries, not symlinks)
STANDALONE_UTILITIES = mkfs.hfs fsck.hfs mount.hfs

# Default target - just build hfsutil without symlinks
all: libhfs librsrc hfsck hfsutil

# Standalone utilities target
standalone: $(STANDALONE_UTILITIES)

# Alternative target that avoids autotools completely
build-manual: libhfs librsrc hfsck-manual hfsutil

# Set targets - build complete toolsets
.PHONY: set-hfs set-hfsplus

# HFS toolset: mkfs.hfs, fsck.hfs, mount.hfs
set-hfs: standalone
	@echo "Building HFS toolset (mkfs.hfs, fsck.hfs, mount.hfs)..."
	$(MAKE) -C src/mkfs mkfs.hfs
	$(MAKE) -C src/fsck fsck.hfs  
	$(MAKE) -C src/mount mount.hfs
	@echo "HFS toolset complete"

# HFS+ toolset: mkfs.hfs+, fsck.hfs+, mount.hfs+ (with .hfsplus symlinks)
set-hfsplus: standalone
	@echo "Building HFS+ toolset (mkfs.hfs+, fsck.hfs+, mount.hfs+)..."
	$(MAKE) -C src/mkfs mkfs.hfs+
	$(MAKE) -C src/fsck fsck.hfs+
	$(MAKE) -C src/mount mount.hfs+
	@echo "Creating .hfsplus symlinks..."
	cd build/standalone && ln -sf mkfs.hfs+ mkfs.hfsplus
	cd build/standalone && ln -sf fsck.hfs+ fsck.hfsplus
	cd build/standalone && ln -sf mount.hfs+ mount.hfsplus
	@echo "HFS+ toolset complete"


# Target to create symlinks for backward compatibility
symlinks: hfsutil $(EXECUTABLES) hdir $(MKFS_LINKS) $(FSCK_LINKS)

# Build libraries
libhfs:
	$(MAKE) -C libhfs CC="$(CC)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" PREFIX="$(PREFIX)" DESTDIR="$(DESTDIR)"

librsrc:
	$(MAKE) -C librsrc CC="$(CC)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" PREFIX="$(PREFIX)" DESTDIR="$(DESTDIR)"

hfsck:
	@echo "Building hfsck..."
	@if $(MAKE) -C hfsck CC="$(CC)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" PREFIX="$(PREFIX)" SBINDIR="$(SBINDIR)" DESTDIR="$(DESTDIR)" 2>/dev/null; then \
		echo "hfsck built successfully with autotools"; \
	else \
		echo "Autotools build failed, building hfsck manually with journaling support..."; \
		cd hfsck && \
		$(CC) $(CFLAGS) -I./../include -I./../include/common -I./../libhfs -I./../src/common -DHAVE_CONFIG_H -c *.c && \
		$(CC) $(CFLAGS) -I./../include -I./../include/common -I./../libhfs -I./../src/common -DHAVE_CONFIG_H -c ../src/common/suid.c -o suid.o && \
		$(CC) $(CFLAGS) -I./../include -I./../include/common -I./../libhfs -I./../src/common -DHAVE_CONFIG_H -c ../src/common/version.c -o version.o && \
		$(CC) $(CFLAGS) -I./../include -I./../include/common -I./../libhfs -I./../src/common -DHAVE_CONFIG_H -c ../src/common/hfs_detect.c -o hfs_detect.o && \
		$(CC) $(CFLAGS) -o hfsck ck_btree.o ck_mdb.o ck_volume.o hfsck.o main.o util.o journal.o suid.o version.o hfs_detect.o ./../libhfs/libhfs.a && \
		echo "hfsck built successfully with manual compilation"; \
	fi

hfsck-manual:
	@echo "Building hfsck manually (skipping autotools)..."
	@cd hfsck && \
	$(CC) $(CFLAGS) -I./../include -I./../include/common -I./../libhfs -I./../src/common -DHAVE_CONFIG_H -c *.c && \
	$(CC) $(CFLAGS) -I./../include -I./../include/common -I./../libhfs -I./../src/common -DHAVE_CONFIG_H -c ../src/common/suid.c -o suid.o && \
	$(CC) $(CFLAGS) -I./../include -I./../include/common -I./../libhfs -I./../src/common -DHAVE_CONFIG_H -c ../src/common/version.c -o version.o && \
	$(CC) $(CFLAGS) -I./../include -I./../include/common -I./../libhfs -I./../src/common -DHAVE_CONFIG_H -c ../src/common/hfs_detect.c -o hfs_detect.o && \
	$(CC) $(CFLAGS) -o hfsck ck_btree.o ck_mdb.o ck_volume.o hfsck.o main.o util.o journal.o suid.o version.o hfs_detect.o ./../libhfs/libhfs.a
	@echo "hfsck built successfully with manual compilation"

# Object files in build directory
$(OBJDIR)/hcwd.o: src/common/hcwd.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/suid.o: src/common/suid.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/glob.o: src/common/glob.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/version.o: src/common/version.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/charset.o: src/common/charset.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/hfs_detect.o: src/common/hfs_detect.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/hfsplus_format.o: src/common/hfsplus_format.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/binhex.o: src/binhex/binhex.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/copyin.o: src/common/copyin.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/copyout.o: src/common/copyout.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/crc.o: src/common/crc.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/darray.o: src/common/darray.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/dlist.o: src/common/dlist.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/dstring.o: src/common/dstring.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

# Utility object files
$(OBJDIR)/hattrib.o: src/hfsutil/hattrib.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/hcd.o: src/hfsutil/hcd.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/hcopy.o: src/hfsutil/hcopy.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/hdel.o: src/hfsutil/hdel.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/hformat.o: src/hfsutil/hformat.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/hls.o: src/hfsutil/hls.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/hmkdir.o: src/hfsutil/hmkdir.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/hmount.o: src/hfsutil/hmount.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/hpwd.o: src/hfsutil/hpwd.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/hrename.o: src/hfsutil/hrename.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/hrmdir.o: src/hfsutil/hrmdir.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/humount.o: src/hfsutil/humount.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/hvol.o: src/hfsutil/hvol.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR)/hfsutil.o: src/hfsutil/hfsutil.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

# Common objects
COMMON_OBJS = $(OBJDIR)/hcwd.o $(OBJDIR)/suid.o $(OBJDIR)/glob.o \
              $(OBJDIR)/version.o $(OBJDIR)/charset.o $(OBJDIR)/binhex.o \
              $(OBJDIR)/hfs_detect.o $(OBJDIR)/hfsplus_format.o

# All utility objects  
UTIL_OBJS = $(OBJDIR)/hattrib.o $(OBJDIR)/hcd.o $(OBJDIR)/hcopy.o \
            $(OBJDIR)/hdel.o $(OBJDIR)/hformat.o $(OBJDIR)/hls.o \
            $(OBJDIR)/hmkdir.o $(OBJDIR)/hmount.o $(OBJDIR)/hpwd.o \
            $(OBJDIR)/hrename.o $(OBJDIR)/hrmdir.o $(OBJDIR)/humount.o \
            $(OBJDIR)/hvol.o $(OBJDIR)/hfsutil.o $(OBJDIR)/copyin.o \
            $(OBJDIR)/copyout.o $(OBJDIR)/crc.o $(OBJDIR)/darray.o \
            $(OBJDIR)/dlist.o $(OBJDIR)/dstring.o

# Build unified binary
hfsutil: libhfs librsrc $(UTIL_OBJS) $(COMMON_OBJS)
	$(CC) $(ALL_LDFLAGS) $(UTIL_OBJS) $(COMMON_OBJS) $(LIBS) -o $@

# Create symlinks for individual commands
$(EXECUTABLES): hfsutil
	ln -sf hfsutil $@

hdir: hfsutil
	ln -sf hfsutil hdir

# Create filesystem utility symlinks (disabled - using standalone utilities now)
# $(MKFS_LINKS): hfsutil
#	ln -sf hfsutil $@

# $(FSCK_LINKS): hfsck
#	ln -sf hfsck/hfsck $@

clean:
	@echo "Cleaning executables and build artifacts..."
	@for f in $(EXECUTABLES) hdir hfsutil $(MKFS_LINKS) $(FSCK_LINKS); do \
		if [ -L $$f ] || [ -f $$f ]; then \
			rm -f $$f; \
			echo " - removed $$f"; \
		else \
			echo " - skipping $$f (not a file or symlink)"; \
		fi; \
	done
	@rm -rf $(BUILDDIR)
	@$(MAKE) -C libhfs clean
	@$(MAKE) -C librsrc clean
	@$(MAKE) -C hfsck clean SBINDIR="$(SBINDIR)"

distclean: clean
	rm -f config.h config.log config.status
	rm -rf autom4te.cache
	$(MAKE) -C libhfs distclean
	$(MAKE) -C librsrc distclean
	$(MAKE) -C hfsck distclean SBINDIR="$(SBINDIR)"

install: all install-libs
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(SBINDIR)
	install -d $(DESTDIR)$(MANDIR)/man1
	install -d $(DESTDIR)$(MANDIR)/man8
	install -m 755 hfsutil $(DESTDIR)$(BINDIR)/
	# Install manual pages (section 1)
	for man in doc/man/*.1; do \
		install -m 644 $$man $(DESTDIR)$(MANDIR)/man1/; \
	done
	# Install manual pages (section 8)
	for man in doc/man/*.8; do \
		install -m 644 $$man $(DESTDIR)$(MANDIR)/man8/; \
	done
	@echo "Installed hfsutil to $(DESTDIR)$(BINDIR)/"
	@echo "Installed manual pages to $(DESTDIR)$(MANDIR)/man1/ and $(DESTDIR)$(MANDIR)/man8/"
	@echo "System utilities will be installed to $(DESTDIR)$(SBINDIR)/"
	@echo "You can optionally create symlinks with 'make install-symlinks'"

install-libs:
	$(MAKE) -C libhfs install CC="$(CC)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" PREFIX="$(PREFIX)" DESTDIR="$(DESTDIR)"
	$(MAKE) -C librsrc install CC="$(CC)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" PREFIX="$(PREFIX)" DESTDIR="$(DESTDIR)"
	@if $(MAKE) -C hfsck install CC="$(CC)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" PREFIX="$(PREFIX)" SBINDIR="$(SBINDIR)" DESTDIR="$(DESTDIR)" 2>/dev/null; then \
		echo "hfsck installed successfully"; \
	else \
		echo "Installing hfsck manually..."; \
		install -d "$(DESTDIR)$(SBINDIR)"; \
		install -m 755 hfsck/hfsck "$(DESTDIR)$(SBINDIR)/"; \
		echo "hfsck installed manually to $(DESTDIR)$(SBINDIR)/"; \
	fi

install-symlinks: install
	for prog in $(EXECUTABLES) hdir; do \
		ln -sf hfsutil $(DESTDIR)$(BINDIR)/$$prog; \
	done
	# Create .hfsplus symlink (fsck.hfsplus -> fsck.hfs+)
	@if [ "$(SBINDIR)" = "$(BINDIR)" ]; then \
		echo "Creating .hfsplus symlinks for merged /bin system..."; \
		ln -sf fsck.hfs+ $(DESTDIR)$(BINDIR)/fsck.hfsplus; \
	else \
		echo "Creating .hfsplus symlinks for separate /sbin system..."; \
		ln -sf fsck.hfs+ $(DESTDIR)$(SBINDIR)/fsck.hfsplus; \
	fi
	# Create mkfs.hfsplus symlink (mkfs.hfsplus -> mkfs.hfs+)
	ln -sf mkfs.hfs+ $(DESTDIR)$(SBINDIR)/mkfs.hfsplus
	@echo "Created symlinks for traditional command names"
	@echo "Created .hfsplus symlinks (fsck.hfsplus -> fsck.hfs+, mkfs.hfsplus -> mkfs.hfs+)"

# Old test target (disabled - using new test targets now)
# test: all
#	cd test && ./generate_test_data.sh && ./run_tests.sh

help:
	@echo "HFS Utilities for Apple Silicon - Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make              - Build hfsutil executable"
	@echo "  make build-manual - Build avoiding autotools (if configure issues)"
	@echo "  make symlinks     - Create command symlinks (optional)"
	@echo "  make clean        - Remove built files"
	@echo "  make distclean    - Remove all generated files"
	@echo "  make install      - Install hfsutil, libraries, and manual pages"
	@echo "  make install-symlinks - Install with traditional command names"
	@echo "  make test         - Run test suite"
	@echo "  make help         - Show this help"
	@echo ""
	@echo "Installation Variables:"
	@echo "  PREFIX=/path      - Installation prefix (default: /usr/local)"
	@echo "  DESTDIR=/path     - Staging directory for package building"
	@echo "  BINDIR=/path      - Binary installation directory (default: PREFIX/bin)"
	@echo "  SBINDIR=/path     - System binary directory (default: PREFIX/sbin)"
	@echo "  LIBDIR=/path      - Library installation directory"
	@echo "  MANDIR=/path      - Manual page installation directory"
	@echo ""
	@echo "Build Variables:"
	@echo "  CC=compiler       - C compiler (default: gcc)"
	@echo "  CXX=compiler      - C++ compiler (default: g++)"
	@echo "  CFLAGS='flags'    - C compiler flags"
	@echo "  CXXFLAGS='flags'  - C++ compiler flags"
	@echo "  LDFLAGS='flags'   - Linker flags"
	@echo ""
	@echo "Examples:"
	@echo "  make install PREFIX=/opt/hfsutils"
	@echo "  make install DESTDIR=/tmp/staging"
	@echo "  make CC=clang CFLAGS='-O3 -march=native'"
	@echo ""
	@echo "Distribution Packaging Examples:"
	@echo "  make install PREFIX=/usr DESTDIR=/tmp/pkg"
	@echo "  make install PREFIX=/usr SBINDIR=/usr/bin DESTDIR=/tmp/pkg  # Merged /bin systems"
	@echo "  make install-symlinks PREFIX=/usr SBINDIR=/usr/bin         # With symlinks"
	@echo ""
	@echo "Modern Systems Compatibility:"
	@echo "  Many distributions have merged /sbin into /bin. Use SBINDIR to control this:"
	@echo "  - Traditional systems: SBINDIR=/usr/sbin (default)"
	@echo "  - Merged systems: SBINDIR=/usr/bin"
	@echo "  - The build system automatically detects and creates appropriate symlinks"
	@echo ""
	@echo "The single 'hfsutil' binary contains all utilities."
	@echo "Usage: hfsutil <command> [options]"
	@echo "Run 'hfsutil' without arguments to see available commands."
	@echo ""
	@echo "Filesystem Utilities:"
	@echo "  hfsck           - Check/repair HFS and HFS+ filesystems"
	@echo "  hformat         - Format HFS and HFS+ filesystems"
	@echo ""
	@echo "Standard Names (via symlinks):"
	@echo "  fsck.hfs        - Check HFS filesystem"
	@echo "  fsck.hfs+       - Check HFS+ filesystem"
	@echo "  mkfs.hfs        - Create HFS filesystem"
	@echo "  mkfs.hfs+       - Create HFS+ filesystem"

# ============================================================================
# STANDALONE UTILITIES BUILD RULES
# ============================================================================

# Standalone mkfs.hfs utilities
mkfs.hfs: libhfs librsrc
	@echo "Building standalone mkfs.hfs utilities..."
	$(MAKE) -C src/mkfs all

# Standalone fsck.hfs utilities  
fsck.hfs: libhfs librsrc
	@echo "Building standalone fsck.hfs utilities..."
	$(MAKE) -C src/fsck all

# HFS+ utilities (mkfs.hfs+ and fsck.hfs+)
hfsplus: mkfs.hfs fsck.hfs
	@echo "HFS+ utilities built successfully"
	@echo "Available executables:"
	@echo "  - build/standalone/mkfs.hfs"
	@echo "  - build/standalone/mkfs.hfs+"
	@echo "  - build/standalone/fsck.hfs"
	@echo "  - build/standalone/fsck.hfs+"

# Standalone mount.hfs utility
mount.hfs: libhfs librsrc
	@echo "Building standalone mount.hfs utility..."
	@mkdir -p build/standalone
	@echo "Note: mount.hfs implementation will be added in subsequent tasks"
	@echo "Placeholder created for mount.hfs build target"

# ============================================================================
# TEST TARGETS
# ============================================================================

# Unit tests
test-unit: hfsplus
	@echo "Running unit tests..."
	@mkdir -p build/tests
	$(CC) $(CFLAGS) -I src -o build/tests/test_mkfs_common tests/unit/mkfs/test_mkfs_common.c src/mkfs/mkfs_common.c build/standalone/libshared.a -I src/embedded/shared
	build/tests/test_mkfs_common

# Integration tests
test-integration: hfsplus
	@echo "Running integration tests..."
	bash tests/integration/mkfs/test_mkfs_basic.sh

# All tests
test: test-unit test-integration
	@echo "All tests completed successfully!"

# ============================================================================
# FLEXIBLE INSTALLATION TARGETS
# ============================================================================

# Manpage directory
MAN8DIR = $(MANDIR)/man8

# ============================================================================
# INDIVIDUAL UTILITY INSTALL TARGETS
# ============================================================================

# mkfs utilities
install-mkfs.hfs: 
	@echo "Installing mkfs.hfs..."
	install -d $(DESTDIR)$(SBINDIR)
	install -d $(DESTDIR)$(MAN8DIR)
	install -m 755 build/standalone/mkfs.hfs $(DESTDIR)$(SBINDIR)/mkfs.hfs
	install -m 644 doc/man/mkfs.hfs.8 $(DESTDIR)$(MAN8DIR)/mkfs.hfs.8
	@echo "[OK] mkfs.hfs installed"
	@echo ""
	@echo "Additional options available:"
	@echo "  make install-mkfs.hfs+     # Install HFS+ version"
	@echo "  make install-fsck.hfs      # Install filesystem checker"
	@echo "  make install-mount.hfs     # Install mount utility"
	@echo "  make install-set-hfs       # Install complete HFS toolset"
	@echo ""

install-mkfs.hfs+:
	@echo "Installing mkfs.hfs+..."
	install -d $(DESTDIR)$(SBINDIR)
	install -d $(DESTDIR)$(MAN8DIR)
	install -m 755 build/standalone/mkfs.hfs+ $(DESTDIR)$(SBINDIR)/mkfs.hfs+
	install -m 644 doc/man/mkfs.hfs+.8 $(DESTDIR)$(MAN8DIR)/mkfs.hfs+.8
	@echo "[OK] mkfs.hfs+ installed"
	@echo ""
	@echo "Additional options available:"
	@echo "  make install-fsck.hfs+     # Install filesystem checker"
	@echo "  make install-mount.hfs+    # Install mount utility"
	@echo "  make install-set-hfsplus   # Install complete HFS+ toolset with .hfsplus symlinks"
	@echo ""

# fsck utilities
install-fsck.hfs:
	@echo "Installing fsck.hfs..."
	install -d $(DESTDIR)$(SBINDIR)
	install -d $(DESTDIR)$(MAN8DIR)
	install -m 755 build/standalone/fsck.hfs $(DESTDIR)$(SBINDIR)/fsck.hfs
	install -m 644 doc/man/fsck.hfs.8 $(DESTDIR)$(MAN8DIR)/fsck.hfs.8
	@echo "[OK] fsck.hfs installed"
	@echo ""
	@echo "Additional options available:"
	@echo "  make install-mkfs.hfs      # Install filesystem creator"
	@echo "  make install-mount.hfs     # Install mount utility"
	@echo "  make install-set-hfs       # Install complete HFS toolset"
	@echo ""

install-fsck.hfs+:
	@echo "Installing fsck.hfs+..."
	install -d $(DESTDIR)$(SBINDIR)
	install -d $(DESTDIR)$(MAN8DIR)
	install -m 755 build/standalone/fsck.hfs+ $(DESTDIR)$(SBINDIR)/fsck.hfs+
	install -m 644 doc/man/fsck.hfs+.8 $(DESTDIR)$(MAN8DIR)/fsck.hfs+.8
	@echo "[OK] fsck.hfs+ installed"
	@echo ""
	@echo "Additional options available:"
	@echo "  make install-mkfs.hfs+     # Install filesystem creator"
	@echo "  make install-mount.hfs+    # Install mount utility"
	@echo "  make install-set-hfsplus   # Install complete HFS+ toolset with .hfsplus symlinks"
	@echo ""

# mount utilities
install-mount.hfs:
	@echo "Installing mount.hfs..."
	install -d $(DESTDIR)$(SBINDIR)
	install -d $(DESTDIR)$(MAN8DIR)
	install -m 755 src/mount/mount.hfs $(DESTDIR)$(SBINDIR)/mount.hfs
	install -m 644 doc/man/mount.hfs.8 $(DESTDIR)$(MAN8DIR)/mount.hfs.8
	@echo "[OK] mount.hfs installed"
	@echo ""
	@echo "Additional options available:"
	@echo "  make install-mkfs.hfs      # Install filesystem creator"
	@echo "  make install-fsck.hfs      # Install filesystem checker"
	@echo "  make install-set-hfs       # Install complete HFS toolset"
	@echo ""

install-mount.hfs+:
	@echo "Installing mount.hfs+..."
	install -d $(DESTDIR)$(SBINDIR)
	install -d $(DESTDIR)$(MAN8DIR)
	install -m 755 src/mount/mount.hfs+ $(DESTDIR)$(SBINDIR)/mount.hfs+
	install -m 644 doc/man/mount.hfs+.8 $(DESTDIR)$(MAN8DIR)/mount.hfs+.8
	@echo "[OK] mount.hfs+ installed"
	@echo ""
	@echo "Additional options available:"
	@echo "  make install-mkfs.hfs+     # Install filesystem creator"
	@echo "  make install-fsck.hfs+     # Install filesystem checker"
	@echo "  make install-set-hfsplus   # Install complete HFS+ toolset with .hfsplus symlinks"
	@echo ""

# ============================================================================
# GROUP INSTALL TARGETS (all utilities of one type)
# ============================================================================

install-mkfs: install-mkfs.hfs install-mkfs.hfs+
	@echo "All mkfs utilities installed"

install-fsck: install-fsck.hfs install-fsck.hfs+
	@echo "All fsck utilities installed"

install-mount: install-mount.hfs install-mount.hfs+
	@echo "All mount utilities installed"

# ============================================================================
# SET INSTALL TARGETS (complete toolsets)
# ============================================================================

install-set-hfs: install-mkfs.hfs install-fsck.hfs install-mount.hfs
	@echo ""
	@echo "========================================="
	@echo "[OK] HFS toolset installed (mkfs.hfs, fsck.hfs, mount.hfs)"
	@echo "========================================="
	@echo ""
	@echo "Not installed yet:"
	@echo "  make install-set-hfsplus   # Install HFS+ toolset (mkfs.hfs+, fsck.hfs+, mount.hfs+)"
	@echo "  make install-complete      # Install all utilities including hfsutil"
	@echo ""

install-set-hfsplus: install-mkfs.hfs+ install-fsck.hfs+ install-mount.hfs+
	@echo "Creating .hfsplus symlinks..."
	ln -sf mkfs.hfs+ $(DESTDIR)$(SBINDIR)/mkfs.hfsplus
	ln -sf fsck.hfs+ $(DESTDIR)$(SBINDIR)/fsck.hfsplus
	ln -sf mount.hfs+ $(DESTDIR)$(SBINDIR)/mount.hfsplus
	ln -sf mkfs.hfs+.8 $(DESTDIR)$(MAN8DIR)/mkfs.hfsplus.8
	ln -sf fsck.hfs+.8 $(DESTDIR)$(MAN8DIR)/fsck.hfsplus.8
	ln -sf mount.hfs+.8 $(DESTDIR)$(MAN8DIR)/mount.hfsplus.8
	@echo ""
	@echo "========================================="
	@echo "[OK] HFS+ toolset installed (mkfs.hfs+, fsck.hfs+, mount.hfs+ + .hfsplus symlinks)"
	@echo "========================================="
	@echo ""
	@echo "Not installed yet:"
	@echo "  make install-set-hfs       # Install HFS toolset (mkfs.hfs, fsck.hfs, mount.hfs)"
	@echo "  make install-complete      # Install all utilities including hfsutil"
	@echo ""

# ============================================================================
# UNINSTALL TARGETS
# ============================================================================

uninstall-mkfs.hfs:
	rm -f $(DESTDIR)$(SBINDIR)/mkfs.hfs
	rm -f $(DESTDIR)$(MAN8DIR)/mkfs.hfs.8

uninstall-mkfs.hfs+:
	rm -f $(DESTDIR)$(SBINDIR)/mkfs.hfs+
	rm -f $(DESTDIR)$(MAN8DIR)/mkfs.hfs+.8

uninstall-fsck.hfs:
	rm -f $(DESTDIR)$(SBINDIR)/fsck.hfs
	rm -f $(DESTDIR)$(MAN8DIR)/fsck.hfs.8

uninstall-fsck.hfs+:
	rm -f $(DESTDIR)$(SBINDIR)/fsck.hfs+
	rm -f $(DESTDIR)$(MAN8DIR)/fsck.hfs+.8

uninstall-mount.hfs:
	rm -f $(DESTDIR)$(SBINDIR)/mount.hfs
	rm -f $(DESTDIR)$(MAN8DIR)/mount.hfs.8

uninstall-mount.hfs+:
	rm -f $(DESTDIR)$(SBINDIR)/mount.hfs+
	rm -f $(DESTDIR)$(MAN8DIR)/mount.hfs+.8

uninstall-set-hfs: uninstall-mkfs.hfs uninstall-fsck.hfs uninstall-mount.hfs
	@echo "HFS toolset uninstalled"

uninstall-set-hfsplus: uninstall-mkfs.hfs+ uninstall-fsck.hfs+ uninstall-mount.hfs+
	rm -f $(DESTDIR)$(SBINDIR)/mkfs.hfsplus
	rm -f $(DESTDIR)$(SBINDIR)/fsck.hfsplus
	rm -f $(DESTDIR)$(SBINDIR)/mount.hfsplus
	rm -f $(DESTDIR)$(MAN8DIR)/mkfs.hfsplus.8
	rm -f $(DESTDIR)$(MAN8DIR)/fsck.hfsplus.8
	rm -f $(DESTDIR)$(MAN8DIR)/mount.hfsplus.8
	@echo "HFS+ toolset uninstalled"

# ============================================================================
# CONVENIENCE INSTALL TARGETS
# ============================================================================

# Linux: Only filesystem utilities (mkfs, fsck, mount)
# These work with Linux kernel HFS/HFS+ drivers
install-linux: install-set-hfs install-set-hfsplus
	@echo "Linux utilities installed (HFS and HFS+ toolsets)"
	@echo "Note: Requires kernel modules: modprobe hfs hfsplus"

# Complete: All utilities including hfsutil
# For systems without HFS mount support or for full installation
install-complete: install-set-hfs install-set-hfsplus
	@echo "Installing hfsutil..."
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(MANDIR)/man1
	install -m 755 hfsutil $(DESTDIR)$(BINDIR)/hfsutil
	# Install hfsutil manpages
	install -m 644 doc/man/hfsutils.1 $(DESTDIR)$(MANDIR)/man1/hfsutils.1
	install -m 644 doc/man/hformat.1 $(DESTDIR)$(MANDIR)/man1/hformat.1
	install -m 644 doc/man/hls.1 $(DESTDIR)$(MANDIR)/man1/hls.1
	install -m 644 doc/man/hcopy.1 $(DESTDIR)$(MANDIR)/man1/hcopy.1
	install -m 644 doc/man/hmount.1 $(DESTDIR)$(MANDIR)/man1/hmount.1
	install -m 644 doc/man/humount.1 $(DESTDIR)$(MANDIR)/man1/humount.1
	@echo "Complete installation finished (filesystem utilities + hfsutil)"

# ============================================================================
# DOCUMENTATION TARGETS
# ============================================================================

# PDF documentation (requires TeXLive)
docs-pdf:
	@echo "Building PDF documentation..."
	@command -v pdflatex >/dev/null 2>&1 || { \
		echo "Error: pdflatex not found. Install TeXLive:"; \
		echo "  Debian/Ubuntu: sudo apt-get install texlive-latex-base texlive-latex-extra"; \
		echo "  Fedora: sudo dnf install texlive-scheme-medium"; \
		echo "  macOS: brew install --cask mactex-no-gui"; \
		echo "  BSD: pkg install texlive-full"; \
		exit 1; \
	}
	cd doc/latex && pdflatex -interaction=nonstopmode hfsutils-manual.tex
	cd doc/latex && pdflatex -interaction=nonstopmode hfsutils-manual.tex
	@echo "PDF documentation built: doc/latex/hfsutils-manual.pdf"

# Text documentation (from PDF, requires pdftotext)
docs-txt: docs-pdf
	@command -v pdftotext >/dev/null 2>&1 || { \
		echo "Error: pdftotext not found. Install poppler-utils:"; \
		echo "  Debian/Ubuntu: sudo apt-get install poppler-utils"; \
		echo "  Fedora: sudo dnf install poppler-utils"; \
		echo "  macOS: brew install poppler"; \
		exit 1; \
	}
	pdftotext doc/latex/hfsutils-manual.pdf doc/latex/hfsutils-manual.txt
	@echo "Text documentation built: doc/latex/hfsutils-manual.txt"

# HTML documentation (from PDF, requires pdf2htmlEX or pandoc)
docs-html: docs-pdf
	@echo "HTML documentation not yet implemented"
	@echo "Alternative: use 'make docs-pdf' and distribute the PDF"

# Build all documentation
docs: docs-pdf docs-txt
	@echo "All documentation formats built"

# Install documentation
install-docs: docs
	@echo "Installing documentation..."
	install -d $(DESTDIR)$(PREFIX)/share/doc/hfsutils
	install -m 644 doc/latex/hfsutils-manual.pdf \
		$(DESTDIR)$(PREFIX)/share/doc/hfsutils/
	install -m 644 doc/latex/hfsutils-manual.txt \
		$(DESTDIR)$(PREFIX)/share/doc/hfsutils/
	install -m 644 README.md $(DESTDIR)$(PREFIX)/share/doc/hfsutils/
	@echo "Documentation installed to $(PREFIX)/share/doc/hfsutils"

# Clean documentation build artifacts
docs-clean:
	cd doc/latex && rm -f *.aux *.log *.out *.toc *.pdf *.txt

.PHONY: all standalone symlinks clean distclean install-mkfs.hfs install-mkfs.hfs+ install-fsck.hfs install-fsck.hfs+ install-mount.hfs install-mount.hfs+ install-mkfs install-fsck install-mount install-set-hfs install-set-hfsplus uninstall-mkfs.hfs uninstall-mkfs.hfs+ uninstall-fsck.hfs uninstall-fsck.hfs+ uninstall-mount.hfs uninstall-mount.hfs+ uninstall-set-hfs uninstall-set-hfsplus install-linux install-complete test help libhfs librsrc hfsck mkfs.hfs fsck.hfs mount.hfs