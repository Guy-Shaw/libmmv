/*
 * Filename: src/libmmv/mmv-main.c
 * Library: libmmv
 * Brief: Handle main-level -- command-line arguments, oprions, global variables
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

// XXX #define _GNU_SOURCE 1

#include <dirent.h>    // for dirent
#include <errno.h>     // for EINVAL, errno
#include <stdio.h>     // for fprintf, NULL, stderr
#include <string.h>    // for strcmp
#include <unistd.h>    // for getgid, setgid, setuid, uid_t, gid_t

#define IMPORT_PATTERN
#define IMPORT_RFLAGS
#define IMPORT_OPS
#define IMPORT_POLICY
#define IMPORT_REP
#define IMPORT_HANDLE
#define IMPORT_DIRINFO
#define IMPORT_FILEINFO
#define IMPORT_ALLOC
#define IMPORT_DEBUG

#include <mmv-impl.h>  // for mmv_t, explain_err, DFLTOP, ops::DFLT, ops::DI...

#ifdef BEFORE_INCLUDE_WHAT_YOU_USE

#include <assert.h>

#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>	// Import EINVAL, ENOTSUP

/* For various flavors of Unix */

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <signal.h>
#include <fcntl.h>

#include <dirent.h>
typedef struct dirent DIRENTRY;

#include <eprint.h>
#include <dbgprint.h>

#define IMPORT_PATTERN
#define IMPORT_RFLAGS
#define IMPORT_OPS
#define IMPORT_POLICY
#define IMPORT_REP
#define IMPORT_HANDLE
#define IMPORT_DIRINFO
#define IMPORT_FILEINFO
#define IMPORT_ALLOC
#define IMPORT_DEBUG

#include <mmv-impl.h>

#endif /* BEFORE_INCLUDE_WHAT_YOU_USE */

static char MOVENAME[]   = "mmv";
static char COPYNAME[]   = "mcp";
static char APPENDNAME[] = "mad";
static char LINKNAME[]   = "mln";

/*
 * Declare external functions.
 */

#if 0
extern void mmv_set_default_options(mmv_t *mmv);
extern int mmv_setopt(mmv_t *mmv, int opt);
extern int getpat(mmv_t *mmv);
#endif

char USAGE[] =
    "Usage: %s [-m|x|r|c|o|a|l] [-h] [-d|p] [-g|t] [-v|n] [from to]\n"
    "\n"
    "Use =[l|u]N in the ``to'' pattern to get the [lowercase|uppercase of the]\n"
    "string matched by the N'th ``from'' pattern wildcard.\n"
    "\n"
    "A ``from'' pattern containing wildcards should be quoted when given\n"
    "on the command line.\n";

extern uid_t uid, euid;

/**
 * @brief Parse mmv-classic arguments
 *
 * @param  argc      IN   Together with argv ...
 * @param  argv      IN   the main() program command-line arguments
 * @param  pfrompat  OUT  Set to the 'from' pattern, if any
 * @param  ptopat    OUT  Set to the 'to'   pattern, if any
 * @return errno-style -- 0 == success, non-zero is some type of error
 *
 */

int
procargs(mmv_t *mmv, int argc, char *const *argv, char **pfrompat, char **ptopat)
{
    char *p;
    char *cmdname = argv[0];

    mmv_set_default_options(mmv);

    // XXX Use GNU getopt() or getopt_long()

    for (--argc, ++argv; argc > 0 && **argv == '-'; --argc, ++argv) {
        for (p = *argv + 1; *p != '\0'; ++p) {
            int err;
            int c;

            c = *p;
            err = mmv_setopt(mmv, c);
            if (err) {
                return ((EINVAL << 8) + c);
            }
        }
    }

    if (mmv->op == DFLT) {
        if (strcmp(cmdname, MOVENAME) == 0)
            mmv->op = XMOVE;
        else if (strcmp(cmdname, COPYNAME) == 0)
            mmv->op = NORMCOPY;
        else if (strcmp(cmdname, APPENDNAME) == 0)
            mmv->op = NORMAPPEND;
        else if (strcmp(cmdname, LINKNAME) == 0)
            mmv->op = HARDLINK;
        else
            mmv->op = DFLTOP;
    }

    if (euid != uid && !(mmv->op & DIRMOVE)) {
        int rv;
        gid_t gid;
        int err;

        rv = setuid(uid);
        if (rv) {
            err = errno;
            fprintf(stderr, "setuid(%d) failed.\n", uid);
            explain_err(err);
            return (err);
        }
        gid = getgid();
        rv = setgid(gid);
        if (rv) {
            err = errno;
            fprintf(stderr, "setgid(%d) failed.\n", gid);
            explain_err(err);
            return (err);
        }
    }

    if (mmv->badstyle != ASKBAD && mmv->delstyle == ASKDEL) {
        mmv->delstyle = NODEL;
    }

    if (argc == 0) {
        *pfrompat = NULL;
        *ptopat   = NULL;
    }
    else if (argc == 2) {
        *pfrompat = argv[0];
        *ptopat   = argv[1];
    }
    else {
        return (EINVAL);
    }

    return (0);
}
