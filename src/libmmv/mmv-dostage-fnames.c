/*
 * Filename: src/libmmv/mmv-dostage-fnames.c
 * Library: libmmv
 * Brief: After from->to simple filename pairs have been collected, do actual move
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

/*
 * Symbolic constants used to pass C-int-boolean flags
 * functions like to keepmatch() and dostage_fnames()
 */

#define ANY_LEVEL   1
#define WANT_DIRS   1
#define WANT_FILES  1
#define NEED_SLASH 1

/*
 * Declare external functions.
 */


/*
 * System
 */

extern char *sys_home;
extern size_t sys_homelen;

/*
 * From mmv-dostage-common.c
 */

extern int direrr;

static char *stagel[20], *stager[20];
static int nstages;

/**
 * @brief Do match of a given string, up to the end of a simple filename.
 *
 * @param pat     IN   pattern to match
 * @param s       IN   string to match agains |pattern|
 * @return 0/1 status: 1 = match, 0 = not match
 *
 * No pattern matching is done.  That is, there are no wildcards.
 * But, it is still necessary to stop on either '/' or '\0',
 * and to modify s in place by translating '/' to '\0'.
 *
 */

static int
match_sfn(char *pat, char *s)
{
    char c;

    for (;;) {
        switch (c = *pat) {
        case '\0':
        case SLASH:
            return (*s == '\0');
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

/**
 * @brief Parse 'from' pattern; record substrings for back-references, etc.
 *
 * @param mmv
 * @return (-1/0) style status
 *
 */

int
parse_src_fname(mmv_t *mmv)
{
    char *p, *lastname;
    int instage;
    int c;
    sbuf_t m_fname;
    cstr_t m_home;
    char *fn;
    int rv;

    assert(check_encoding(mmv->encoding));
    assert(mmv->encoding != ENCODE_PAT);

    // XXX Use new initializer or assignment notation to set all fields
    // XXX in one statement.
    // XXX OR plan B: use a macro to set all fields of |sbuf_t| and |cstr_t|.
    fn = mmv->from;
    m_fname.s    = fn;
    m_fname.size = MAXPATLEN;
    m_fname.len  = mmv->fromlen;
    m_home.s = sys_home;
    m_home.len = sys_homelen;

    /*
     * Scan 'from' path -- expand any leading ~/
     */
    lastname = fn;
    if (fn[0] == '~' && fn[1] == SLASH) {
        rv = tilde_expand(&m_home, &m_fname);
        if (rv) {
            return (rv);
        }
        mmv->fromlen += m_home.len;
        lastname += m_home.len;
    }

    /*
     * Scan 'from' path; split on '/'
     */

    nstages = instage = 0;
    for (p = lastname; (c = *p) != '\0'; ++p) {
        switch (c) {
        case SLASH:
            lastname = p + 1;
            if (instage) {
                stager[nstages++] = p;
                instage = 0;
            }
            break;
        }
    }

    if (instage) {
        stager[nstages++] = p;
    }
    else {
        stagel[nstages] = lastname;
        stager[nstages++] = p;
    }

    return (0);
}

/**
 * @brief Parse 'to' pattern; do ~-expansion, validate back references, etc.
 *
 * @param mmv
 * @return (-1/0) style status
 *
 */

int
parse_dst_fname(mmv_t *mmv)
{
    char *p, *lastname;
    int c;

    assert(check_encoding(mmv->encoding));
    assert(mmv->encoding != ENCODE_PAT);

    /*
     * Scan 'to' path -- expand any leading ~/
     */
    lastname = mmv->to;
    if (mmv->to[0] == '~' && mmv->to[1] == SLASH) {
        if (sys_homelen + mmv->tolen > MAXPATLEN) {
            fexplain_char_pattern_too_long(stderr, mmv->to, MAXPATLEN);
            return (-1);
        }
        memmove(mmv->to + sys_homelen, mmv->to + 1, mmv->tolen);
        memmove(mmv->to, sys_home, sys_homelen);
        lastname += sys_homelen + 1;
    }

    /*
     * Scan 'to' path -- record the positions of any backreferences
     * (e.g. '#1') in the replacement pattern.
     */
    for (p = lastname; (c = *p) != '\0'; ++p) {
        int cclass;

        cclass = c;
        switch (cclass) {
        default:
            break;
        case SLASH:
            if (mmv->op & DIRMOVE) {
                printf("%s -> %s : no path allowed in target under -r.\n",
                    mmv->from, mmv->to);
                return (-1);
            }
            // XXX NOT USED; lastname = p + 1;
            break;
        }
    }

    return (0);
}

/*
 * Constants
 */

static char TOOLONG[] = "(too long)";
static char EMPTY[] = "(empty)";

static void
makerep_fnames(mmv_t *mmv)
{
    extern int repbad;

    char *pat;
    char *p;
    size_t l;
    int c;

    repbad = 0;
    p = mmv->fullrep;
    // XXX Use strncpy()-like function, instead of my own loop
    //
    for (pat = mmv->to, l = 0; (c = *pat) != '\0'; ++pat, ++l) {
        if (l >= PATH_MAX) {
            goto toolong;
        }
        *(p++) = c;
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
 * @brief  dostage_fnames ???
 *
 * @param mmv
 * @param lastend  ???  ???
 * @param pathend  ???  ???
 * @param stage    ???  ???
 * @param anylev   IN   ???
 *
 */

int
dostage_fnames(mmv_t *mmv, char *lastend, char *pathend, int stage, int anylev)
{
    DIRINFO *di;
    HANDLE *h, *hto;
    int prelen, litlen, nfils, i, k, flags, match_rv;
    FILEINFO **pf, *fdel;
    char *nto;
    REP *p;
    int ret;
    bool laststage;

    if (dbgprint_fh) {
        fprintf(dbgprint_fh, "%s:\n", __FUNCTION__);
        fprintln_str(dbgprint_fh, "    lastend=", lastend);
        fprintln_str(dbgprint_fh, "    pathend=", pathend);
        fprintf(dbgprint_fh, "    stage=%d\n", stage);
        fprintf(dbgprint_fh, "    anylev=%d\n", anylev);
    }

    ret = 1;
    laststage = (stage + 1 == nstages);

    if (!anylev) {
        prelen = stagel[stage] - lastend;
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
        lastend = stagel[stage];
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

    nfils = di->di_nfils;

    if ((mmv->op & MOVE) && !dwritable(h)) {
        printf("%s -> %s : directory %s does not allow writes.\n",
            mmv->from, mmv->to, mmv->pathbuf);
        mmv->paterr = 1;
        goto skiplev;
    }

    litlen = 0;
    while (lastend[litlen + 1] && lastend[litlen + 1] != '/') {
        ++litlen;
    }
    pf = di->di_fils + (i = ffirst(lastend, litlen, di));
    if (i < nfils) {
        do {
            if ((match_rv = trymatch(mmv, *pf, lastend)) != 0 && (match_rv == 1 || match_sfn(lastend + litlen, (*pf)->fi_name + litlen)) && keepmatch(mmv, *pf, pathend, &k, !NEED_SLASH, WANT_DIRS, laststage)) {
                if (!laststage) {
                    ret &= dostage_fnames(mmv, stager[stage], pathend + k, stage + 1, !ANY_LEVEL);
                }
                else {
                    ret = 0;
                    makerep_fnames(mmv);
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
            if (*((*pf)->fi_name) != '.' && keepmatch(mmv, *pf, pathend, &k, NEED_SLASH, WANT_DIRS, WANT_FILES)) {
                ret &= dostage_fnames(mmv, lastend, pathend + k, stage, ANY_LEVEL);
            }
        }
    }

    return (ret);
}
