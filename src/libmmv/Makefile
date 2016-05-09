
LIBRARY := libmmv
SOURCES := $(wildcard *.c)
OBJECTS := $(patsubst %.c, %.o, $(SOURCES))

CC := gcc
CONFIG := -DDEBUG
CPPFLAGS := -I../inc
CFLAGS += -std=c99 -g -Wall -Wextra $(CONFIG)

.PHONY: all clean

all: $(LIBRARY).a

$(LIBRARY).a: $(OBJECTS)
	ar crv $(LIBRARY).a $(OBJECTS)

clean:
	rm -f $(LIBRARY).a $(OBJECTS) *.o
	cscope-clean
