# Makefile for embedded libraries - Reorganized structure
# Builds static libraries for standalone utilities

CC ?= gcc
CFLAGS ?= -g -O2
INTERNAL_CFLAGS = -Wall -I. -I./shared -I../../include -I../../libhfs

# Combined flags
ALL_CFLAGS = $(CFLAGS) $(INTERNAL_CFLAGS)

# Build directories
BUILDDIR = ../../build/standalone
OBJDIR = $(BUILDDIR)/embedded

# Ensure build directories exist
$(shell mkdir -p $(OBJDIR)/mkfs $(OBJDIR)/fsck $(OBJDIR)/mount $(OBJDIR)/shared)

# Shared source files (used by all utilities)
SHARED_SOURCES = \
	shared/hfs_mount.c \
	shared/volume_mkfs.c \
	shared/stubs.c \
	shared/hfsutil_mkfs.c \
	shared/hfs_detect.c \
	shared/version.c \
	shared/suid.c \
	shared/device_utils.c \
	shared/error_utils.c \
	shared/common_utils.c \
	shared/hfs_utils.c

# mkfs.hfs specific sources
MKFS_SOURCES = \
	mkfs/hfs_format.c

# fsck.hfs specific sources (simplified for now)
FSCK_SOURCES = \
	fsck/hfs_check.c

# mount.hfs specific sources
MOUNT_SOURCES = \
	mount/hfs_mount_util.c

# Object files
SHARED_OBJECTS = $(SHARED_SOURCES:%.c=$(OBJDIR)/%.o)
MKFS_OBJECTS = $(MKFS_SOURCES:%.c=$(OBJDIR)/%.o)
FSCK_OBJECTS = $(FSCK_SOURCES:%.c=$(OBJDIR)/%.o)
MOUNT_OBJECTS = $(MOUNT_SOURCES:%.c=$(OBJDIR)/%.o)

# Static libraries
MKFS_LIB = $(BUILDDIR)/libmkfs.a
FSCK_LIB = $(BUILDDIR)/libfsck.a
MOUNT_LIB = $(BUILDDIR)/libmount.a
SHARED_LIB = $(BUILDDIR)/libshared.a

# Default target
all: $(MKFS_LIB) $(FSCK_LIB) $(MOUNT_LIB) $(SHARED_LIB)

# Build shared library
$(SHARED_LIB): $(SHARED_OBJECTS)
	ar rcs $@ $^

# Build mkfs.hfs library
$(MKFS_LIB): $(MKFS_OBJECTS) $(SHARED_LIB)
	ar rcs $@ $(MKFS_OBJECTS)

# Build fsck.hfs library
$(FSCK_LIB): $(FSCK_OBJECTS) $(SHARED_LIB)
	ar rcs $@ $(FSCK_OBJECTS)

# Build mount.hfs library
$(MOUNT_LIB): $(MOUNT_OBJECTS) $(SHARED_LIB)
	ar rcs $@ $(MOUNT_OBJECTS)

# Object file rules
$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(ALL_CFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf $(OBJDIR)
	rm -f $(MKFS_LIB) $(FSCK_LIB) $(MOUNT_LIB) $(SHARED_LIB)

# Dependencies
$(MKFS_OBJECTS): mkfs/mkfs_hfs.h shared/libhfs.h
$(FSCK_OBJECTS): fsck/fsck_hfs.h shared/libhfs.h
$(MOUNT_OBJECTS): mount/mount_hfs.h shared/libhfs.h
$(SHARED_OBJECTS): shared/libhfs.h

.PHONY: all clean