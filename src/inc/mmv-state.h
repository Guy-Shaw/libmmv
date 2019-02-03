/*
 * Filename: src/inc/mmv-state.h
 * Library: libmmv
 * Brief: Define the mmv_t object, and its dependencies
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

#ifndef MMV_STATE_H
#define MMV_STATE_H

#include <mmv-impl.h>
#include <mmv-impl-rep.h>

#include <stdbool.h>
#include <unistd.h>         // Import size_t

#include <sys/types.h>      // Import type mode_t for umask()
#include <sys/stat.h>

#define MMV_MAGIC 0x65AF11AF

enum encode {
    ENCODE_NONE = 0x101,
    ENCODE_PAT,         // mmv-classic patterns
    ENCODE_NUL,         // filenames terminated by NUL byte
    ENCODE_QP,          // quoted-printable encoded filenames
    ENCODE_VIS,         // vis-encoded filenames (Berkeley BSD vis/unvis)
    ENCODE_XNN,         // filenames encode with \xnn for all non-graphic chars
};

/*
 * Data structure containing all state related to an instance of mmv.
 */

struct mmv_state {
    int magic;
    char *from;
    size_t fromsz;
    size_t fromlen;

    char *to;
    size_t tosz;
    size_t tolen;

    char *pathbuf;
    char *fullrep;

    FILE *debug_fh;
    bool verbose;

    enum encode encoding;
    int patflags;
    int op;
    int badstyle;
    int delstyle;
    int noex;

    int oldumask;
    int badreps;
    int paterr;
    int gotsig;
    int failed;

    int  matchall;
    FILE *outfile;
    FILE *errfile;

    REP hrep;
    REP *lastrep;
    REP mistake;
    int nreps;

    // An opaque pointer to implementation-specific data
    // This is not only private to the implementation
    // and should not be considered part of the interface;
    // it is even private to certain subsets of the libmmv.
    // Other parts of libmmv that do no get used should not
    // know anything about how they are used, and should not
    // even pollute their namespace with knowledge of funtions
    // and data types that might be related to mmv->aux.
    void *aux;
};

#define DFLTOP XMOVE

#endif /* MMV_STATE_H */
