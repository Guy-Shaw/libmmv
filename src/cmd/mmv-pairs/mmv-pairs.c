/*
 * Filename: src/cmd/mmv-pairs/mmv-pairs.c
 * Project: libmmv
 * Brief: Like mmv, but just takes in raw from->to filenames, not patterns.
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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>		// Import strcmp()
#include <stdlib.h>		// Import abort(), getenv()
#include <dbgprint.h>
#include <cscript.h>
#include <mmv.h>

extern void mmv_set_default_options(mmv_t *mmv);

extern void set_debug_fh(const char *dbg_fname);
extern void fdump_mmv_op(FILE *f, mmv_t *mmv);

enum opt_encode {
    OPT_ENCODE_NUL,
    OPT_ENCODE_QP,
    OPT_ENCODE_XNN,
};


FILE *errprint_fh;
FILE *dbgprint_fh;

static enum opt_encode encoding = OPT_ENCODE_NUL;

/*
 * XXX To do:
 * XXX   Use getopt_long.
 * XXX   Parse all encodings and other options.
 * XXX   Except do not take --encoding=pattern
 */

int
main(int argc, char *const *argv)
{
    mmv_t *mmv;
    int err;

    errprint_fh = stderr;

    mmv = mmv_new();
    mmv_set_default_options(mmv);
    mmv_setopt(mmv, 'x');
    mmv_setopt(mmv, 'D');
    if (argc >= 2 && strcmp(argv[1], "--qp") == 0) {
        encoding = OPT_ENCODE_QP;
    }

    switch (encoding) {
    default:
        abort();
        break;
    case OPT_ENCODE_NUL:
        err = mmv_get_pairs_nul(mmv, stdin);
        break;
    case OPT_ENCODE_QP:
        err = mmv_get_pairs_qp(mmv, stdin);
        break;
    }

    if (err) {
        return (err);
    }

    if (dbgprint_fh) {
        fputs("op=", dbgprint_fh);
        fdump_mmv_op(dbgprint_fh, mmv);
    }

    return (mmv_pairs(mmv));
}
