# Filename: src/Makefile
# Project: libmmv
# Brief: Top-lovel Makefile for libmmv -- build libraries and commands
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

.PHONY: all test clean show-targets

all:
	cd libcscript && make
	cd libmmv && make
	cd cmd && make

test:
	@cd cmd && make test | grep -v -E ': (Entering|Leaving) directory'

clean:
	cd libmmv && make clean
	cd libcscript && make clean
	cd cmd && make clean

show-targets:
	@show-makefile-targets

show-%:
	@echo $*=$($*)
