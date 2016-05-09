/*
 * Filename: src/libmmv/ask-filename.c
 * Library: libmmv
 * Brief: Prompt, then ask for a filename, then open that file
 *
 * Original mmv code by Vladimir Lanin. et al.
 * See the the file, LICENSE.md at the top of the libmmv tree.
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

#define _GNU_SOURCE 1

#include <stdio.h>
#include <string.h>     // Import strnlen()


/**
 * @brief Write a prompt, then ask for a filename and open that file.
 *
 * @param  prompt     IN  The prompt string to write before reading the answer
 * @param  ttyw       IN  stdio stream, where to write prompt
 * @param  ttyr       IN  stdio stream, where to read response
 * @param  fname_buf  OUT Receives the response read from the terminal
 * @param  bufsz      IN  Maximum capacity of |fname_buf|
 * @return a stdio stream (FILE *)  or  NULL, if something went wrong
 *
 */

FILE *
ask_filename_tty(FILE *ttyw, FILE *ttyr, const char *prompt, char *fname_buf, size_t bufsz)
{
    FILE *f;
    char *rbuf;
    size_t len;

    fputs(prompt, ttyw);
    rbuf = fgets(fname_buf, bufsz, ttyr);
    if (rbuf == NULL) {
        return (NULL);
    }
    len = strnlen(fname_buf, bufsz);
    if (fname_buf[len - 1] == '\n') {
        fname_buf[len - 1] = '\0';
    }
    f = fopen(fname_buf, "w");
    return (f);
}

/*
 * XXX TO DO
 * XXX -----
 * XXX Probe to determine if {ttyw==stderr, ttyr=stdin} are associated
 * XXX with a terminal.  Possibly open other streams for reading and
 * XXX writing to/from /dev/tty.
 * XXX
 * XXX One possible conflict is that we are reading {from->to} filename pairs
 * XXX from stdin, so we cannot count on stdin being connected to a terminal.
 * XXX In any case, we should not assume {stderr, stdin}.
 *
 */

FILE *
ask_filename(const char *prompt, char *fname_buf, size_t bufsz)
{
    return (ask_filename_tty(stderr, stdin, prompt, fname_buf, bufsz));
}
