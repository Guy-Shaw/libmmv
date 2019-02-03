# Filename: src/cmd/mmv-classic/test/Makefile
# Project: libmmv
#
# Copyright (C) 2016-2019 Guy Shaw
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

test:
	./test-01-collision
	./test-02-swap
	./test-03-cycles
	./test-04-backrefs
	./test-05-backref-zero

clean:
	rm -rf tmp tmp-*

show-targets:
	@show-makefile-targets

show-%:
	@echo $*=$($*)
