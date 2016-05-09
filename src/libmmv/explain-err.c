/*
 * Filename: src/libmmv/explain-err.c
 * Library: libmmv
 * Brief: A collection of functions that aid in explaining errors.
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

#include <stdio.h>
#include <errno.h>		// Import errno
#include <ctype.h>		// Import isprint()
#include <eprint.h>

extern char *decode_esym_r(char *buf, size_t bufsz, int err);
extern char *decode_emsg_r(char *buf, size_t bufsz, int err);

/*
 *
 * XXX Transition
 * --------------
 * libmmv is being converted from relatively simple calls to *printf()
 * functions to functions that are a bit more defensive, and capable
 * of explaining more.  As of 2016-05-01, this process is not complete.
 *
 * Some of the helper functions decode errno values or signals.
 * Others print filenames in a way that send only safe graphic characters
 * to a terminal, and in other ways are protected against bad data.
 * 
 */



/**
 * @brief Print a given filename using all safe graphic characters.
 *
 * @param  f     IN  The stdio stream to print to
 * @param  fname IN  The file name to be printed
 *
 * Any characters considered to be unsafe are translated to \xnn
 * before printing.
 *
 */

/*
 * XXX Use libcscript function to print a filename.
 */

void
fprint_filename(FILE *f, const char *fname)
{
    const char *p;
    int chr;

    p = fname;
    if (p == NULL) {
        p = "(null)";
    }

    while ((chr = *p) != '\0') {
        if (isprint(chr)) {
            fputc(chr, f);
        }
        else {
            fprintf(f, "%%%02x", chr);
        }
        ++p;
    }
}

/**
 * @brief print a filename, safely, to stdout.
 *
 */

void
print_filename(const char *fname)
{
    fprint_filename(stdout, fname);
}


/**
 * @brief print a filename, safely, to stderr.
 *
 */

void
eprint_filename(const char *fname)
{
    fprint_filename(stderr, fname);
}


/**
 * @brief Like perror(), but show symbolic name and explanation of given errno.
 * @param  f     IN  The stdio stream to print to
 * @param  err   IN  errno-like number to be explained
 *
 */

void
fexplain_err(FILE *f, int err)
{
    char estrbuf[100];
    char *emsgp;
    char esymbuf[32];
    char *esymp;

    emsgp = decode_emsg_r(estrbuf, sizeof (estrbuf), err);
    esymp = decode_esym_r(esymbuf, sizeof (esymbuf), err);
    fprintf(f, "    %d=%s='%s'\n", err, esymp, emsgp);
}

/**
 * @brief Like fexplain_err() but default to sdtout
 *
 */

void
explain_err(int err)
{
    fexplain_err(stdout, err);
}

/**
 * @brief Like fexplain_err() but default to sdterr
 *
 */

void
eexplain_err(int err)
{
    fexplain_err(stderr, err);
}
