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

distclean: clean
install:
uninstall:
