/*
 * Filename: src/libmmv/mmv-unlink.c
 * Library: libmmv
 * Brief: unlink a filename, report/explain errors.
 *
 * Original mmv code by Vladimir Lanin. et al.
 * See the file, LICENSE.md at the top of the libmmv tree.
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

#include <stdio.h>
#include <unistd.h>		// Import unlink()
#include <errno.h>		// Import errno
#include <eprint.h>

extern void eexplain_err(int err);
extern void eprint_filename(const char *fname);


/**
 * @brief Unlink a file name; report/explain any errors.
 *
 * @param  fname  IN  file name to be unlinked
 * @return errno-style status
 *
 */

int
mmv_unlink(const char *fname)
{
    int rv;
    int err;

    rv = unlink(fname);

    if (rv == 0) {
        return (0);
    }
    err = errno;
    eprint("unlink('");
    eprint_filename(fname);
    eprint("') failed\n");
    eexplain_err(err);
    return (rv);
}
