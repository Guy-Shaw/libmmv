# Filename: src/cmd/Makefile
# Project: libmmv
# Brief: Build all commands
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

COMMANDS := mmv-classic mmv-pairs mmv-direct mmv-torture

.PHONY: all test clean show-targets

all:
	for cmd in $(COMMANDS) ; do ( cd $$cmd && make $@) || break; done

test:
	for cmd in $(COMMANDS) ; do ( cd $$cmd && make $@ ) || break; done

clean:
	for cmd in $(COMMANDS) ; do ( cd $$cmd && make $@) || break; done

show-targets:
	@show-makefile-targets

show-%:
	@echo $*=$($*)
