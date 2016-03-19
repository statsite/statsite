#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([Statsite], [0.7.1])

STATSITE_MAJOR_VERSION=0
STATSITE_MINOR_VERSION=7
STATSITE_MICRO_VERSION=1
STATSITE_VERSION=$STATSITE_MAJOR_VERSION.$STATSITE_MINOR_VERSION.$STATSITE_MICRO_VERSION


AC_CONFIG_SRCDIR([src/statsite.c])

# place to put some extra build scripts installed
AC_CONFIG_AUX_DIR([ac_config])

# place where extra autoconf macros are kept
AC_CONFIG_MACRO_DIR([m4])

# define things like _GNU_SOURCE appropriately
AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE(foreign subdir-objects)

# This replaces a few constants we setup previously
AC_SUBST(STATSITE_MAJOR_VERSION)
AC_SUBST(STATSITE_MINOR_VERSION)
AC_SUBST(STATSITE_MICRO_VERSION)
AC_SUBST(STATSITE_VERSION)

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC

# Automake wants this for per-target CFLAGS
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL

AC_PROG_CC([gcc cc])

LT_INIT


AC_OUTPUT(Makefile)
