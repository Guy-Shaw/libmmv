/*
 * Filename: src/libmmv/mmv-setopt.c
 * Library: libmmv
 * Brief: Set options in a way that is decoupled from most mmv internals.
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

#define _GNU_SOURCE 1
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

void
mmv_set_default_options(mmv_t *mmv)
{
    mmv->op       = DFLT;
    mmv->verbose  = false;
    mmv->noex     = false;
    mmv->matchall = false;
    mmv->delstyle = ASKDEL;
    mmv->badstyle = ASKBAD;
}

int
mmv_setopt(mmv_t *mmv, int opt)
{
    switch (opt) {
    case 'v':
        mmv->verbose = true;
        break;
    case 'i':
        // Accept -i option for people with 'mv' finger memory,
        // even though it is not needed for mmv.
        break;
    case 'D':
        mmv->debug_fh = stderr;
        break;
    case 'Z':
        mmv->encoding = ENCODE_NUL;
        break;
    case 'Q':
        mmv->encoding = ENCODE_QP;
        break;
    case 'X':
        mmv->encoding = ENCODE_XNN;
        break;
    case 'n':
        mmv->noex = true;
        break;
    case 'h':
        mmv->matchall = true;
        break;
    case 'd':
        if (mmv->delstyle == ASKDEL) {
            mmv->delstyle = ALLDEL;
        }
        break;
    case 'p':
        if (mmv->delstyle == ASKDEL) {
            mmv->delstyle = NODEL;
        }
        break;
    case 'g':
        if (mmv->badstyle == ASKBAD) {
            mmv->badstyle = SKIPBAD;
        }
        break;
    case 't':
        if (mmv->badstyle == ASKBAD) {
            mmv->badstyle = ABORTBAD;
        }
        break;
    case 'm':
        if (mmv->op == DFLT) {
            mmv->op = NORMMOVE;
        }
        break;
    case 'x':
        if (mmv->op == DFLT) {
            mmv->op = XMOVE;
        }
        break;
    case 'r':
        if (mmv->op == DFLT) {
            mmv->op = DIRMOVE;
        }
        break;
    case 'c':
        if (mmv->op == DFLT) {
            mmv->op = NORMCOPY;
        }
        break;
    case 'o':
        if (mmv->op == DFLT) {
            mmv->op = OVERWRITE;
        }
        break;
    case 'a':
        if (mmv->op == DFLT) {
            mmv->op = NORMAPPEND;
        }
        break;
    case 'l':
        if (mmv->op == DFLT) {
            mmv->op = HARDLINK;
        }
        break;
    default:
        return (EINVAL);
    }

    return (0);
}
