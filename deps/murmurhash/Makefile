SOURCES=\
./MurmurHash3.c \

CFLAGS   = -std=c99 -O3
LDFLAGS  =
CC = /usr/bin/gcc

OBJECTS=$(SOURCES:%.c=%.o)

all: libmurmur.a

libmurmur.a: $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

.PHONY: clean
clean:
	$(RM) *.o *.a

check: all
test: all
dist:
distdir:
distclean: clean
install:
uninstall:
