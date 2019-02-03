/*
 * Filename: src/libmmv/mmv-debug.c
 * Library: libmmv
 * Brief: Debug helper functions to pretty-print some data structures
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
#include <stdint.h>		// Import uintptr_t
#include <eprint.h>

#define IMPORT_REP
#define IMPORT_HANDLE
#define IMPORT_DIRINFO
#define IMPORT_FILEINFO
#define IMPORT_DEBUG
#define IMPORT_OPS
#include <mmv-impl.h>

typedef unsigned int uint_t;


extern FILE *dbgprint_fh;

/**
 * @brief Append a string to a buffer, avoid buffer overflow.
 *
 * @param buf     OUT    buffer that the string is to be appended to
 * @param sz      IN     maximum capacity of |buf|
 * @param posaddr INOUT  reference to a position, get updated by append
 * @param str     IN     nul-terminated string to append to |buf|
 *
 */

void
append_str(char *buf, size_t sz, size_t *posaddr, const char *str)
{
    size_t pos;

    pos = *posaddr;
    while (pos < sz && *str) {
        buf[pos] = *str;
        ++str;
        ++pos;
    }
    *posaddr = pos;
}

/**
 * @brief Accumulate decoded values of masked flag bits; append to a buffer.
 *
 * @param flg     IN     A set of flags represented as a bit mask
 * @param msk     IN     A mask used to select one bit from |flg|
 * @param buf     OUT    A buffer to build the string of decoded bits
 * @param sz      IN     Maximum capacity of |buf|
 * @param posaddr INOUT  Reference to current position in |buf|; gets updated
 * @param str     IN     String representing the decoded value of flg&msk
 *
 */

void
append_flag(uint_t flg, uint_t msk, char *buf, size_t sz, size_t *posaddr, const char *str)
{
    if ((flg & msk) == 0) {
        return;
    }

    if (*posaddr != 0) {
        append_str(buf, sz, posaddr, "|");
    }
    append_str(buf, sz, posaddr, str);
}

/*
 * @brief Compose the decoded values of all bits in a |fi_stflags|
 *
 * @param buf     OUT    buffer in which to accumulate the decoded flags
 * @param sz      IN     maximum capacity of |buf|
 * @param flgs    IN     a set of si_stflags bits to be decoded
 *
 */

void
decode_fi_stflags(char *buf, size_t sz, unsigned int flgs)
{
    size_t pos;

    pos = 0;
    append_flag(flgs, FI_ISLNK, buf, sz, &pos, "FI_ISLNK");
    append_flag(flgs, FI_ISDIR, buf, sz, &pos, "FI_ISDIR");
    append_flag(flgs, FI_CANWRITE, buf, sz, &pos, "FI_CANWRITE");
    append_flag(flgs, FI_KNOWWRITE, buf, sz, &pos, "FI_KNOWWRITE");
    append_flag(flgs, FI_NODEL, buf, sz, &pos, "FI_NODEL");
    append_flag(flgs, FI_INSTICKY, buf, sz, &pos, "FI_INSTICKY");
    append_flag(flgs, FI_LINKERR, buf, sz, &pos, "FI_LINKERR");
    append_flag(flgs, FI_STTAKEN, buf, sz, &pos, "FI_STTAKEN");
    buf[pos] = '\0';
}


const char *
decode_op(unsigned int msk)
{
    switch (msk) {
    case DFLT:       return ("DFLT");
    case NORMCOPY:   return ("NORMCOPY");
    case OVERWRITE:  return ("OVERWRITE");
    case NORMMOVE:   return ("NORMMOVE");
    case XMOVE:      return ("XMOVE");
    case DIRMOVE:    return ("DIRMOVE");
    case NORMAPPEND: return ("NORMAPPEND");
    case HARDLINK:   return ("HARDLINK");
    case SYMLINK:    return ("SYMLINK");
    default:         return (NULL);
    }
}

void
fdump_op(FILE *f, enum ops op)
{
    unsigned int op_bits;
    unsigned int msk;
    unsigned int nflags;

    op_bits = (int)op;

    if (op_bits == 0) {
        fputs("0\n", f);
        return;
    }

    nflags = 0;
    for (msk = 1 << 31; msk; msk >>= 1) {
        if ((op_bits & msk) != 0) {
            const char *sym;

            sym = decode_op(msk);
            if (nflags) {
                fputs("|", f);
            }
            if (sym) {
                fputs(sym, f);
            }
            else {
                fprintf(f, "0x%x", msk);
            }
            ++nflags;
        }
    }
    fputs("\n", f);
}

void
fdump_mmv_op(FILE *f, mmv_t *mmv)
{
    fdump_op(f, mmv->op);
}

// Keep track of level, to control indentation.
//
static int level;

/**
 * @brief Print leading space appropriate for the current level of indentation.
 *
 * @param  f  IN  Where to print
 *
 */

void
fdump_indent(FILE *f)
{
    int i;

    for (i = 0; i < level; ++i) {
        fputs("    ", f);
    }
}

/**
 * @brief print a description, be careful about NULL pointers.
 *
 * @param  f     IN  Where to print
 * @param  desc  IN  Description; possibly NULL.
 *
 */

void
fdump_desc(FILE *f, void *p, const char *desc)
{
    if (desc == NULL) {
        desc = "(no description)";
    }

    fdump_indent(f);
    fprint_vis(f, desc);
    if (p == NULL) {
        fprintf(f, "=NULL");
    }
    else {
        fprintf(f, " @%p:", p);
    }
    fputs("\n", f);
}

/**
 * @brief print a pointer and its description, be careful about NULL pointers.
 *
 * @param  f     IN  Where to print
 * @param  p     IN  Pointer to represent; possibly NULL.
 * @param  desc  IN  Description; possibly NULL.
 *
 */

void
fdump_pointer(FILE *f, void *p, const char *desc)
{
    if (desc == NULL) {
        desc = "(no description)";
    }

    fdump_indent(f);
    fprint_vis(f, desc);
    if (p == NULL) {
        fprintf(f, "=NULL");
    }

    fprintf(f, "=0x%p", p);
    fputs("\n", f);
}

/**
 * @brief print a pointer and its description, be careful about NULL pointers.
 *
 * @param  f     IN  Where to print
 * @param  p     IN  Pointer to a zstring to represent; possibly NULL.
 * @param  desc  IN  Description; possibly NULL.
 *
 * Like fdump_pointer(), but p is a string.
 *
 */

void
fdump_name(FILE *f, const char *p, const char *desc)
{
    if (desc == NULL) {
        desc = "(no description)";
    }

    fdump_indent(f);
    fprint_vis(f, desc);
    if (p == NULL) {
        fprintf(f, "=NULL");
    }
    else {
        fprint_str(f, "=", p);
    }
    fputs("\n", f);
}

/**
 * @brief print the elements of a |HANDLE|.
 *
 * @param  f     IN  Where to print
 * @param  h     IN  |HANDLE| to be represented
 * @param  desc  IN  A description of h
 *
 */

void
fdump_handle(FILE *f, HANDLE *h, const char *desc)
{
    fdump_desc(f, h, desc);
    if (h == NULL) {
        return;
    }

    ++level;
    fdump_name(f, h->h_name, "h_name");
    fdump_pointer(f, h->h_di, "h_di");
    fdump_indent(f);
    fprintf(f, "h_err = %u\n", h->h_err);
    --level;
}

/**
 * @brief Print the elements of a |FILEINFO|.
 *
 * @param  f     IN  Where to print
 * @param  fip   IN  |FILEINFO| to be represented
 * @param  desc  IN  A description of |fip|
 *
 */

void
fdump_fileinfo(FILE *f, FILEINFO *fip, const char *desc)
{
    char dbuf[128];

    fdump_desc(f, fip, desc);
    if (fip == NULL) {
        return;
    }

    ++level;
    fdump_name(f, fip->fi_name, "fi_name");
    fdump_pointer(f, fip->fi_rep, "fi_rep");
    fdump_indent(f);
    fprintf(f, "fi_mode=0x%x\n", fip->fi_mode);
    decode_fi_stflags(dbuf, sizeof (dbuf), fip->fi_stflags);
    fdump_indent(f);
    fprintf(f, "fi_stflags=0x%x=%s\n", fip->fi_stflags, dbuf);
    --level;
}

/**
 * @brief Print the elements of a |REP|
 *
 * @param  f     IN  Where to print
 * @param  rp    IN  The |REP| to be represented
 *
 */

void
fdump_replacement_structure(FILE *f, REP *rp)
{
    fprintf(f, "REP @%p\n", rp);
    ++level;
    fdump_handle(f, rp->r_hfrom, "r_hfrom");
    fdump_fileinfo(f, rp->r_ffrom, "r_ffrom");
    fdump_handle(f, rp->r_hto, "r_hto");
    fdump_name(f, rp->r_nto, "r_nto");
    fdump_fileinfo(f, rp->r_fdel, "r_fdel");
    fdump_pointer(f, rp->r_first, "r_first");
    fdump_pointer(f, rp->r_thendo, "r_thendo");
    fdump_pointer(f, rp->r_next, "r_next");
    --level;
}

/**
 * @brief Print all |REP| data structures, starting with |head|.
 *
 * @param  f     IN  Where to print
 * @param  head  IN  Start of a chain of |REP| structures
 *
 */

void
fdump_all_replacement_structures(FILE *f, REP *head)
{
    REP *rp1, *rp;

    level = 0;
    for (rp1 = head; rp1 != NULL; rp1 = rp1->r_next) {
        for (rp = rp1; rp != NULL; rp = rp->r_thendo) {
            fdump_replacement_structure(f, rp);
        }
    }
}

#include <bsd/vis.h>

void
fprint_vis(FILE *f, const char *str)
{
    const char *s;
    char visbuf[8];

    s = str;
    while (*s) {
        vis(visbuf, *s, VIS_WHITE, 0);
        fputs(visbuf, f);
        ++s;
    }
}

void
fprint_str(FILE *f, const char *var, const char *str)
{
    fputs(var, f);
    fputs("'", f);
    fprint_vis(f, str);
    fputs("'", f);
}

void
fprintln_str(FILE *f, const char *var, const char *str)
{
    fprint_str(f, var, str);
    fputs("\n", f);
}

void
dbg_println_str(const char *var, const char *str)
{
    if (dbgprint_fh) {
        fprintln_str(dbgprint_fh, var, str);
    }
}
