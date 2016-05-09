
SOURCES := $(wildcard *.c)
OBJECTS := $(patsubst %.c, %.o, $(SOURCES))

CC := gcc
CPPFLAGS := -I../inc
CFLAGS := -std=c99 -Wall -Wextra -g

.PHONY: all install clean show-targets

all: libcscript.a

libcscript.a: $(OBJECTS)
	ar crv libcscript.a $(OBJECTS)

clean:
	rm -f libcscript.a $(OBJECTS) *.o

show-targets:
	@show-makefile-targets

__show-%:
	@echo $*=$($*)
