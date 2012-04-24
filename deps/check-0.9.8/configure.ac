#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# Prelude.
AC_PREREQ([2.59])

AC_INIT([Check], [0.9.8], [check-devel at lists dot sourceforge dot net])
CHECK_MAJOR_VERSION=0
CHECK_MINOR_VERSION=9
CHECK_MICRO_VERSION=8
CHECK_VERSION=$CHECK_MAJOR_VERSION.$CHECK_MINOR_VERSION.$CHECK_MICRO_VERSION

# unique source file --- primitive safety check 
AC_CONFIG_SRCDIR([src/check.c])

# place where extra autoconf macros are kept
AC_CONFIG_MACRO_DIR([m4])

# place where portability library functions are kept
AC_CONFIG_LIBOBJ_DIR([lib])

# place to put some extra build scripts installed
AC_CONFIG_AUX_DIR([build-aux])

# define things like _GNU_SOURCE appropriately
AC_USE_SYSTEM_EXTENSIONS

# really severe build strictness
AM_INIT_AUTOMAKE([-Wall -Werror gnits 1.9.6])

# FIXME: find the other places this stuff is used

AC_SUBST(CHECK_MAJOR_VERSION)
AC_SUBST(CHECK_MINOR_VERSION)
AC_SUBST(CHECK_MICRO_VERSION)
AC_SUBST(CHECK_VERSION)

# Configure options.

AC_ARG_ENABLE(gcov,
AC_HELP_STRING([--enable-gcov],
	       [turn on test coverage @<:@default=no@:>@]),
[case "${enableval}" in
  yes) enable_gcov=true ;;
  no)  enable_gcov=false ;;
  *)   AC_MSG_ERROR(bad value ${enableval} for --enable-gcov) ;;
esac], [enable_gcov=false ])

if test x$enable_gcov = xtrue ; then
  if test -n "$GCC"; then
    AC_MSG_ERROR([gcov only works if gcc is used])
  fi

  GCOV_CFLAGS="-fprofile-arcs -ftest-coverage"
  AC_SUBST(GCOV_CFLAGS)

  dnl libtool 1.5.22 and lower strip -fprofile-arcs from the flags
  dnl passed to the linker, which is a bug; -fprofile-arcs implicitly
  dnl links in -lgcov, so we do it explicitly here for the same effect
  GCOV_LIBS=-lgcov
  AC_SUBST(GCOV_LIBS)
fi

AM_CONDITIONAL(ENABLE_GCOV, test x"$enable_gcov" = "xtrue")

AC_ARG_ENABLE(timeout-tests,
AC_HELP_STRING([--enable-timeout-tests],
	       [turn on timeout tests @<:@default=yes@:>@]),
[case "${enableval}" in
  yes) enable_timeout_tests=true ;;
  no)  enable_timeout_tests=false ;;
  *)   AC_MSG_ERROR(bad value ${enableval} for --enable-timeout-tests) ;;
esac], [enable_timeout_tests=true ])

AM_CONDITIONAL(NO_TIMEOUT_TESTS, test x"$enable_timeout_tests" = "xfalse")

AC_ARG_ENABLE(subunit,
AC_HELP_STRING([--enable-subunit],
	       [enable support for the subunit test protocol @<:@default=autodetect@:>@]),
[case "${enableval}" in
  yes)
    enable_subunit=true
    echo "Enabled subunit support"
    ;;
  no)
    enable_subunit=false
    echoo "Disabled subunit support"
    ;;
  autodetect)
    echo "Subunit support will enable automatically."
    ;;
  *)   AC_MSG_ERROR(bad value ${enableval} for --enable-subunit) ;;
esac], 
[echo "Subunit support will enable automatically."
 enable_subunit=autodetect])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
# Automake wants this for per-target CFLAGS
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL

# add these options to CFLAGS if the compiler supports them
AC_DEFUN([AX_CFLAGS_ADD],[AX_C_CHECK_FLAG($1, , , CFLAGS="$CFLAGS $1")])
AX_CFLAGS_WARN_ALL_ANSI
AX_CFLAGS_ADD([-Wextra])
AX_CFLAGS_ADD([-Wstrict-prototypes])
AX_CFLAGS_ADD([-Wmissing-prototypes])
AX_CFLAGS_ADD([-Wwrite-strings])
AX_CFLAGS_ADD([-Wno-variadic-macros])

AC_CHECK_PROGS(GCOV, gcov, false)
AC_CHECK_PROGS(LCOV, lcov, false)
AC_CHECK_PROGS(GENHTML, genhtml, false)
AC_CHECK_PROGS(TEX, tex, false)
if test -n "$TEX"; then
    # Make it [somewhat] clear to maintainers that tex is missing. Not an error
    # though because 'make install' (which users need) does not build the docs
    # anyway.
    AC_MSG_WARN(tex not installed: cannot rebuild HTML documentation.)
fi

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h stddef.h stdint.h stdlib.h string.h sys/time.h unistd.h])

if test xfalse != x"$enable_subunit"; then
AC_CHECK_LIB(subunit, subunit_test_start, ,
[case "$enable_subunit" in
 autodetect)
   enable_subunit=false
   ;;
 true)
   AC_MSG_ERROR([libsubunit is required for subunit protocol support. The homepage for subunit is https://launchpad.net/subunit/])
   ;;
 esac
])
fi
if test xfalse != x"$enable_subunit"; then
AC_CHECK_HEADER([subunit/child.h], ,
[case "$enable_subunit" in
 autodetect)
   enable_subunit=false
   ;;
 true)
   AC_MSG_ERROR([The header subunit/child.h could not be succesfully included and is required for subunit protocol support. The homepage for subunit is https://launchpad.net/subunit/])
   ;;
 esac
])
fi
if test xfalse = x"$enable_subunit"; then
ENABLE_SUBUNIT="0"
else
ENABLE_SUBUNIT="1"
fi
AC_SUBST(ENABLE_SUBUNIT)
AC_DEFINE_UNQUOTED(ENABLE_SUBUNIT, $ENABLE_SUBUNIT, [Subunit protocol result output])

AM_CONDITIONAL(SUBUNIT, test x"$enable_subunit" != "xfalse")



# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(short, 2)
AC_CHECK_SIZEOF(long, 4)

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_REPLACE_FUNCS([fileno localtime_r pipe putenv setenv sleep strdup strsignal unsetenv])
AC_CHECK_DECLS([fileno, localtime_r, pipe, putenv, setenv, sleep, strdup, strsignal, unsetenv])

# Checks for pthread implementation.
ACX_PTHREAD
CC="$PTHREAD_CC"

# Output files
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([check.pc
                 Makefile
		 doc/Makefile
		 lib/Makefile
                 src/check.h
                 src/Makefile
                 tests/Makefile
		 tests/test_vars])

AC_OUTPUT
