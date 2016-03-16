AUTOMAKE_OPTIONS := subdir-objects
ACLOCAL_AMFLAGS := -I m4



SUBDIRS = deps/ae deps/inih deps/murmurhash .

bin_PROGRAMS = src/statsite

AM_CFLAGS = -g -std=gnu99 -pthread -D_GNU_SOURCE -DLOG_PERROR=0 -O3 -pthread -lm -lrt -Ideps/inih/ -Ideps/ae/ -Isrc/

#=========================================================
# statsite, the binary itself

src_statsite_SOURCES = \
       src/cm_quantile.c \
       src/cm_quantile.h \
       src/config.c \
       src/config.h \
       src/conn_handler.c \
       src/conn_handler.h \
       src/counter.c \
       src/counter.h \
       src/hashmap.c \
       src/hashmap.h \
       src/heap.c \
       src/heap.h \
       src/hll.c \
       src/hll_constants.c \
       src/hll_constants.h \
       src/hll.h \
       src/metrics.c \
       src/metrics.h \
       src/networking.c \
       src/networking.h \
       src/radix.c \
       src/radix.h \
       src/set.c \
       src/set.h \
       src/statsite.c \
       src/streaming.c \
       src/streaming.h \
       src/timer.c \
       src/timer.h

src_statsite_LDADD = deps/ae/libae.a deps/inih/libinih.a deps/murmurhash/libmurmur.a

# examples seem to add $(EXT_A_LIBS) $(EXT_B_LIBS) as well

# Compiler options from the scons file, not using them right now

#CFLAGS = '-g -std=c99 -D_GNU_SOURCE -Wall -Werror -Wstrict-aliasing=0 -O3 -pthread -Ideps/inih/ -Ideps/ae/ -Isrc/'
#CFLAGS_WITHOUT_ERR = '-g -std=c99 -D_GNU_SOURCE -O3 -pthread -Ideps/inih/ -Ideps/ae/ -Isrc/'

# Compiler options for SunOS from the scons file
##    statsite_with_err_cc_flags = '-g -std=gnu99 -D_GNU_SOURCE -DLOG_PERROR=0 -Wall -Wstrict-aliasing=0 -Wformat=0 -O3 -pthread -Ideps/inih/ -Ideps/ae/ -Isrc/'
##    statsite_without_err_cc_flags = '-g -std=gnu99 -D_GNU_SOURCE -DLOG_PERROR=0 -O3 -pthread -Ideps/inih/ -Ideps/ae/ -Isrc/'

.PHONY: all test clean sdist
