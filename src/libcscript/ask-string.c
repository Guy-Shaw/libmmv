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

#if defined(__GNUC__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE 1
#endif

#include <stddef.h>
    // Import constant NULL
#include <stdio.h>
    // Import type FILE
    // Import fflush()
    // Import fgets()
    // Import fputs()
    // Import var stderr
    // Import var stdin
#include <string.h>
    // Import strnlen()
#include <unistd.h>
    // Import type size_t

#include <eprint.h>

extern FILE *fopen_ttyw_fh(FILE *df);
extern FILE *fopen_ttyr_fh(FILE *df);

extern FILE *ttyw_fh;
extern FILE *ttyr_fh;

/**
 * @brief Write a prompt, then ask for a string.
 *
 * @param  prompt     IN  The prompt string to write before reading the answer
 * @param  ttyw       IN  stdio stream, where to write prompt
 * @param  ttyr       IN  stdio stream, where to read response
 * @param  buf        OUT Receives the response read from the terminal
 * @param  bufsz      IN  Maximum capacity of |buf|
 * @return buf or NULL, if something went wrong
 *
 */

char *
ask_string_tty(FILE *ttyw, FILE *ttyr, const char *prompt, char *buf, size_t bufsz)
{
    char *rbuf;
    size_t len;

    fputs(prompt, ttyw);
    fflush(ttyw);
    rbuf = fgets(buf, bufsz, ttyr);
    if (rbuf == NULL) {
        return (NULL);
    }
    len = strnlen(buf, bufsz);
    if (buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    }
    return (buf);
}


char *
ask_string(const char *prompt, char *buf, size_t bufsz)
{

    if (ttyw_fh == NULL) {
        fopen_ttyw_fh(stderr);
    }

    if (ttyr_fh == NULL) {
        fopen_ttyr_fh(stdin);
    }

    if (ttyw_fh == NULL || ttyr_fh == NULL) {
        return (NULL);
    }

    return (ask_string_tty(ttyw_fh, ttyr_fh, prompt, buf, bufsz));
}
