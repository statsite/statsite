## Process this file with automake to produce Makefile.in

TESTS = \
	check_check_export	\
	check_check		\
	test_output.sh		\
	test_xml_output.sh	\
	test_log_output.sh

# check_thread_stress is kind of slow.
# add this line back to TESTS to enable check_thread_stress
# 	check_thread_stress     \
#
# uncomment if check_thread_stress enabled
# XFAIL_TESTS = \
# 	check_thread_stress

noinst_PROGRAMS = \
	check_check_export \
	check_check	\
	check_stress	\
	check_thread_stress \
	ex_output	\
	ex_xml_output	\
	ex_log_output

EXTRA_DIST = test_output.sh test_log_output.sh test_vars.in test_xml_output.sh

if NO_TIMEOUT_TESTS
check_check_CFLAGS = -DTIMEOUT_TESTS_ENABLED=0
check_check_export_CFLAGS = -DTIMEOUT_TESTS_ENABLED=0
endif

check_check_export_SOURCES = \
	check_check.h		\
	check_check_sub.c	\
	check_check_master.c	\
	check_check_log.c	\
	check_check_fork.c	\
	check_check_export_main.c
check_check_export_LDADD = $(top_builddir)/src/libcheck.la $(top_builddir)/lib/libcompat.la

check_check_SOURCES = \
	check_check.h			\
	check_list.c			\
	check_check_sub.c		\
	check_check_master.c		\
	check_check_msg.c		\
	check_check_log.c		\
	check_check_log_internal.c	\
	check_check_limit.c		\
	check_check_fork.c		\
	check_check_fixture.c		\
	check_check_pack.c		\
	check_check_exit.c		\
	check_check_main.c
check_check_LDADD = $(top_builddir)/src/libcheckinternal.la $(top_builddir)/lib/libcompat.la

check_stress_SOURCES = check_stress.c
check_stress_LDADD = $(top_builddir)/src/libcheck.la $(top_builddir)/lib/libcompat.la

check_thread_stress_SOURCES = check_thread_stress.c
check_thread_stress_LDADD = $(top_builddir)/src/libcheck.la $(top_builddir)/lib/libcompat.la @PTHREAD_LIBS@
check_thread_stress_CFLAGS = @PTHREAD_CFLAGS@

ex_output_SOURCES = ex_output.c
ex_output_LDADD = $(top_builddir)/src/libcheck.la $(top_builddir)/lib/libcompat.la

ex_log_output_SOURCES = ex_log_output.c
ex_log_output_LDADD = $(top_builddir)/src/libcheck.la $(top_builddir)/lib/libcompat.la

ex_xml_output_SOURCES = ex_xml_output.c
ex_xml_output_LDADD = $(top_builddir)/src/libcheck.la $(top_builddir)/lib/libcompat.la

AM_CPPFLAGS = -I$(top_builddir)/src -I$(top_srcdir)/src

CLEANFILES = *~ *.log *.xml test_logfile
