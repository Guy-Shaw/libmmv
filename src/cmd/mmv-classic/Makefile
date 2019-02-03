# Filename: src/cmd/mmv-classic/Makefile
# Project: libmmv
# Brief: Build main program for the classic mmv program
#
# Copyright (C) 2016 Guy Shaw
# Written by Guy Shaw <gshaw@acm.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

PROGRAM := mmv
SRCS = $(PROGRAM).c
OBJS = $(PROGRAM).o
LIBS := ../../libmmv/libmmv.a  ../../libcscript/libcscript.a -lbsd

CC := gcc
CONFIG := -DSYSV -DDIRENT -DRENAME
CFLAGS := -g -Wall -Wextra
CPPFLAGS := -I../../inc

.PHONY: all test install clean-test clean

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(CONFIG) $(OBJS) $(LIBS)

test: $(PROGRAM)
	@cd test && make test

install: all
	@echo "Install mmv according to local convention,"
	@echo "then make links named mcp, mad, and mln to mmv."
	@echo "Under System V, edit mmv.1 to uncomment the .nr O 1 line."

clean-test:
	cd test && make clean

clean: clean-test
	rm -f core a.out mmv *.o *.a
	rm -f test_?? T.??
	rm -f *,FAILED

show-targets:
	@show-makefile-targets

show-%:
	@echo $*=$($*)
