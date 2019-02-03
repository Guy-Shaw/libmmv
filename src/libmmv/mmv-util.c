/*
 * Filename: src/libmmv/mmv-util.c
 * Library: libmmv
 * Brief: Miscellaneous helper functions / signal handlers
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
// XXX #include <signal.h>	// Import sighandler_t
#include <stdlib.h>	// Import exit()

#include <eprint.h>

typedef void sigreturn_t;	// XXX Unify signal typedefs

extern int gotsig;

/*
 * Provide quit() function.
 * Memory allocator functions quit() on error.
 */


/*
 * XXX Note: quit() says nothing done.
 * It should either check that nothing was done
 * or it should be enforced that quit() is called only
 * on errors detected _before_ anything was done.
 */

void
quit(void)
{
    fflush(stdout);
    fflush(stderr);
    fprintf(stderr, "Aborting, nothing done.\n");
    fflush(stderr);
    exit(1);
}

/*
 * XXX To do: explain signal value.  Use decode_signal_r()
 *            See libcscript::decode-signal.c
 *
 */

void
sig_report(int s)
{
    fflush(stdout);
    fflush(stderr);
    fprintf(stderr, "Got signal %d.\n", s);
    fflush(stderr);
}

sigreturn_t
breakout(int s)
{
    sig_report(s);
    eprint("Aborting, nothing done.\n");
    exit(1);
}

sigreturn_t
breakrep(int s)
{
    sig_report(s);
    gotsig = 1;
}

sigreturn_t
breakstat(int s)
{
    sig_report(s);
    exit(1);
}

void
mmv_abort(void)
{
    fflush(stdout);
    fflush(stderr);
    fprintf(stderr, "ABORT\n");
    fflush(stderr);
    abort();
}
