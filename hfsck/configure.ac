dnl -*- shell-script -*-
dnl Modernized autoconf input for hfsck
dnl
dnl hfsck - filesystem checker for HFS volumes
dnl Copyright (C) 1996-1998 Robert Leslie
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
dnl

AC_PREREQ([2.71])
AC_INIT([hfsck],[0.1],[prflr@users.noreply])
AC_CONFIG_SRCDIR([hfsck.h])
AC_CONFIG_HEADERS([config.h])

AC_ARG_ENABLE([debug],
    [  --enable-debug          enable diagnostic debugging support], [enable_debug=no])

if test "x$enable_debug" = xyes; then
  AC_DEFINE([DEBUG], [1], [Enable diagnostic debugging support])
fi

dnl Programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_MAKE_SET
AC_PROG_GCC_TRADITIONAL

dnl Headers
AC_CHECK_HEADERS([unistd.h fcntl.h stdio.h stdlib.h string.h inttypes.h stdint.h strings.h sys/stat.h sys/types.h])

dnl Types and structures
AC_TYPE_SIZE_T
AC_STRUCT_TM

dnl Const handling
AC_C_CONST

dnl Functions
AC_CHECK_FUNCS([mktime])

dnl Output files
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
