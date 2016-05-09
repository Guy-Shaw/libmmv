/*
 * Filename: src/libmmv/mmv-getpat.c
 * Library: libmmv
 * Brief: Get a { from->to } pair of patterns from stdin
 *
 * Original mmv code by Vladimir Lanin. et al.
 * See the file, LICENSE.md at the top of the libmmv tree.
 *
 * Modifications for libmmv:
 *   Copyright (C) 2016 Guy Shaw
 *   Written by Guy Shaw <gshaw@acm.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctype.h>      // for isspace
#include <stdbool.h>    // for bool, false, true
#include <stddef.h>     // for size_t
#include <stdio.h>      // for EOF, printf, getchar, etc
#include <string.h>     // for strcmp

#define IMPORT_PATTERN
#define IMPORT_RFLAGS
#define IMPORT_REP
#define IMPORT_FILEINFO
#define IMPORT_ALLOC
#include <mmv-impl.h>

/**
 * @brief getchar() with sticky EOF
 *
 */

static int
mygetc(void)
{
    static int lastc = 0;

    if (lastc == EOF) {
        return (EOF);
    }
    return (lastc = getchar());
}

/**
 * @brief Read a whitespace-word from stdin, into a given buffer.
 *
 * @param buf  Buffer to receive the word
 * @param sz   Capacity of the buffer
 * @return     Length of word
 *
 * Buffer will contain ASCII-NUL terminated word.
 *
 * Failure is not an option.  That is, no error status is returned.
 * On any error, an error message is printed to stdout,
 * and another attempt is made to read a word from stdin.
 *
 */

size_t
getword(char *buf, size_t sz)
{
    size_t len;
    int c, prevc;
    char *p;

    p = buf;
    prevc = ' ';

    // Read from stdin until we get a full word.
    //
    len = 0;
    while ((c = mygetc()) != EOF && (prevc == ESC || !isspace(c))) {
        if (len < sz - 1) {
            *(p++) = c;
            ++len;
        }
        prevc = c;
    }
    *p = '\0';
    if (len >= sz - 1) {
        fexplain_char_pattern_too_long(stderr, buf, sz);
    }

    // Consume and discard a run of whitespace
    while (c != EOF && isspace(c) && c != '\n') {
        c = mygetc();
    }

    if (c != EOF) {
        ungetc(c, stdin);
    }

    return (len);
}

/*
 * The output of a previous run of mmv can be fed into another instance
 * of mmv.  So, any leading marks of the form { -> -^ => =^ } are
 * ignored.  Test for those leading marks.
 */

static inline int
is_rescan(char *s, size_t sz)
{
    return (sz == 2 && (s[0] == '-' || s[0] == '=') && (s[1] == '>' || s[1] == '^'));
}

/**
 * @brief Get a { from->to } pair of patterns from stdin
 *
 */

int
getpat(mmv_t *mmv)
{
    int c;
    bool gotit;
    char extra[MAXPATLEN];

    mmv->patflags = 0;
    gotit = false;
    do {
        if ((mmv->fromlen = getword(mmv->from, mmv->fromsz)) == 0 || mmv->fromlen >= mmv->fromsz) {
            goto nextline;
        }

        do {
            if ((mmv->tolen = getword(mmv->to, mmv->tosz)) == 0) {
                printf("%s -> ? : missing replacement pattern.\n", mmv->from);
                goto nextline;
            }
            if (mmv->tolen >= mmv->tosz) {
                goto nextline;
            }
        } while (is_rescan(mmv->to, mmv->tolen));

        if (getword(extra, sizeof (extra)) == 0) {
            gotit = true;
        }
        else if (strcmp(extra, "(*)") == 0) {
            mmv->patflags |= R_DELOK;
            gotit = (getword(extra, sizeof (extra)) == 0);
        }

      nextline:
        while ((c = mygetc()) != '\n' && c != EOF);
        if (c == EOF) {
            return (0);
        }
    } while (!gotit);

    return (1);
}
