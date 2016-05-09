/*
 * Filename: src/inc/mmv-impl.h
 * Library: libmmv
 * Brief: Define data types, etc. used to _implement_ libmmv library
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

#ifndef MMV_IMPL_H
#define MMV_IMPL_H

#include <sys/types.h>
#include <strbuf.h>

#define SIZE_UNLIMITED ((size_t)(-1))

/*
 * DIRINFO depends on FILEINFO
 */
#ifdef IMPORT_DIRINFO
#define IMPORT_FILEINFO
#endif

/*
 * HANDLE depends on DIRINFO
 */
#ifdef IMPORT_HANDLE
#define IMPORT_DIRINFO
#endif


/*
 * REP depends directly on all of {HANDLE, DIRINFO, FILEINFO }
 */
#ifdef IMPORT_REP
#define IMPORT_HANDLE
#define IMPORT_DIRINFO
#define IMPORT_FILEINFO
#endif

typedef ino_t DIRID;
typedef dev_t DEVID;

// ==================== FILEINFO ====================
//

#ifdef IMPORT_FILEINFO

enum fi_stflags {
    FI_STTAKEN    = 0x01,
    FI_LINKERR    = 0x02,
    FI_INSTICKY   = 0x04,
    FI_NODEL      = 0x08,
    FI_KNOWWRITE  = 0x10,
    FI_CANWRITE   = 0x20,
    FI_ISDIR      = 0x40,
    FI_ISLNK      = 0x80,
};

struct fileinfo {
    char *       fi_name;
    struct rep * fi_rep;
    short        fi_mode;
    unsigned int fi_stflags;
};

typedef struct fileinfo FILEINFO;

#endif /* IMPORT_FILEINFO */


#ifdef IMPORT_DIRINFO

enum di_flags {
    DI_KNOWWRITE = 0x01,
    DI_CANWRITE  = 0x02,
    DI_CLEANED   = 0x04,
};

typedef struct {
    DEVID di_vid;
    DIRID di_did;
    unsigned int di_nfils;
    FILEINFO **  di_fils;
    unsigned int di_flags;
} DIRINFO;

#endif /* IMPORT_DIRINFO */

#ifdef IMPORT_HANDLE

enum h_flags {
    H_NODIR      = 1,
    H_NOREADDIR  = 2,
};

typedef struct {
    char *    h_name;
    DIRINFO * h_di;
    char      h_err;
} HANDLE;

#endif /* IMPORT_HANDLE */

#ifdef IMPORT_REP

typedef struct rep {
    HANDLE *     r_hfrom;
    FILEINFO *   r_ffrom;
    HANDLE *     r_hto;
    char *       r_nto;         // non-path part of new name
    FILEINFO *   r_fdel;
    struct rep * r_first;
    struct rep * r_thendo;
    struct rep * r_next;
    char         r_flags;
} REP;

typedef struct {
    REP *         rd_p;
    DIRINFO *     rd_dto;
    char *        rd_nto;
    unsigned int  rd_i;		// Unique index, position before sorting
} REPDICT;


#endif /* IMPORT_REP */

typedef void sigreturn_t;

extern sigreturn_t breakout(int s);
extern sigreturn_t breakrep(int s);
extern sigreturn_t breakstat(int s);


// ==================== POLICIES ====================
//
// Policy with respect to deleting / overwriting files.
//
// Policy with respect to bad replacement specifications.

#ifdef IMPORT_POLICY

enum policy_del {
    ASKDEL  = 0,
    ALLDEL  = 1,
    NODEL   = 2,
};

enum policy_bad {
    ASKBAD   = 0,
    SKIPBAD  = 1,
    ABORTBAD = 2,
};

#endif /* IMPORT_POLICY */


// ==================== PATTERN ====================
//

#ifdef IMPORT_PATTERN

#include <linux/limits.h>	// Import PATH_MAX

#define MAXPATLEN PATH_MAX

// Symbolic names for character constants
//
enum chr {
    ESC     = '\\',
    SLASH   = '/',
    BACKREF = '#'
};

#endif /* IMPORT_PATTERN */

// ==================== RFLAGS ====================
//
#ifdef IMPORT_RFLAGS

enum rflags {
    R_ISX        = 0x01,
    R_SKIP       = 0x02,
    R_DELOK      = 0x04,
    R_ISALIASED  = 0x08,
    R_ISCYCLE    = 0x10,
    R_ONEDIRLINK = 0x20,
};

#endif /* IMPORT_RFLAGS */

// ==================== mmv operations ====================
//

#ifdef IMPORT_OPS

enum ops {
    DFLT      = 0x001,
    NORMCOPY  = 0x002,
    OVERWRITE = 0x004,
    NORMMOVE  = 0x008,
    XMOVE     = 0x010,
    DIRMOVE   = 0x020,
    NORMAPPEND= 0x040,
    HARDLINK  = 0x100,
    SYMLINK   = 0x200,

    DFLTOP    = XMOVE,

    COPY      = (NORMCOPY | OVERWRITE),
    MOVE      = (NORMMOVE | XMOVE | DIRMOVE),
    APPEND    = NORMAPPEND,
    LINK      = (HARDLINK | SYMLINK),
};

#endif /* IMPORT_OPS */

#include <mmv-state.h>

// ==================== system context ====================
//

#ifdef IMPORT_SYS
struct system {
    size_t maxpathlen;
    char *home;
    size_t homelen;
    int uid;
    int euid;
    int oldumask;
    int umask;
};

typedef struct system sys_t;

#endif /* IMPORT_SYS */

#ifdef IMPORT_ALLOC

void *myalloc(size_t sz);
void *challoc(size_t sz, unsigned int which);
void chgive(void *p, size_t sz);

#endif /* IMPORT_ALLOC */

#ifdef IMPORT_DEBUG

void fdump_replacement_structure(FILE *f, REP *rp);
void fdump_all_replacement_structures(FILE *f, REP *head);

#endif /* IMPORT_DEBUG */

extern void fexplain_sbuf_pattern_too_long(FILE *f, sbuf_t *pattern);
extern void fexplain_char_pattern_too_long(FILE *f, const char *pattern, size_t maxlen);

extern void fprint_filename(FILE *, const char *fname);
extern void fexplain_err(FILE *f, int err);
extern void explain_err(int err);
extern void eexplain_err(int err);

#endif /* MMV_IMPL_H */
