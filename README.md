# libmmv -- mmv unleashed

### Enhanced version of `mmv`

`libmmv` is a library that implements all of the
functionality of the classic program, `mmv`.
`mmv` is a bulk rename command-line utility
that has been around since about 1991.

The new `mmv` is re-written as a library so that
much of the functionality of `mmv` is accessible
to other programs.  The new implementation of
the `mmv` program is roughly a dozen lines of code
that calls `libmmv` functions to initialize, configure,
and run `libmmv` services.

### Other improvements

Besides just being a rewrite,
other cpabilities were added.

There are options for dealing with other ways of
encoding { from -> to } pairs of filenames.
The classic `mmv` syntax for _form_ patterns
and _to_ patterns is now a special case.
Other encodings are 'null', 'quoted-printable',
and 'xnn'.  A file consisting of { from `->` to }
pairs can be sent to the `mmv-pairs` program,
which does not interpret any characters of either
the _from_ or the _to_ filenames.

This is mostly useful for use by other programs.
A program can use its own pattern matching and
substitution functions to calculate the bulk
rename/move/copy operations it wants; then,
that list of raw _from_ `->` _to_ pairs can be
fed into `mmv-pairs`, which will do all of the
important safety checks.


## Examples

### Use Perl script and `mmv-pairs`

```
ls *.pdf \
| perl -e 'while (<>) { chomp; print $_, "\000"; s/\s+/-/g; print $_, "\000"; }' \
| mmv-pairs

```

The perl script takes file names in,
one per line,
changes it it some way,
then write two lines for each input line,
the _before_ and _after_.

`mmv-pairs` read two lines at a time.
The default encoding is --null.

## License

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.


-- Guy Shaw

   gshaw@acm.org

