#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.


## Set the version in both instances below here:
AC_INIT([Statsite], [0.8.1])

STATSITE_MAJOR_VERSION=0
STATSITE_MINOR_VERSION=8
STATSITE_MICRO_VERSION=1
STATSITE_VERSION=$STATSITE_MAJOR_VERSION.$STATSITE_MINOR_VERSION.$STATSITE_MICRO_VERSION



# place to put some extra build scripts installed
AC_CONFIG_AUX_DIR([ac_config])

# place where extra autoconf macros are kept
AC_CONFIG_MACRO_DIR([m4])

# define things like _GNU_SOURCE appropriately
AC_USE_SYSTEM_EXTENSIONS


AC_CONFIG_SRCDIR([src/statsite.c])
AC_CONFIG_HEADERS([src/buildconfig.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
# Automake wants this for per-target CFLAGS
AC_PROG_INSTALL
AC_PROG_LN_S


# Check if we have librt
AC_CHECK_LIB([rt], [clock_gettime], [LINK_TO_RT=-lrt], [LINK_TO_RT=])


# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h limits.h netdb.h netinet/in.h stdint.h stdlib.h string.h strings.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_CHECK_HEADER_STDBOOL # - this seems to be archived
AC_C_INLINE
AC_TYPE_INT8_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([bzero dup2 getpagesize gettimeofday inet_ntoa memchr memset pow select socket sqrt strcasecmp strdup strerror strncasecmp strtol])


# PKG-Config based checks
PKG_CHECK_MODULES([CHECK], [check >= 0.9.7], [have_check="yes"], AC_MSG_WARN([Check not found; cannot run unit tests!]))

AM_CONDITIONAL(HAVE_CHECK, test x"$have_check" = "xyes")

# tests
#TESTS = test_runner
#check_PROGRAMS = test_runner
#test_runner_SOURCES = tests/runner.c $(top_builddir)/tests/runner.h
#test_runner_CFLAGS = @CHECK_CFLAGS@


AM_INIT_AUTOMAKE(foreign subdir-objects)

# This replaces a few constants we setup previously
AC_SUBST(STATSITE_MAJOR_VERSION)
AC_SUBST(STATSITE_MINOR_VERSION)
AC_SUBST(STATSITE_MICRO_VERSION)
AC_SUBST(STATSITE_VERSION)
AC_SUBST(LINK_TO_RT)




LT_INIT

AC_OUTPUT(Makefile man/Makefile)
