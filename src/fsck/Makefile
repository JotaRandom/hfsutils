# Makefile for fsck.hfs standalone utility

CC ?= gcc
CFLAGS ?= -g -O2
INTERNAL_CFLAGS = -Wall -I. -I../embedded -I../embedded/shared -I../../include -I../../libhfs

# Combined flags
ALL_CFLAGS = $(CFLAGS) $(INTERNAL_CFLAGS)

# Build directories
BUILDDIR = ../../build/standalone
OBJDIR = $(BUILDDIR)/fsck

# Ensure build directories exist
$(shell mkdir -p $(OBJDIR))

# Source files
FSCK_HFS_SOURCES = \
	fsck_main.c \
	fsck_common.c \
	hfsplus_check.c \
	journal.c

FSCK_HFSPLUS_SOURCES = \
	fsck_hfsplus_main.c \
	fsck_common.c \
	hfsplus_check.c \
	journal.c

# Object files
FSCK_HFS_OBJECTS = $(FSCK_HFS_SOURCES:%.c=$(OBJDIR)/%.o)
FSCK_HFSPLUS_OBJECTS = $(FSCK_HFSPLUS_SOURCES:%.c=$(OBJDIR)/%.o)

# Embedded libraries
EMBEDDED_DIR = ../embedded
SHARED_LIB = $(BUILDDIR)/libshared.a
FSCK_LIB = $(BUILDDIR)/libfsck.a

# Target executables
FSCK_EXECUTABLE = $(BUILDDIR)/fsck.hfs
FSCK_HFSPLUS_EXECUTABLE = $(BUILDDIR)/fsck.hfs+

# Default target
all: $(FSCK_EXECUTABLE) $(FSCK_HFSPLUS_EXECUTABLE)

# Build fsck.hfs executable
$(FSCK_EXECUTABLE): $(FSCK_HFS_OBJECTS) $(SHARED_LIB) $(FSCK_LIB)
	$(CC) $(ALL_CFLAGS) -o $@ $(FSCK_HFS_OBJECTS) $(FSCK_LIB) $(SHARED_LIB)

# Build fsck.hfs+ executable
$(FSCK_HFSPLUS_EXECUTABLE): $(FSCK_HFSPLUS_OBJECTS) $(SHARED_LIB) $(FSCK_LIB)
	$(CC) $(ALL_CFLAGS) -o $@ $(FSCK_HFSPLUS_OBJECTS) $(FSCK_LIB) $(SHARED_LIB)

# Build embedded libraries first
$(SHARED_LIB) $(FSCK_LIB):
	$(MAKE) -C $(EMBEDDED_DIR) all

# Object file rules
$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(ALL_CFLAGS) -c $< -o $@

# Create symlink (fsck.hfsplus -> fsck.hfs+)
links: $(FSCK_HFSPLUS_EXECUTABLE)
	ln -sf fsck.hfs+ $(BUILDDIR)/fsck.hfsplus

# Install target
install: $(FSCK_EXECUTABLE) $(FSCK_HFSPLUS_EXECUTABLE) links
	install -d $(DESTDIR)/sbin
	install -m 755 $(FSCK_EXECUTABLE) $(DESTDIR)/sbin/
	install -m 755 $(FSCK_HFSPLUS_EXECUTABLE) $(DESTDIR)/sbin/
	ln -sf fsck.hfs+ $(DESTDIR)/sbin/fsck.hfsplus

# Clean
clean:
	rm -rf $(OBJDIR)
	rm -f $(FSCK_EXECUTABLE)
	rm -f $(FSCK_HFSPLUS_EXECUTABLE)
	rm -f $(BUILDDIR)/fsck.hfsplus

# Dependencies
$(FSCK_HFS_OBJECTS) $(FSCK_HFSPLUS_OBJECTS): ../embedded/fsck/fsck_hfs.h ../embedded/shared/common_utils.h

.PHONY: all links install clean