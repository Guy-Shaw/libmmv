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

#ifndef MMV_H
#include <mmv.h>
#endif

// ==================== opaque types ====================

/*
 * Any functions in the implementation of libmmv can see the data types
 * needed to use other functions.  Those functions can take as arguments
 * pointers to various structures, so the names of the data types involved
 * in their function signatures are made public, at least to the rest of
 * the implementation.
 *
 * In order to see inside a structure, a module must define what further
 * access it needs, via preprocessor definitions, before including mmv-impl.h.
 *
 * For example:
 *
 *   #defined IMPORT_FILEINFO
 *   #include <mmv-impl.h>
 *
 */

typedef ino_t DIRID;
typedef dev_t DEVID;

struct fileinfo;
typedef struct fileinfo FILEINFO;

struct dirinfo;
typedef struct dirinfo DIRINFO;

struct handle;
typedef struct handle HANDLE;

struct rep;
typedef struct rep REP;

struct repdict;
typedef struct repdict REPDICT;

#define SIZE_UNLIMITED ((size_t)(-1))

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

enum di_flags {
    DI_KNOWWRITE = 0x01,
    DI_CANWRITE  = 0x02,
    DI_CLEANED   = 0x04,
};

enum h_flags {
    H_NODIR      = 1,
    H_NOREADDIR  = 2,
};


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

struct backref;
typedef struct backref backref_t;

struct stage;
typedef struct stage stage_t;

struct pattern;
typedef struct pattern pattern_t;


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

#include <mmv-impl-functions.h>

#ifdef IMPORT_BACKREFS

// mmv_patgen parses 'from' and 'to' patterns and records
// the span ( start, length ) of all wildcards in the 'from' pattern.
// Backreferences in the 'to' pattern (\n) are in index into this
// array of wildcard descriptors.
//
struct backref {
    char  *br_start;
    size_t br_len;
};

// Account for the backreferences in each stage.
// Each stage is accounted for by an array slice,
// which is a subset of the whole array of backref_t descriptors.
// The concatenation of all backrefs for all stages
// is a partition; that is, there are no gaps and no overlap.
// There can be a single stage; and so the slice for that stage
// would cover all backrefs for the whole path.

struct stage {
    char    *stg_first;   // Pointer to first backref in this stage
    size_t   stg_count;   // Count of number of backreferences in this slice
    char *l;
    char *r;
};


// XXX document struct pattern
//

#define PATTERN_MAGIC 0x29cfafcd1de27164

struct pattern {
    size_t     pat_magic;
    backref_t *bkref_vec;
    size_t     bkref_siz;
    size_t     bkref_cnt;

    stage_t   *stage_vec;
    size_t     stage_siz;
    size_t     stage_cnt;
};

// Identifier, 'mmv', is an explicit argument
// So, we must use preprocessor macros, for now.
//

static int
mmv_backref_idx_diag(mmv_t *mmv, int br_index)
{
    pattern_t *pat = mmv->aux;
    size_t idx;

    if (pat->pat_magic != PATTERN_MAGIC) {
        eprintf("%s: Bad magic in pattern_t.\n", __FUNCTION__);
        return (2);  // XXX Use E* codes
    }

    if (br_index < 0) {
        eprintf("%s: br_index=%d\n", __FUNCTION__, br_index);
        eprintf("    br_index must be in (0..%zu).\n", pat->bkref_cnt);
        return (2);  // XXX Use E* codes
    }

    idx = br_index;

    if (idx > pat->bkref_cnt) {
        eprintf("%s: Backref index must be in (0..%zu); br_index=%zu\n",
                __FUNCTION__, pat->bkref_cnt, idx);
        return (2);
    }

    return (0);
}

static backref_t *
mmv_backref_idx(mmv_t *mmv, int br_index)
{
    pattern_t *pat = mmv->aux;
    backref_t *bkrefp;
    size_t idx;
    int rc;

    rc = mmv_backref_idx_diag(mmv, br_index);
    if (rc) {
        mmv_abort();
    }

    bkrefp = pat->bkref_vec;
    idx = br_index;
    return (bkrefp + idx);
}

static int
mmv_brstage_idx_diag(mmv_t *mmv, int stg_index)
{
    pattern_t *pat = mmv->aux;
    size_t idx;

    if (pat->pat_magic != PATTERN_MAGIC) {
        eprintf("%s: Bad magic in pattern_t.\n", __FUNCTION__);
        return (2);  // XXX Use E* codes
    }

    if (stg_index < 0) {
        eprintf("%s: stg_index=%d\n", __FUNCTION__, stg_index);
        eprintf("    stg_index must be in (0..%zu).\n", pat->stage_cnt);
        return (2);  // XXX Use E* codes
    }

    idx = stg_index;

    if (idx > pat->stage_cnt) {
        eprintf("%s: Stage index must be in (0..%zu); stg_index=%zu\n",
                __FUNCTION__, pat->stage_cnt, idx);
        return (2);
    }

    return (0);
}

static inline stage_t *
mmv_brstage_idx(mmv_t *mmv, int stg_index)
{
    pattern_t *pat = mmv->aux;
    stage_t *stagep;
    size_t idx;
    int rc;

    rc = mmv_brstage_idx_diag(mmv, stg_index);
    if (rc) {
        mmv_abort();
    }
    stagep = pat->stage_vec;
    idx = stg_index;
    return (stagep + idx);
}

// Identifier, 'mmv', is an implicit "by name" argument
// So, we must use preprocessor macros, for now.
//
#define mmv_backref(brn) mmv_backref_idx(mmv, (brn))
#define mmv_brstage(stg) mmv_brstage_idx(mmv, (stg))

// Access into array of backreferences
//
#define start(brn) (mmv_backref(brn)->br_start)
#define wild_len(brn) (mmv_backref(brn)->br_len)

// Access into array of backreferences for each stage.
//
#define firstwild(stg) (mmv_brstage(stg)->stg_first)
#define nwilds(stg) (mmv_brstage(stg)->stg_count)

#endif /* IMPORT_BACKREFS */

#endif /* MMV_IMPL_H */
