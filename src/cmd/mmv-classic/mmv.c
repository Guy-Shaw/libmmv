/*
 * Filename: src/cmd/mmv-classic/mmv.c
 * Project: libmmv
 * Brief: Main program to mimic classic mmv command using libmmv
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
#include <cscript.h>
#include <mmv.h>

extern void fdump_mmv_op(FILE *f, mmv_t *mmv);

const char *program_path;
const char *program_name;

FILE *errprint_fh;
FILE *dbgprint_fh;

/**
 * @brief Run libmmv with encoding==ENCODE_PAT, mimic the classic mmv program.
 *
 * Classic 'mmv' gathered patterns either from the command-line arguments
 * or from a file.  From->to pattern pairs were interpreted using the
 * encoding for patterns that libmmv calls ENCODE_PAT, which means that
 * wildcard characters are interpreted in the 'from' pattern
 * and back-references are interpreted in the 'to' pattern.
 *
 * libmmv separates the gathering up of { from->to } pattern pairs
 * from the analysis and execution of the move operations.
 *
 * All that is needed to get libmmv to do the same is to:
 *   1) create a context, mmv_t, 
 *   2) call patgen() to get patterns in the manner of ENCODE_PAT,
 *   3) call mmv_execute(mmv) to analyze the pairs and do the actual moves.
 *
 * Other programs can use libmmv in other ways, using other encodings,
 * using other means to creat raw from->to pairs.
 *
 */

int
main(int argc, char *const *argv)
{
    mmv_t *mmv;
    int err;

    set_eprint_fh();
    set_debug_fh(getenv("MMV_DEBUG"));
    program_path = *argv;
    program_name = sname(program_path);
    mmv = mmv_new();
    mmv_init_patgen(mmv);
    err = patgen(mmv, argc, argv);
    if (err) {
        return (err);
    }

    if (dbgprint_fh) {
        fputs("op=", dbgprint_fh);
        fdump_mmv_op(dbgprint_fh, mmv);
    }
    return (mmv_execute(mmv));
}
