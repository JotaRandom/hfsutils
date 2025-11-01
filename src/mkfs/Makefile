# Makefile for mkfs.hfs standalone utility

CC ?= gcc
CFLAGS ?= -g -O2
INTERNAL_CFLAGS = -Wall -I. -I../embedded -I../embedded/shared -I../../include -I../../libhfs

# Combined flags
ALL_CFLAGS = $(CFLAGS) $(INTERNAL_CFLAGS)

# Build directories
BUILDDIR = ../../build/standalone
OBJDIR = $(BUILDDIR)/mkfs

# Ensure build directories exist
$(shell mkdir -p $(OBJDIR))

# Source files
MKFS_HFS_SOURCES = \
	mkfs_hfs_main.c \
	mkfs_hfs_format.c \
	mkfs_common.c

MKFS_HFSPLUS_SOURCES = \
	mkfs_hfsplus_main.c \
	mkfs_hfs_format.c \
	mkfs_common.c

# Object files
MKFS_HFS_OBJECTS = $(MKFS_HFS_SOURCES:%.c=$(OBJDIR)/%.o)
MKFS_HFSPLUS_OBJECTS = $(MKFS_HFSPLUS_SOURCES:%.c=$(OBJDIR)/%.o)

# Embedded libraries
EMBEDDED_DIR = ../embedded
SHARED_LIB = $(BUILDDIR)/libshared.a
MKFS_LIB = $(BUILDDIR)/libmkfs.a

# Target executables
MKFS_EXECUTABLE = $(BUILDDIR)/mkfs.hfs
MKFS_HFSPLUS_EXECUTABLE = $(BUILDDIR)/mkfs.hfs+

# Default target
all: $(MKFS_EXECUTABLE) $(MKFS_HFSPLUS_EXECUTABLE)

# Build mkfs.hfs executable
$(MKFS_EXECUTABLE): $(MKFS_HFS_OBJECTS) $(SHARED_LIB) $(MKFS_LIB)
	$(CC) $(ALL_CFLAGS) -o $@ $(MKFS_HFS_OBJECTS) $(MKFS_LIB) $(SHARED_LIB)

# Build mkfs.hfs+ executable
$(MKFS_HFSPLUS_EXECUTABLE): $(MKFS_HFSPLUS_OBJECTS) $(SHARED_LIB) $(MKFS_LIB)
	$(CC) $(ALL_CFLAGS) -o $@ $(MKFS_HFSPLUS_OBJECTS) $(MKFS_LIB) $(SHARED_LIB)

# Build embedded libraries first
$(SHARED_LIB) $(MKFS_LIB):
	$(MAKE) -C $(EMBEDDED_DIR) all

# Object file rules
$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(ALL_CFLAGS) -c $< -o $@

# Create symlink (mkfs.hfsplus -> mkfs.hfs+)
links: $(MKFS_HFSPLUS_EXECUTABLE)
	ln -sf mkfs.hfs+ $(BUILDDIR)/mkfs.hfsplus

# Install target
install: $(MKFS_EXECUTABLE) $(MKFS_HFSPLUS_EXECUTABLE) links
	install -d $(DESTDIR)/sbin
	install -m 755 $(MKFS_EXECUTABLE) $(DESTDIR)/sbin/
	install -m 755 $(MKFS_HFSPLUS_EXECUTABLE) $(DESTDIR)/sbin/
	ln -sf mkfs.hfs+ $(DESTDIR)/sbin/mkfs.hfsplus

# Clean
clean:
	rm -rf $(OBJDIR)
	rm -f $(MKFS_EXECUTABLE)
	rm -f $(MKFS_HFSPLUS_EXECUTABLE)
	rm -f $(BUILDDIR)/mkfs.hfsplus

# Dependencies
$(MKFS_HFS_OBJECTS) $(MKFS_HFSPLUS_OBJECTS): ../embedded/mkfs/mkfs_hfs.h ../embedded/shared/common_utils.h

.PHONY: all links install clean