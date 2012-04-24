## Process this file with automake to produce Makefile.in

info_TEXINFOS = check.texi
check_TEXINFOS = fdl.texi

## we need to include several diffs as we evolve the example in the
## tutorial.  this means we'll generate them from the example source.

$(srcdir)/check.texi: money.1-2.h.diff \
	    money.1-3.c.diff \
	    money.3-4.c.diff \
	    money.4-5.c.diff \
	    money.5-6.c.diff \
	    check_money.1-2.c.diff \
	    check_money.2-3.c.diff \
	    check_money.3-6.c.diff \
	    check_money.6-7.c.diff

eg_root = $(top_srcdir)/doc/example
eg_src = $(eg_root)/src
eg_tests = $(eg_root)/tests

## now a rule for each diff.  the redundancy here can probably be
## parameterized, but I don't know how.  if you know, please tell us!

# diff returns 1 if there is a difference, but we don't want make to
# think that means there is an error
money.1-2.h.diff: $(eg_src)/money.1.h $(eg_src)/money.2.h
	cd $(eg_root); \
	diff -u src/money.1.h src/money.2.h > @abs_builddir@/$@ || test $$? -eq 1; \
	cd -;

money.1-3.c.diff: $(eg_src)/money.1.c $(eg_src)/money.3.c
	cd $(eg_root); \
	diff -u src/money.1.c src/money.3.c > @abs_builddir@/$@ || test $$? -eq 1; \
	cd -;

money.3-4.c.diff: $(eg_src)/money.3.c $(eg_src)/money.4.c
	cd $(eg_root); \
	diff -u src/money.3.c src/money.4.c > @abs_builddir@/$@ || test $$? -eq 1; \
	cd -;

money.4-5.c.diff: $(eg_src)/money.4.c $(eg_src)/money.5.c
	cd $(eg_root); \
	diff -u src/money.4.c src/money.5.c > @abs_builddir@/$@ || test $$? -eq 1; \
	cd -;

money.5-6.c.diff: $(eg_src)/money.5.c $(eg_src)/money.6.c
	cd $(eg_root); \
	diff -u src/money.5.c src/money.6.c > @abs_builddir@/$@ || test $$? -eq 1; \
	cd -;

check_money.1-2.c.diff: $(eg_tests)/check_money.1.c $(eg_tests)/check_money.2.c
	cd $(eg_root); \
	diff -u tests/check_money.1.c tests/check_money.2.c > @abs_builddir@/$@ || test $$? -eq 1; \
	cd -;

check_money.2-3.c.diff: $(eg_tests)/check_money.2.c $(eg_tests)/check_money.3.c
	cd $(eg_root); \
	diff -u tests/check_money.2.c tests/check_money.3.c > @abs_builddir@/$@ || test $$? -eq 1; \
	cd -;

check_money.3-6.c.diff: $(eg_tests)/check_money.3.c $(eg_tests)/check_money.6.c
	cd $(eg_root); \
	diff -u tests/check_money.3.c tests/check_money.6.c > @abs_builddir@/$@ || test $$? -eq 1; \
	cd -;

check_money.6-7.c.diff: $(eg_tests)/check_money.6.c $(eg_tests)/check_money.7.c
	cd $(eg_root); \
	diff -u tests/check_money.6.c tests/check_money.7.c > @abs_builddir@/$@ || test $$? -eq 1; \
	cd -;

# explicitly list every file in the example.

example_docs = example/Makefile.am \
               example/README \
               example/configure.ac

example_src_docs = example/src/Makefile.am \
                   example/src/main.c \
                   example/src/money.c \
                   example/src/money.h \
		   example/src/money.1.h \
		   example/src/money.2.h \
		   example/src/money.1.c \
		   example/src/money.3.c \
		   example/src/money.4.c \
		   example/src/money.5.c \
		   example/src/money.6.c

example_tests_docs = example/tests/Makefile.am \
                     example/tests/check_money.c \
                     example/tests/check_money.1.c \
                     example/tests/check_money.2.c \
                     example/tests/check_money.3.c \
                     example/tests/check_money.6.c \
                     example/tests/check_money.7.c

## what to clean

CLEANFILES = *~ *.diff

## what to distribute

EXTRA_DIST = $(example_docs) \
             $(example_src_docs) \
             $(example_tests_docs)

## what to install

docdir = $(datadir)/doc/$(PACKAGE)

# install money example

exampledir = $(docdir)/example
example_DATA = $(example_docs)

examplesrcdir = $(docdir)/example/src
examplesrc_DATA = $(example_src_docs)

exampletestsdir = $(docdir)/example/tests
exampletests_DATA = $(example_tests_docs)
