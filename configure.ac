#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([Statsite], [0.8.0])

STATSITE_MAJOR_VERSION=0
STATSITE_MINOR_VERSION=8
STATSITE_MICRO_VERSION=0
STATSITE_VERSION=$STATSITE_MAJOR_VERSION.$STATSITE_MINOR_VERSION.$STATSITE_MICRO_VERSION


AC_CONFIG_SRCDIR([src/statsite.c])

# place to put some extra build scripts installed
AC_CONFIG_AUX_DIR([ac_config])

# place where extra autoconf macros are kept
AC_CONFIG_MACRO_DIR([m4])

# define things like _GNU_SOURCE appropriately
AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE(foreign subdir-objects)

# Check if we have librt
AC_CHECK_LIB([rt], [clock_gettime], [LINK_TO_RT=-lrt], [LINK_TO_RT=])


# This replaces a few constants we setup previously
AC_SUBST(STATSITE_MAJOR_VERSION)
AC_SUBST(STATSITE_MINOR_VERSION)
AC_SUBST(STATSITE_MICRO_VERSION)
AC_SUBST(STATSITE_VERSION)
AC_SUBST(LINK_TO_RT)

# Checks for programs.
AC_PROG_CC([gcc cc])



# Automake wants this for per-target CFLAGS
AC_PROG_INSTALL
AC_PROG_LN_S

LT_INIT

AC_OUTPUT(Makefile)
