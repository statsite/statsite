## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS := subdir-objects
ACLOCAL_AMFLAGS = -I m4


SUBDIRS = deps/ae deps/inih deps/murmurhash .


CLEANFILES = test_runner

AM_CFLAGS = -g -std=gnu99 -pthread -D_GNU_SOURCE -DLOG_PERROR=0 -O3 -pthread -lm @LINK_TO_RT@ -Ideps/inih/ -Ideps/ae/ -Isrc/

#=========================================================
# statsite, the binary itself

src_statsite_SOURCES = \
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


src_statsite_LDADD = deps/ae/libae.a deps/inih/libinih.a deps/murmurhash/libmurmur.a


# Standard compiler options from the scons file, for reference
#CFLAGS = '-g -std=c99 -D_GNU_SOURCE -Wall -Werror -Wstrict-aliasing=0 -O3 -pthread -Ideps/inih/ -Ideps/ae/ -Isrc/'
#CFLAGS_WITHOUT_ERR = '-g -std=c99 -D_GNU_SOURCE -O3 -pthread -Ideps/inih/ -Ideps/ae/ -Isrc/'

# Compiler options for SunOS from the scons file
##    statsite_with_err_cc_flags = '-g -std=gnu99 -D_GNU_SOURCE -DLOG_PERROR=0 -Wall -Wstrict-aliasing=0 -Wformat=0 -O3 -pthread -Ideps/inih/ -Ideps/ae/ -Isrc/'
##    statsite_without_err_cc_flags = '-g -std=gnu99 -D_GNU_SOURCE -DLOG_PERROR=0 -O3 -pthread -Ideps/inih/ -Ideps/ae/ -Isrc/'



## Install directions:
# This makes statsite the result for make install, also allows for make uninstall if needed
bin_PROGRAMS = src/statsite

# This adds the sinks on make install, also allows for make uninstall if needed
nobase_pkgdata_DATA = sinks/*


## Stuff we want to include for the default dist task (not sdist)
EXTRA_DIST = .

# Targets
scons-build:
	scons statsite

test:
	scons test_runner
	./test_runner

integ: build test
	py.test integ/

sdist: clean
	mkdir -vp /tmp/statsite-@STATSITE_VERSION@
	cp -R . /tmp/statsite-@STATSITE_VERSION@
	tar -zcv --exclude='.git' --exclude='.gitignore' -f statsite-@STATSITE_VERSION@.tar.gz -C /tmp statsite-@STATSITE_VERSION@

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
