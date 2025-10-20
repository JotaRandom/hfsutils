# HFS Utilities for Apple Silicon
# Builds unified hfsutil binary with all utilities

CC = gcc
CFLAGS = -g -O2 -Wall -I. -I./include -I./include/common -I./include/hfsutil -I./include/binhex -I./libhfs -I./librsrc
LDFLAGS = -L./libhfs -L./librsrc
LIBS = -lhfs -lrsrc

# Build directory for object files
BUILDDIR = build
OBJDIR = $(BUILDDIR)/obj

# Ensure build directories exist
$(shell mkdir -p $(OBJDIR))

# Executables (symlinks to hfsutil)
EXECUTABLES = hattrib hcd hcopy hdel hformat hls hmkdir hmount hpwd hrename hrmdir humount hvol

# Default target - just build hfsutil without symlinks
all: libhfs librsrc hfsck hfsutil

# Target to create symlinks for backward compatibility
symlinks: hfsutil $(EXECUTABLES) hdir

# Build libraries
libhfs:
	$(MAKE) -C libhfs

librsrc:
	$(MAKE) -C librsrc

hfsck:
	$(MAKE) -C hfsck

# Object files in build directory
$(OBJDIR)/hcwd.o: src/common/hcwd.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/suid.o: src/common/suid.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/glob.o: src/common/glob.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/version.o: src/common/version.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/charset.o: src/common/charset.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/binhex.o: src/binhex/binhex.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/copyin.o: src/common/copyin.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/copyout.o: src/common/copyout.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/crc.o: src/common/crc.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/darray.o: src/common/darray.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/dlist.o: src/common/dlist.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/dstring.o: src/common/dstring.c
	$(CC) $(CFLAGS) -c $< -o $@

# Utility object files
$(OBJDIR)/hattrib.o: src/hfsutil/hattrib.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/hcd.o: src/hfsutil/hcd.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/hcopy.o: src/hfsutil/hcopy.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/hdel.o: src/hfsutil/hdel.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/hformat.o: src/hfsutil/hformat.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/hls.o: src/hfsutil/hls.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/hmkdir.o: src/hfsutil/hmkdir.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/hmount.o: src/hfsutil/hmount.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/hpwd.o: src/hfsutil/hpwd.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/hrename.o: src/hfsutil/hrename.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/hrmdir.o: src/hfsutil/hrmdir.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/humount.o: src/hfsutil/humount.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/hvol.o: src/hfsutil/hvol.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/hfsutil.o: src/hfsutil/hfsutil.c
	$(CC) $(CFLAGS) -c $< -o $@

# Common objects
COMMON_OBJS = $(OBJDIR)/hcwd.o $(OBJDIR)/suid.o $(OBJDIR)/glob.o \
              $(OBJDIR)/version.o $(OBJDIR)/charset.o $(OBJDIR)/binhex.o

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
	$(CC) $(LDFLAGS) $(UTIL_OBJS) $(COMMON_OBJS) $(LIBS) -o $@

# Create symlinks for individual commands
$(EXECUTABLES): hfsutil
	ln -sf hfsutil $@

hdir: hfsutil
	ln -sf hfsutil hdir

clean:
	rm -f $(EXECUTABLES) hdir hfsutil
	rm -rf $(BUILDDIR)
	$(MAKE) -C libhfs clean
	$(MAKE) -C librsrc clean

distclean: clean
	rm -f config.h config.log config.status Makefile
	rm -rf autom4te.cache
	$(MAKE) -C libhfs distclean
	$(MAKE) -C librsrc distclean

install: all
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 hfsutil $(DESTDIR)/usr/local/bin/
	@echo "Installed hfsutil to $(DESTDIR)/usr/local/bin/"
	@echo "You can optionally create symlinks with 'make install-symlinks'"

install-symlinks: install
	for prog in $(EXECUTABLES) hdir; do \
		ln -sf hfsutil $(DESTDIR)/usr/local/bin/$$prog; \
	done
	@echo "Created symlinks for traditional command names"

test: all
	cd test && ./generate_test_data.sh && ./run_tests.sh

help:
	@echo "HFS Utilities for Apple Silicon - Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make              - Build hfsutil executable"
	@echo "  make symlinks     - Create command symlinks (optional)"
	@echo "  make clean        - Remove built files"
	@echo "  make distclean    - Remove all generated files"
	@echo "  make install      - Install hfsutil to /usr/local/bin"
	@echo "  make install-symlinks - Install with traditional command names"
	@echo "  make test         - Run test suite"
	@echo "  make help         - Show this help"
	@echo ""
	@echo "The single 'hfsutil' binary contains all utilities."
	@echo "Usage: hfsutil <command> [options]"
	@echo "Run 'hfsutil' without arguments to see available commands."

.PHONY: all symlinks clean distclean install install-symlinks test help libhfs librsrc