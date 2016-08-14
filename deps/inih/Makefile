SOURCES=\
./inih.c \

CFLAGS   = -O3
LDFLAGS  =
CC = /usr/bin/gcc

OBJECTS=$(SOURCES:%.c=%.o)

all: libinih.a

libinih.a: $(OBJECTS)
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
