/*
 * Filename: src/inc/mmv-impl-rep.h
 * Library: libmmv
 * Brief: Define the REP data type and all data types it depends on
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

#ifndef MMV_IMPL_REP_H
#define MMV_IMPL_REP_H

#include <sys/types.h>

#ifndef MMV_H
#include <mmv.h>
#endif

#ifndef MMV_IMPL_H
#include <mmv-impl.h>
#endif

struct fileinfo {
    char *       fi_name;
    struct rep * fi_rep;
    short        fi_mode;
    unsigned int fi_stflags;
};

struct dirinfo {
    DEVID di_vid;
    DIRID di_did;
    unsigned int di_nfils;
    FILEINFO **  di_fils;
    unsigned int di_flags;
};

struct handle {
    char *    h_name;
    DIRINFO * h_di;
    char      h_err;
};

struct rep {
    HANDLE *     r_hfrom;
    FILEINFO *   r_ffrom;
    HANDLE *     r_hto;
    char *       r_nto;         // non-path part of new name
    FILEINFO *   r_fdel;
    struct rep * r_first;
    struct rep * r_thendo;
    struct rep * r_next;
    char         r_flags;
};

struct repdict {
    REP *         rd_p;
    DIRINFO *     rd_dto;
    char *        rd_nto;
    unsigned int  rd_i;		// Unique index, position before sorting
};

#endif /* MMV_IMPL_REP_H */
