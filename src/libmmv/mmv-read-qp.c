/*
 * Filename: src/libmmv/mmv-read-qp.c
 * Library: libmmv
 * Brief: Read quote-printable encoded from->to filenames from a file.
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

#include <errno.h>          // for ENAMETOOLONG
#include <stdbool.h>        // for true
#include <stddef.h>         // for size_t
#include <stdio.h>          // for EOF, fprintf, FILE, fgetc, etc
#include <sys/types.h>      // for ssize_t
#include <string.h>         // Import strlen()
#include <dbgprint.h>

#define IMPORT_REP
#define IMPORT_PATTERN
#include <mmv-impl.h>  // for mmv_t, etc

extern void matchpat(mmv_t *mmv);

extern FILE *errprint_fh;
extern FILE *dbgprint_fh;

/**
 * @brief Read one quote-printable encoded line into a buffer.
 *
 * @param f     IN   File (stdio stream) to read from
 * @param buf   OUT  buffer to hold file name
 * @param sz    IN   maximum capacity of |buf|.
 * @param rlen  OUT  reference to return number of bytes read
 * @return errno-style status
 *
 */

static int
get_filename_qp(FILE *f, char *buf, size_t sz, size_t *rlen)
{
    extern ssize_t qp_decode_str(char *buf, size_t sz, char *str);
    size_t len;
    ssize_t qprv;
    int rv;
    int c;

    len = 0;
    rv = 0;
    while (rv == 0) {
        c = fgetc(f);
        if (len >= sz) {
            len = sz - 1;
            buf[len] = '\0';
            fexplain_char_pattern_too_long(stderr, buf, sz);
            rv = -ENAMETOOLONG;
        }
        else if (c == EOF) {
            rv = EOF;
            buf[len] = '\0';
        }
        else if (c == '\n') {
            if (len != 0 && buf[len - 1] == '\r') {
                --len;
            }
            buf[len] = '\0';
            break;
        }
        else {
            buf[len++] = c;
        }
    }

    /*
     * It is safe to qp_decode_str() in place, because the decoded string
     * is the same length or shorter at every stage during decoding.
     */
    qprv = qp_decode_str(buf, sz, buf);

    if (qprv < 0) {
        rv = -1;
    }
    *rlen = strlen(buf);
    return (rv);
}

/**
 * @brief Read in { from->to } pairs, one qp-encoded line at a time.
 *
 * @param mmv
 * @param f     IN   File (stdio stream) to read from
 * @return errno-style status (-errno or -1 or 0)
 *
 */

int
mmv_get_pairs_qp(mmv_t *mmv, FILE *f)
{
    int rv;
    size_t rlen;

    mmv->encoding = ENCODE_QP;
    while (true) {
        rv = get_filename_qp(f, mmv->from, MAXPATLEN, &rlen);
        if (rv == EOF) {
            rv = 0;
            break;
        }
        if (rv) {
            break;
        }
        mmv->fromlen = rlen;
        rv = get_filename_qp(f, mmv->to, MAXPATLEN, &rlen);
        if (rv == EOF) {
            rv = 0;
            break;
        }
        if (rv) {
            break;
        }
        mmv->tolen = rlen;
        dbg_printf("from='%s'\n", mmv->from);
        dbg_printf("to  ='%s'\n", mmv->to);

        matchpat(mmv);
        if (mmv->paterr) {
            return (-1);
        }
    }

    return (rv);
}
