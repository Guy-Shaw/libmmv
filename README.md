# libmmv -- mmv unleashed

### Enhanced version of `mmv`

`libmmv` is a library that implements all of the
functionality of the classic program, `mmv`.
`mmv` is a bulk rename command-line utility
that has been around since about 1990.

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
then writes two lines for each input line,
the _before_ and _after_.

`mmv-pairs` reads two lines at a time.
The default encoding is --null.

## License

See the file `LICENSE.md`

-- Guy Shaw

   gshaw@acm.org

