/*
 * Filename: src/cmd/mmv-torture/mmv-torture.c
 * Project: libmmv
 * Brief: Stress libmmv.  Run test that would be difficult to test, normally.
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
#include <string.h>	// Import memset()
#include <dbgprint.h>
#include <cscript.h>
#include <mmv.h>


/*
 * XXX To do: Have libmmv provide some configurable values, such PATH_MAX
 * XXX Or, rather, something equivalent.  The value could be arrived at
 * XXX by interrogating some configuration at run time, rather than use
 * XXX a compile-time constant.
 */

#include <linux/limits.h>       // Import PATH_MAX

extern void set_debug_fh(const char *dbg_fname);

FILE *errprint_fh;
FILE *dbgprint_fh;

/**
 * @brief Stress libmmv.  Run test that would be difficult to test, normally.
 *
 * Most testing of libmmv functionality can be accomplished by running
 * 'mmv' or 'mmv-pairs' with suitable inputs.  But, some corner cases
 * are easier to synthesize by running a special test program, designed
 * to do unusual things.
 *
 * For now, the only special test is to construct a very long path
 * and test to make sure libmmv reports the problem correctly, and
 * does not just die, or otherwise misbehave..
 *
 */

static int
test_path_too_long(void)
{
    mmv_t *mmv;
    int err;
    char *src_pattern;
    size_t big = PATH_MAX + 100;
    size_t len;
    char *p;

    src_pattern = guard_malloc(big);

    for (p = src_pattern, len = 0; len < big; p += 64, len += 64) {
        memset(p, 'A', 64);
        p[63] = '/';
    }
    p[63] = '\0';

    mmv = mmv_new();
    err = mmv_add_pair(mmv, src_pattern, "TMP-01");
    if (err) {
        return (err);
    }

    return (mmv_pairs(mmv));
}

int
main(int argc, char *const *argv)
{
    int rv;

    set_debug_fh(getenv("MMV_DEBUG"));
    errprint_fh = stderr;

    fshow_str_array(stderr, argc, argv);
    rv = test_path_too_long();
    return (rv);
}
