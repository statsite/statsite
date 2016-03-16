AUTOMAKE_OPTIONS := subdir-objects
ACLOCAL_AMFLAGS := -I m4


SUBDIRS = deps/ae deps/inih deps/murmurhash .

bin_PROGRAMS = src/statsite

AM_CFLAGS = -g -std=gnu99 -pthread -D_GNU_SOURCE -DLOG_PERROR=0 -O3 -pthread -lm -lrt -Ideps/inih/ -Ideps/ae/ -Isrc/

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


# Compiler options from the scons file, not using them right now

#CFLAGS = '-g -std=c99 -D_GNU_SOURCE -Wall -Werror -Wstrict-aliasing=0 -O3 -pthread -Ideps/inih/ -Ideps/ae/ -Isrc/'
#CFLAGS_WITHOUT_ERR = '-g -std=c99 -D_GNU_SOURCE -O3 -pthread -Ideps/inih/ -Ideps/ae/ -Isrc/'

# Compiler options for SunOS from the scons file
##    statsite_with_err_cc_flags = '-g -std=gnu99 -D_GNU_SOURCE -DLOG_PERROR=0 -Wall -Wstrict-aliasing=0 -Wformat=0 -O3 -pthread -Ideps/inih/ -Ideps/ae/ -Isrc/'
##    statsite_without_err_cc_flags = '-g -std=gnu99 -D_GNU_SOURCE -DLOG_PERROR=0 -O3 -pthread -Ideps/inih/ -Ideps/ae/ -Isrc/'

.PHONY: all test clean sdist
