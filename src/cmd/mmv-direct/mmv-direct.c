/*
 * Filename: src/cmd/mmv-direct/mmv-direct.c
 * Project: libmmv
 * Brief: Trivial program illustrating how to call libmmv functions from C code
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
#include <stdlib.h>     // Import getenv()
#include <dbgprint.h>
#include <mmv.h>

extern void set_debug_fh(const char *dbg_fname);

FILE *errprint_fh;
FILE *dbgprint_fh;

/**
 * @brief Show how libmmv can be used directly from C code.
 *
 * C code can get the benefits of all the safety features of mmv,
 * by calling libmmv functions directly.  Safety features, such
 * as the analysis of all from-to pairs, cycle detection and
 * collision detection are still available.
 *
 * But, it is not necessary to call patgen().
 * A set of raw from-to pairs can be added to an mmv context
 * by calling mmv_add_pair() as many times as is needed,
 * before calling mmv_pairs().
 *
 */

int
main(int argc, char *const *argv)
{
    mmv_t *mmv;
    int err;

    set_debug_fh(getenv("MMV_DEBUG"));
    errprint_fh = stderr;
    mmv = mmv_new();

    if (argc == 3) {
        err = mmv_add_pair(mmv, argv[1], argv[2]);
    }
    else {
        err = mmv_add_pair(mmv, "tmp-01", "TMP-01");
    }
    if (err) {
        return (err);
    }

    return (mmv_pairs(mmv));
}
