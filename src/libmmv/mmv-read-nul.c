/*
 * Filename: src/libmmv/mmv-read-nul.c
 * Library: libmmv
 * Brief: Read nul-terminated from->to filenames from a file.
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

#include <errno.h>      // for ENAMETOOLONG
#include <stdbool.h>    // for true
#include <stddef.h>     // for size_t
#include <stdio.h>      // for EOF, fprintf, FILE, fgetc, etc
#include <dbgprint.h>

#define IMPORT_REP
#define IMPORT_PATTERN
#include <mmv-impl.h>  // for mmv_t, etc

extern FILE *errprint_fh;
extern FILE *dbgprint_fh;

/**
 * @brief Read one nul-terminated record into a buffer.
 *
 * @param f     IN   File (stdio stream) to read from
 * @param buf   OUT  buffer to hold file name
 * @param sz    IN   maximum capacity of |buf|.
 * @param rlen  OUT  reference to return number of bytes read
 * @return errno-style status
 *
 */

static int
get_filename_nul(FILE *f, char *buf, size_t sz, size_t *rlen)
{
    size_t len;
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
        else {
            buf[len++] = c;
            if (c == '\0') {
                break;
            }
        }
    }

    *rlen = len;
    return (rv);
}

/**
 * @brief Read in { from->to } pairs, one nul-terminated record at a time
 *
 * @param mmv
 * @param f     IN   File (stdio stream) to read from
 * @return errno-style status (-errno or -1 or 0)
 *
 */

int
mmv_get_pairs_nul(mmv_t *mmv, FILE *f)
{
    int rv;
    size_t rlen;

    mmv->encoding = ENCODE_NUL;
    while (true) {
        mmv->paterr = 0;
        rv = get_filename_nul(f, mmv->from, MAXPATLEN, &rlen);
        if (rv == EOF) {
            rv = 0;
            break;
        }
        if (rv) {
            break;
        }
        mmv->fromlen = rlen;
        rv = get_filename_nul(f, mmv->to, MAXPATLEN, &rlen);
        if (rv == EOF) {
            rv = 0;
            break;
        }
        if (rv) {
            break;
        }
        mmv->tolen = rlen;
        dbg_println_str("from=", mmv->from);
        dbg_println_str("to  =", mmv->to);

        if (mmv->fromlen >= MAXPATLEN) {
            fexplain_char_pattern_too_long(stderr, mmv->from, MAXPATLEN);
            mmv->paterr = 1;
        }

        if (mmv->tolen >= MAXPATLEN) {
            fexplain_char_pattern_too_long(stderr, mmv->to, MAXPATLEN);
            mmv->paterr = 1;
        }

        if (mmv->paterr) {
            return (mmv->paterr);
        }

        extern int parse_src_fname(mmv_t *mmv);
        extern int parse_dst_fname(mmv_t *mmv);
        int err;

        err = parse_src_fname(mmv);
        if (err) {
            mmv->paterr = 1;
            return (err);
        }

        err = parse_dst_fname(mmv);
        if (err) {
            mmv->paterr = 1;
            return (err);
        }

        if (dostage_fnames(mmv, mmv->from, mmv->pathbuf, 0, 0)) {
            printf("%s -> %s : no match.\n", mmv->from, mmv->to);
            mmv->paterr = 1;
        }

        if (mmv->debug_fh) {
            fdump_all_replacement_structures(mmv->debug_fh, &mmv->hrep);
        }
    }

    return (rv);
}
