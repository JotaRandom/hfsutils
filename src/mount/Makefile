# Mount utilities Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I../../include
LDFLAGS =

SOURCES = mount_main.c mount_hfs.c mount_hfsplus.c
OBJECTS = $(SOURCES:.c=.o)

# Targets
.PHONY: all clean

all: mount.hfs mount.hfs+

mount.hfs: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

mount.hfs+: mount.hfs
	ln -sf mount.hfs mount.hfs+

%.o: %.c mount_common.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) mount.hfs mount.hfs+ mount.hfsplus

.PHONY: install
install: all
	install -m 755 mount.hfs $(DESTDIR)/sbin/mount.hfs
	ln -sf mount.hfs $(DESTDIR)/sbin/mount.hfs+
	ln -sf mount.hfs+ $(DESTDIR)/sbin/mount.hfsplus
