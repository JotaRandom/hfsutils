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

# Filesystem utilities (separate binaries with symlinks)
FSCK_LINKS = fsck.hfs fsck.hfs+ fsck.hfsplus
MKFS_LINKS = mkfs.hfs mkfs.hfs+ mkfs.hfsplus

# Default target - just build hfsutil without symlinks
all: libhfs librsrc hfsck hfsutil

# Alternative target that avoids autotools completely
build-manual: libhfs librsrc hfsck-manual hfsutil

# Target to create symlinks for backward compatibility
symlinks: hfsutil $(EXECUTABLES) hdir $(MKFS_LINKS) $(FSCK_LINKS)

# Build libraries
libhfs:
	$(MAKE) -C libhfs CC="$(CC)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" PREFIX="$(PREFIX)" DESTDIR="$(DESTDIR)"

librsrc:
	$(MAKE) -C librsrc CC="$(CC)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" PREFIX="$(PREFIX)" DESTDIR="$(DESTDIR)"

hfsck:
	@echo "Building hfsck..."
	@if $(MAKE) -C hfsck CC="$(CC)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" PREFIX="$(PREFIX)" DESTDIR="$(DESTDIR)" 2>/dev/null; then \
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

# Create filesystem utility symlinks
$(MKFS_LINKS): hfsutil
	ln -sf hfsutil $@

$(FSCK_LINKS): hfsck
	ln -sf hfsck/hfsck $@

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
	@$(MAKE) -C hfsck clean

distclean: clean
	rm -f config.h config.log config.status
	rm -rf autom4te.cache
	$(MAKE) -C libhfs distclean
	$(MAKE) -C librsrc distclean
	$(MAKE) -C hfsck distclean

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
	@if $(MAKE) -C hfsck install CC="$(CC)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" PREFIX="$(PREFIX)" DESTDIR="$(DESTDIR)" 2>/dev/null; then \
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
	# Create filesystem utility symlinks (smart path detection)
	@if [ "$(SBINDIR)" = "$(BINDIR)" ]; then \
		echo "Creating filesystem utility symlinks for merged /bin system..."; \
		for prog in $(FSCK_LINKS); do \
			ln -sf hfsck $(DESTDIR)$(BINDIR)/$$prog; \
		done; \
	else \
		echo "Creating filesystem utility symlinks for separate /sbin system..."; \
		for prog in $(FSCK_LINKS); do \
			ln -sf ../sbin/hfsck $(DESTDIR)$(BINDIR)/$$prog; \
		done; \
	fi
	for prog in $(MKFS_LINKS); do \
		ln -sf hfsutil $(DESTDIR)$(BINDIR)/$$prog; \
	done
	@echo "Created symlinks for traditional command names"
	@echo "Created symlinks for filesystem utilities (fsck.hfs, mkfs.hfs, etc.)"

test: all
	cd test && ./generate_test_data.sh && ./run_tests.sh

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

.PHONY: all symlinks clean distclean install install-libs install-symlinks test help libhfs librsrc hfsck