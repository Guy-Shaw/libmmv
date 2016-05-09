/*
 * Filename: fshow-strn.c
 * Library: libcscript
 * Brief: Write a graphic representation of a string to an stdio stream
 *
 * Description:
 *   Given a single string, write it to the given stdio stream,
 *   using only "safe" graphic characters, so that nothing is hidden
 *   and nothing can make a terminal misbehave.
 *
 *   fshow_strn takes a maximum length.  This is good for restricting
 *   the length of the displayed string.  Not that the limit is not
 *   on the length of the inout string, but is on the number of characters
 *   shown, taking into account any decoded characters that require
 *   more than one byte to render.
 *
 * Copyright (C) 2015-2016 Guy Shaw
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

#define _GNU_SOURCE 1

#include <stdio.h>
#include <sys/types.h>
#include <string.h>         // Import strlen()

extern size_t show_char_r(char *buf, size_t sz, int chr);

/**
 * @brief Write a graphic representation of a string to an stdio stream
 * @param f    IN  Write to this FILE.
 * @param str  IN  The string to be represented.
 * @return void
 *
 * Description:
 *   Given a single string, write it to the given stdio stream,
 *   using only "safe" graphic characters, so that nothing is hidden
 *   and nothing can make a terminal misbehave.
 */
void
fshow_strn(FILE *f, char *str, size_t maxlen)
{
    char dbuf[8];
    char *s;
    int chr;
    size_t dlen;

    dlen = 0;
    for (s = str; (chr = *s) != '\0'; ++s) {
        show_char_r(dbuf, sizeof (dbuf), chr);
        dlen += strlen(dbuf);
        if (dlen > maxlen) {
            break;
        }
        fputs(dbuf, f);
    }
}
