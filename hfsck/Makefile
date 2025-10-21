# hfsck - filesystem checker for HFS volumes
# Copyright (C) 1996-1998 Robert Leslie
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# $Id: Makefile.in,v 1.0 2025/10/20 user Exp $
#


src =		.
VPATH =		.

### USER CUSTOMIZATIONS FOLLOW ################################################

prefix =	/usr/local
exec_prefix =	${prefix}

MANDEST =	${prefix}/share/man

INCDEST =	${prefix}/include
LIBDEST =	${exec_prefix}/lib

INSTALL =	/usr/bin/install -c

CC =		gcc
INCLUDES =	 -I./../include -I./../include/common -I./../libhfs -I./../src/common
DEFINES =	-DHAVE_CONFIG_H
LIBOBJS =	
LIBS =		 ./../libhfs/libhfs.a

COPTS =		-g -O2
LDOPTS =	

AR =		ar rc
RANLIB =	ranlib

### END OF USER CUSTOMIZATION #################################################

CFLAGS =	$(COPTS) $(INCLUDES) $(DEFINES)
LDFLAGS =	$(LDOPTS)

###############################################################################

TARGETS =	$(HFSCHECK)

HFSCHECK =	hfsck
HFSCKSRCS =	ck_btree.c ck_mdb.c ck_volume.c hfsck.c main.c util.c
HFSCKOBJS =	$(HFSCKSRCS:.c=.o) suid.o version.o

###############################################################################

all :: $(TARGETS)

install ::
	@echo "No install target for hfsck"

again :: clean all

depend ::
	( sed -n '1,/^### DEPEND/p' Makefile.in;  \
	  echo;  \
	  $(CC) -MM $(INCLUDES) $(DEFINES) *.c;  \
	) > Makefile.in.new
	mv -f Makefile.in.new Makefile.in

clean ::
	rm -f $(TARGETS) *.o gmon.* core
	rm -rf autom4te.cache .stamp

distclean :: clean
	rm -f config.status config.cache config.log config.h Makefile
	rm -f configure

maintainer-clean :: distclean
	rm -f config.h.in configure

###############################################################################

Makefile: config.status
	./config.status && touch .stamp/config.h

.stamp/config.h: config.status
	./config.status && touch .stamp/config.h

config.status: .stamp/configure .stamp/config.h.in Makefile.in
	./config.status --recheck

.stamp/configure: configure.in
	autoconf && touch .stamp/configure

.stamp/config.h.in: configure.in acconfig.h
	autoheader && touch .stamp/config.h.in

###############################################################################

$(HFSCHECK): $(HFSCKOBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(HFSCKOBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

# Common support objects from src/common
suid.o:
	$(CC) $(CFLAGS) -c ../src/common/suid.c -o suid.o

version.o:
	$(CC) $(CFLAGS) -c ../src/common/version.c -o version.o

###############################################################################

### DEPENDENCIES FOLLOW #######################################################

ck_btree.o: ck_btree.c ck_btree.h
ck_mdb.o: ck_mdb.c ck_mdb.h
ck_volume.o: ck_volume.c ck_volume.h
hfsck.o: hfsck.c hfsck.h
main.o: main.c util.h
util.o: util.c util.h
