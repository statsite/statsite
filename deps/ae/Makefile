SOURCES=\
./ae.c \

CFLAGS   = -g -std=c99 -Wpointer-arith
LDFLAGS  =
CC = /usr/bin/gcc

OBJECTS=$(SOURCES:%.c=%.o)

all: libae.a

libae.a: $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

.PHONY: clean
clean:
	$(RM) *.o *.a
