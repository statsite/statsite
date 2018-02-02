## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS := subdir-objects
ACLOCAL_AMFLAGS = -I m4


SUBDIRS = deps/ae deps/inih deps/murmurhash man .


AM_CFLAGS = -g -std=gnu99 -pthread -D_GNU_SOURCE -DLOG_PERROR=0 -O3 -pthread -lm @LINK_TO_RT@ -Ideps/inih/ -Ideps/ae/ -Isrc/

#=========================================================
# statsite, the binary itself

statsite_SOURCES = \
       src/ascii_parser.c \
       src/hashmap.c \
       src/heap.c \
       src/radix.c \
       src/hll_constants.c \
       src/hll.c \
       src/set.c \
       src/cm_quantile.c \
       src/timer.c \
       src/counter.c \
       src/metrics.c \
       src/streaming.c \
       src/config.c \
       src/networking.c \
       src/conn_handler.c \
       src/statsite.c


statsite_LDADD = deps/ae/libae.a deps/inih/libinih.a deps/murmurhash/libmurmur.a

## Install directions:
bin_PROGRAMS = statsite

# This adds the sinks on make install, also allows for make uninstall if needed
nobase_pkgdata_DATA = sinks/*

configdir = /etc/statsite
nobase_dist_config_DATA = statsite.conf.example



## Stuff we want to include for the default dist task (not sdist)
EXTRA_DIST = . #everything

## Where to store the dist

distdir = ../$(PACKAGE)-$(VERSION)


#libcheck
if HAVE_CHECK
check_PROGRAMS = tests/runner
tests_runner_SOURCES = \
src/ascii_parser.c \
src/hashmap.c \
src/heap.c \
src/radix.c \
src/hll_constants.c \
src/hll.c \
src/set.c \
src/cm_quantile.c \
src/timer.c \
src/counter.c \
src/metrics.c \
src/streaming.c \
src/config.c \
src/networking.c \
src/conn_handler.c \
tests/runner.c

tests_runner_CFLAGS = -g -std=gnu99 -pthread -D_GNU_SOURCE -DLOG_PERROR=0 -O3 -pthread -lm @LINK_TO_RT@ -Ideps/inih/ -Ideps/ae/ -Isrc/
tests_runner_LDFLAGS = -g -pg -no-install
tests_runner_LDADD = deps/ae/libae.a deps/inih/libinih.a deps/murmurhash/libmurmur.a @CHECK_LIBS@
TESTS = tests/runner
TESTS_ENVIRONMENT = CK_FORK=yes
CK_VERBOSITY = verbose
CLEANFILES = tests/test_runner.log
endif


# Targets
test: integ

integ: all
	py.test integ/

sdist: clean
	mkdir -vp /tmp/statsite-@STATSITE_VERSION@
	cp -R . /tmp/statsite-@STATSITE_VERSION@
	tar -zcv --exclude='.git' --exclude='.gitignore' -f statsite-@STATSITE_VERSION@.tar.gz -C /tmp statsite-@STATSITE_VERSION@
	rm -rf /tmp/statsite-@STATSITE_VERSION@


clean-local: clean-local-check

.PHONY: clean-local-check
clean-local-check:
	test ! -f ".sconsign.dblite" || rm -f .sconsign.dblite
	test ! -d "deps/check-0.9.8/src/.libs" || rm -fr deps/check-0.9.8/src/.libs

RPMBUILDROOT=/tmp/rpm-build
rpms: sdist build
	mkdir -vp $(RPMBUILDROOT)
	cp -v *.tar.gz $(RPMBUILDROOT)
	cp -v rpm/statsite.spec $(RPMBUILDROOT)
	rpmbuild --define "_topdir $(RPMBUILDROOT)" \
        --define "_builddir %{_topdir}" \
        --define "_rpmdir %{_topdir}" \
        --define "_srcrpmdir %{_topdir}" \
        --define "_specdir %{_topdir}" \
        --define '_rpmfilename %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm' \
        --define "_sourcedir  %{_topdir}" \
        -ba $(RPMBUILDROOT)/statsite.spec

.PHONY: all test clean sdist build
