/*
 * Filename: src/libmmv/mmv-explain.c
 * Library: libmmv
 * Brief: Helper functions to explain errors
 *
 * Copyright (C) 2016 Guy Shaw
 * Written by Guy Shaw <gshaw@acm.org>
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


/*
 * XXX To do: unify with explain-err.c
 */

#include <stdio.h>
#include <string.h>     // Import strlen()
#include <cscript.h>
#include <strbuf.h>

/**
 * @brief Explain that a pattern is too long; handle unsafe characters, etc.
 *
 * @param f        IN  Where to print
 * @param pattern  IN  Pattern to report -- either a _from_ or _to_ pattern.
 * @param maxlen   IN  The maximum pattern length allowed
 *
 * At this stage, some other code has already checked and determined that
 * the pattern is too long, so it knows maxlen.  Here, we take it as
 * a parameter, so in this source file, we do not need to bring in
 * mmv-specific include files.
 *
 */

void
fexplain_char_pattern_too_long(FILE *f, const char *pattern, size_t maxlen)
{
    size_t len;

    fputs("E: Pattern too long.\n", f);
    fputs("   ", f);
    fshow_strn(f, pattern, 40);
    fputs(" ...\n", f);
    len = strlen(pattern);
    fprintf(f, "   Pattern is %zu bytes long; limit is %zu.\n", len, maxlen);
}

/**
 * @brief Explain that a pattern is too long; handle unsafe characters, etc.
 *
 * @param f        IN  Where to print
 * @param pattern  IN  The pattern in question; a string buffer (|sbuf_t|)
 *
 * The |sbuf_t| representation of |pattern| has both the string contents
 * and the maximum size allowed.
 *
 */

void
fexplain_sbuf_pattern_too_long(FILE *f, sbuf_t *pattern)
{
    const char *s;
    size_t sz;
    size_t len;
    size_t chklen;

    s   = pattern->s;
    sz  = pattern->size;
    len = pattern->len;
    chklen = strlen(s);
    if (chklen != len) {
        fprintf(f, "INTERNAL ERROR:\n");
        fprintf(f, "   inconsistent length of pattern.\n");
        fprintf(f, "   pattern->len == %zu, strlen() == %zu.\n", len, chklen);
    }
    fexplain_char_pattern_too_long(f, s, sz);
}
