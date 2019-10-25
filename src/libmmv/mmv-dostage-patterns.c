/*
 * Filename: src/libmmv/mmv-dostage-patterns.c
 * Library: libmmv
 * Brief: After from->to pattern pairs have been collected, do the actual move
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
#define IMPORT_BACKREFS

#include <mmv-impl.h>

#define INITROOM 10

static inline int
mylower(int c)
{
    return (isupper(c) ? (c)-'A'+'a' : (c));
}

static inline int
myupper(int c)
{
    return (islower(c) ? (c)-'a'+'A' : (c));
}

static const size_t bkref_alloc_init = 10;
static const size_t stage_alloc_init = 10;

/*
 * Constants
 */

static char TOOLONG[] = "(too long)";
static char EMPTY[] = "(empty)";

/*
 * Declare global data -- shared with mmv-dostage-common.c
 */

extern int direrr;
extern int repbad;

/*
 * Declare static data
 */

#define NULL_DIRID ((DIRID)-1)

static DIRID cwdd = NULL_DIRID;
static DEVID cwdv = NULL_DIRID;

static void
init_backrefs(mmv_t *mmv)
{
    pattern_t *pat;
    size_t sz;

    pat = mmv->aux;
    if (pat) {
        return;
    }

    mmv->aux = (void *) mmv_alloc(sizeof (pattern_t));
    pat = mmv->aux;

    sz = bkref_alloc_init * sizeof (backref_t);
    pat->bkref_vec = (backref_t *) mmv_alloc(sz);
    pat->bkref_siz = sz;
    pat->bkref_cnt = 0;

    sz = stage_alloc_init * sizeof (stage_t);
    pat->stage_vec = (stage_t *) mmv_alloc(sz);
    pat->stage_siz = sz;
    pat->stage_cnt = 0;

    pat->pat_magic = PATTERN_MAGIC;
}

/**
 * @brief  Initialize static data, private to the file, 'mmv-patgen.c'.
 *
 * This is a bit of a hack.
 * It is provided for mmv_init().
 * It means mmv-init() does not need to know all about the data
 * specific to mmv-patgen(); it just needs to know to call this
 * function to initialize all that is necessary, without caring
 * what all it is.
 *
 * It also means that bookkeeping information about directories,
 * files, and from-pattern substrings (for back-refs) can be
 * reorganized without changing any other source files.
 *
 */

void
mmv_init_patgen(mmv_t *mmv)
{
    if (cwdd == NULL_DIRID) {
        struct stat dstat;

        if (stat(".", &dstat) == 0) {
            cwdd = dstat.st_ino;
            cwdv = dstat.st_dev;
        }
    }

    init_backrefs(mmv);
}

/**
 * @brief Do glob-style pattern match of a given string and pattern
 *
 * @param pat     IN   pattern to match
 * @param s       IN   string to match agains |pattern|
 * @param bkref   OUT  Wildcard info, vector of start and length
 * @return 0/1 status: 1 = match, 0 = not match
 *
 */

int
match(char *pat, char *s, backref_t *bkref)
{
    char c;

    bkref->br_start = 0;
    for (;;) {
        switch (c = *pat) {
        case '\0':
        case SLASH:
            return (*s == '\0');
        case '*':
            bkref->br_start = s;
            if ((c = *(++pat)) == '\0') {
                bkref->br_len = strlen(s);
                return (1);
            }
            else {
                for (bkref->br_len = 0; !match(pat, s, bkref + 1); ++bkref->br_len, ++s) {
                    if (*s == '\0') {
                        return (0);
                    }
                }
                return (1);
            }
        case '?':
            if (*s == '\0') {
                return (0);
            }
            bkref->br_start = s;
            bkref->br_len = 1;
            ++bkref;
            ++pat;
            ++s;
            break;
        case '[':
            {
                int matched = 0, notin = 0, inrange = 0;
                char prevc = '\0';

                if ((c = *(++pat)) == '^') {
                    notin = 1;
                    c = *(++pat);
                }
                while (c != ']') {
                    if (c == '-' && !inrange) {
                        inrange = 1;
                    }
                    else {
                        if (c == ESC) {
                            c = *(++pat);
                        }
                        if (inrange) {
                            if (*s >= prevc && *s <= c) {
                                matched = 1;
                            }
                            inrange = 0;
                        }
                        else if (c == *s) {
                            matched = 1;
                        }
                        prevc = c;
                    }
                    c = *(++pat);
                }
                if (inrange && *s >= prevc) {
                    matched = 1;
                }
                if (!(matched ^ notin)) {
                    return (0);
                }
                bkref->br_start = s;
                bkref->br_len = 1;
                ++bkref;
                ++pat;
                ++s;
            }
            break;
        case ESC:
            c = *(++pat);
            __attribute__ ((fallthrough));
        default:
            if (c == *s) {
                ++pat;
                ++s;
            }
            else {
                return (0);
            }
        }
    }
}

/*
 *
 * Functions to convert memory region to upper/lower case.
 *
 * There are functions available to convert zstrings to upper/lower case,
 * but here we need to deal with memory rigions, because we are converting
 * pieces of strings, not whole zstrings.  For example, back-references
 * are kept track of as substrings.
 */

/**
 * @brief copy one region of memory to another, converting to uppercase.
 *
 * @param dst  OUT   Destination region
 * @param src  IN    Source region
 * @param len  IN    Size of both memory regions
 *
 */

static void
memmove_uc(char *dst, const char *src, size_t len)
{
    while (len != 0) {
        *dst = myupper(*src);
        ++src;
        ++dst;
        --len;
    }
}

/**
 * @brief copy one region of memory to another, converting to lowercase.
 *
 * @param dst  OUT   Destination region
 * @param src  IN    Source region
 * @param len  IN    Size of both memory regions
 *
 */

static void
memmove_lc(char *dst, const char *src, size_t len)
{
    while (len != 0) {
        *dst = mylower(*src);
        ++src;
        ++dst;
        --len;
    }
}

/**
 * @brief Do pattern expansion for one source file and one 'to' pattern.
 *
 * @param mmv
 *
 * At this stage, |mmv| already contains a source filename
 * and a replacement pattern.  Do pattern expansion on this pair.
 *
 * The 'from' pattern must have already been parsed, because
 * if there are any back-references in the 'to' pattern, all information
 * about them is in static data: start[], wild_len[], nwilds, etc.
 *
 */

void
makerep(mmv_t *mmv)
{
    char *pat;
    char *p;
    size_t l;
    int c, pc;

    repbad = 0;
    p = mmv->fullrep;
    for (pat = mmv->to, l = 0; (c = *pat) != '\0'; ++pat, ++l) {
        if (mmv->encoding == ENCODE_PAT && c == BACKREF) {
            int cnv;
            int backref_nr;
            char   *mv_start;
            size_t  mv_len;

            c = *(++pat);
            cnv = '=';
            switch (c) {
            case 'l':
            case 'u':
                cnv = c;
                c = *(++pat);
                break;
            }

            for (backref_nr = 0; ; backref_nr *= 10) {
                backref_nr += c - '0';
                c = *(pat + 1);
                if (!isdigit(c)) {
                    break;
                }
                ++pat;
            }

            if (backref_nr == 0) {
                mv_start = mmv->from;
                mv_len   = mmv->fromlen;
            }
            else {
                --backref_nr;
                mv_start = start(backref_nr);
                mv_len   = wild_len(backref_nr);
            }

            if (l + mv_len >= PATH_MAX) {
                goto toolong;
            }

            switch (cnv) {
            case '=':
                memmove(p, mv_start, mv_len);
                break;
            case 'l':
                memmove_lc(p, mv_start, mv_len);
                break;
            case 'u':
                memmove_uc(p, mv_start, mv_len);
                break;
            }
            p += mv_len;
            l += mv_len;
        }
        else {
            if (c == ESC) {
                c = *(++pat);
            }
            if (l >= PATH_MAX) {
                goto toolong;
            }
            if ((c == SLASH) && (p == mmv->fullrep ? pat != mmv->to : (((pc = *(p - 1)) == SLASH) && *(pat - 1) != pc))) {
                repbad = 1;
                if (l + strlen(EMPTY) >= PATH_MAX) {
                    goto toolong;
                }
                strcpy(p, EMPTY);
                p += strlen(EMPTY);
                l += strlen(EMPTY);
            }
            *(p++) = c;
        }
    }

    if (p == mmv->fullrep) {
        strcpy(mmv->fullrep, EMPTY);
        repbad = 1;
    }
    *(p++) = '\0';
    return;

  toolong:
    repbad = 1;
    strcpy(mmv->fullrep, TOOLONG);
}

/**
 * @brief  dostage_patterns ???
 *
 * @param mmv
 * @param lastend  ???  ???
 * @param pathend  ???  ???
 * @param backref  View into a subset of backrefs
 * @param stage    ???  ???
 * @param anylev   IN   ???
 *
 */

int
dostage_patterns(mmv_t *mmv, char *lastend, char *pathend, backref_t *bkref, int istage, int anylev)
{
    DIRINFO *di;
    HANDLE *h, *hto;
    int prelen, litlen, nfils, i, k, flags, match_rv;
    FILEINFO **pf, *fdel;
    char *nto, *firstesc;
    REP *p;
    int wantdirs;
    int ret;
    bool laststage;
    pattern_t *pat;
    backref_t *lastbkref;
    size_t stage;

    stage = (size_t)istage; // XXX convert: make stage argument of type size_t
    pat = mmv->aux;
    if (dbgprint_fh) {
        fprintf(dbgprint_fh, "%s:\n", __FUNCTION__);
        fprintln_str(dbgprint_fh, "    lastend=", lastend);
        fprintln_str(dbgprint_fh, "    pathend=", pathend);
        fprintf(dbgprint_fh, "    stage=%zu\n", stage);
        fprintf(dbgprint_fh, "    anylev=%d\n", anylev);
    }

    ret = 1;
    laststage = (stage + 1 == pat->stage_cnt);
    if (pat->stage_cnt >= 1) {
        lastbkref = mmv_backref(pat->stage_cnt - 1);
    }
    else {
        lastbkref = NULL;
    }
    wantdirs = !laststage || (mmv->op & (DIRMOVE | SYMLINK)) || lastbkref == NULL;

    if (!anylev) {
        prelen = pat->stage_vec[stage].l - lastend;
        if (pathend - mmv->pathbuf + prelen >= PATH_MAX) {
            printf("%s -> %s : search path after '%s' too long.\n",
                mmv->from, mmv->to, mmv->pathbuf);
            // XXX mmv_abort();
            mmv->paterr = 1;
            return (1);
        }
        memmove(pathend, lastend, prelen);
        pathend += prelen;
        *pathend = '\0';
        lastend = pat->stage_vec[stage].l;
    }

    if ((h = checkdir(mmv->pathbuf, pathend, 0)) == NULL) {
        if (stage == 0 || direrr == H_NOREADDIR) {
            printf("%s -> %s : directory '%s'",
                mmv->from, mmv->to, mmv->pathbuf);
            if (stage == 0) {
                printf(" does not exist.\n");
            }
            else if (direrr == H_NOREADDIR) {
                printf(" does not allow reads/searches.\n");
            }
            mmv->paterr = 1;
        }
        return (stage);
    }
    di = h->h_di;

    if (*lastend == ';') {
        anylev = 1;
        bkref->br_start = pathend;
        bkref->br_len = 0;
        ++lastend;
    }

    nfils = di->di_nfils;

    if ((mmv->op & MOVE) && !dwritable(h)) {
        printf("%s -> %s : directory %s does not allow writes.\n",
            mmv->from, mmv->to, mmv->pathbuf);
        mmv->paterr = 1;
        goto skiplev;
    }

    firstesc = strchr(lastend, ESC);
    if (firstesc == NULL || firstesc > firstwild(stage)) {
        firstesc = firstwild(stage);
    }
    litlen = firstesc - lastend;
    pf = di->di_fils + (i = ffirst(lastend, litlen, di));
    if (i < nfils) {
        do {
            if ((match_rv = trymatch(mmv, *pf, lastend)) != 0 && (match_rv == 1 || match(lastend + litlen, (*pf)->fi_name + litlen, bkref + anylev)) && keepmatch(mmv, *pf, pathend, &k, 0, wantdirs, laststage)) {
                if (!laststage) {
                    ret &= dostage_patterns(mmv, pat->stage_vec[stage].r, pathend + k, bkref + nwilds(stage), stage + 1, 0);
                }
                else {
                    ret = 0;
                    makerep(mmv);
                    if (badrep(mmv, h, *pf, &hto, &nto, &fdel, &flags)) {
                        (*pf)->fi_rep = &mmv->mistake;
                    }
                    else {
                        (*pf)->fi_rep = p = (REP *) challoc(sizeof (REP), 1);
                        p->r_flags = flags | mmv->patflags;
                        p->r_hfrom = h;
                        p->r_ffrom = *pf;
                        p->r_hto = hto;
                        p->r_nto = nto;
                        p->r_fdel = fdel;
                        p->r_first = p;
                        p->r_thendo = NULL;
                        p->r_next = NULL;
                        mmv->lastrep->r_next = p;
                        mmv->lastrep = p;
                        ++mmv->nreps;
                    }
                }
            }
            ++pf;
            ++i;
        } while (i < nfils && strncmp(lastend, (*pf)->fi_name, litlen) == 0);
    }

  skiplev:
    if (anylev) {
        for (pf = di->di_fils, i = 0; i < nfils; ++pf, ++i) {
            if (*((*pf)->fi_name) != '.' && keepmatch(mmv, *pf, pathend, &k, 1, 1, false)) {
                bkref->br_len = pathend - bkref->br_start + k;
                ret &= dostage_patterns(mmv, lastend, pathend + k, bkref, stage, 1);
            }
        }
    }

    return (ret);
}
