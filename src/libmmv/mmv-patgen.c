/*
 * Filename: src/libmmv/mmv-patgen.c
 * Library: libmmv
 * Brief: Read and/or generate { from->to } pairs from patterns or raw pairs
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
#include <mmv.h>

/*
 * Declare external functions.
 */

extern int getpat(mmv_t *mmv);

/*
 * System
 */

extern char *sys_home;
extern size_t sys_homelen;

/*
 * Decalre external (global) data
 */

extern char USAGE[];

/**
 * @brief Parse { 'from' -> 'to' } pathname pair.
 *
 * If encoding is ENCODE_PAT, then the 'from' path is
 * scanned for filename generation syntax, ( * ? [...]),
 * and auxilliary data about the position and length
 * of each pattern matching specifier is recorded.
 *
 * If the encoding is ENCODE_NUL or ENCODE_QP, then no filename
 * generation is done and there are no special characters.
 *
 * In any case, a leading ~/ is expanded the $HOME/ for
 * both 'from' and 'to' paths.
 *
 */

static char TRAILESC[] = "%s -> %s : trailing %c is superfluous.\n";

/**
 * @brief Parse 'from' pattern; record substrings for back-references, etc.
 *
 * @param mmv
 * @return (-1/0) style status
 *
 */

int
parse_src_pattern(mmv_t *mmv)
{
    char *p, *lastname;
    int instage;
    int c;
    sbuf_t m_fname;
    cstr_t m_home;
    char *fn;
    int rv;

    assert(check_encoding(mmv->encoding));
    // XXX assert(mmv->encoding == ENCODE_PAT);

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
     * Scan 'from' path for wildcards ( * ? [...] )
     */

    pattern_t *pat = mmv->aux;
    pat->bkref_cnt = 0;
    pat->stage_cnt = 0;
    instage = 0;
    for (p = lastname; (c = *p) != '\0'; ++p) {
        switch (c) {
        case SLASH:
            lastname = p + 1;
            if (instage) {
                if (firstwild(pat->stage_cnt) == NULL) {
                    firstwild(pat->stage_cnt) = p;
                }
                if (pat->stage_cnt == pat->stage_siz) {
                    // Grow the array of descriptors
                    // for wildcards within each stage
                    void *vec = (void *) pat->stage_vec;
                    size_t old_size = pat->stage_siz;
                    size_t new_size = (old_size * 16) / 10;
                    pat->stage_vec = (stage_t *) mmv_realloc(vec, new_size);
                }
                pat->stage_vec[pat->stage_cnt].r = p;
                ++pat->stage_cnt;
                instage = 0;
            }
            break;
        case ';':
            if (lastname != p) {
                printf("%s -> %s : badly placed ;.\n",
                    mmv->from, mmv->to);
                return (-1);
            }
            __attribute__ ((fallthrough));
        case '!':
        case '*':
        case '?':
        case '[':
            if (pat->bkref_cnt == pat->bkref_siz) {
                // Grow the vector of backref descriptors
                void *vec = (void *) pat->bkref_vec;
                size_t old_size = pat->bkref_siz;
                size_t new_size = (old_size * 16) / 10;
                pat->bkref_vec = (backref_t *) mmv_realloc(vec, new_size);
            }
            ++pat->bkref_cnt;
            if (instage) {
                ++nwilds(pat->stage_cnt);
                if (firstwild(pat->stage_cnt) == NULL) {
                    firstwild(pat->stage_cnt) = p;
                }
            }
            else {
                pat->stage_vec[pat->stage_cnt].l = lastname;
                firstwild(pat->stage_cnt) = (c == ';' ? NULL : p);
                nwilds(pat->stage_cnt) = 1;
                instage = 1;
            }
            if (c != '[') {
                break;
            }

            while ((c = *(++p)) != ']') {
                switch (c) {
                case '\0':
                    printf("%s -> %s : missing ].\n",
                        mmv->from, mmv->to);
                    return (-1);
                case SLASH:
                    printf("%s -> %s : '%c' can not be part of [].\n",
                        mmv->from, mmv->to, c);
                    return (-1);
                case ESC:
                    if ((c = *(++p)) == '\0') {
                        printf(TRAILESC, mmv->from, mmv->to, ESC);
                        return (-1);
                    }
                }
            }
            break;
        case ESC:
            if ((c = *(++p)) == '\0') {
                printf(TRAILESC, mmv->from, mmv->to, ESC);
                return (-1);
            }
        }
    }

    if (instage) {
        if (firstwild(pat->stage_cnt) == NULL) {
            firstwild(pat->stage_cnt) = p;
        }
    }
    else {
        pat->stage_vec[pat->stage_cnt].l = lastname;
        nwilds(pat->stage_cnt) = 0;
        firstwild(pat->stage_cnt) = p;
    }
    pat->stage_vec[pat->stage_cnt].r = p;
    ++pat->stage_cnt;

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
parse_dst_pattern(mmv_t *mmv)
{
    char *p, *lastname;
    int c;

    assert(check_encoding(mmv->encoding));
    assert(mmv->encoding == ENCODE_PAT);

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
        size_t backref_nr;

        switch (c) {
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
        case BACKREF:
            c = *(++p);
            if (c == 'l' || c == 'u') {
                c = *(++p);
            }
            if (!isdigit(c)) {
                printf("%s -> %s : expected digit (not '%c') after '%c'.\n",
                    mmv->from, mmv->to, c, BACKREF);
                return (-1);
            }
            for (backref_nr = 0; ; backref_nr *= 10) {
                backref_nr += c - '0';
                c = *(p + 1);
                if (!isdigit(c)) {
                    break;
                }
                ++p;
            }
            pattern_t *pat = mmv->aux;
            if (backref_nr > pat->bkref_cnt) {
                printf("%s -> %s : wildcard #%zu does not exist.\n",
                    mmv->from, mmv->to, backref_nr);
                return (-1);
            }
            break;
        case ESC:
            if ((c = *(++p)) == '\0') {
                printf(TRAILESC, mmv->from, mmv->to, ESC);
                return (-1);
            }
        }
    }

    return (0);
}

/**
 * @brief Do pattern expansion on a { from->to } pair.
 *
 * @param mmv
 *
 * No return value.
 * Instead, mmv->paterr is set, in case of any error.
 *
 */

void
matchpat(mmv_t *mmv)
{
    if (parse_src_pattern(mmv)) {
        mmv->paterr = 1;
        return;
    }

    if (parse_dst_pattern(mmv)) {
        mmv->paterr = 1;
        return;
    }

    if (dostage_patterns(mmv, mmv->from, mmv->pathbuf, mmv_backref(0), 0, 0)) {
        printf("%s -> %s : no match.\n", mmv->from, mmv->to);
        mmv->paterr = 1;
    }
}

/**
 * @brief match a single { from->to } pair.
 *
 * A { from->to } pair was given on the command line.
 * Validate, parse, and match it.
 *
 */

static int
matchpats_from_argv(mmv_t *mmv, const char *cfrom, const char *cto)
{
    if ((mmv->fromlen = strlen(cfrom)) >= MAXPATLEN) {
        fexplain_char_pattern_too_long(stderr, cfrom, MAXPATLEN);
        mmv->paterr = 1;
    }

    if ((mmv->tolen = strlen(cto)) >= MAXPATLEN) {
        fexplain_char_pattern_too_long(stderr, cto, MAXPATLEN);
        mmv->paterr = 1;
    }

    if (!mmv->paterr) {
        strcpy(mmv->from, cfrom);
        strcpy(mmv->to, cto);
        matchpat(mmv);
    }

    return (mmv->paterr);
}

/**
 * @brief Read, parse, validate, and match {from->to } pairs from a file.
 *
 *
 * No { from->to } pair was given on the command line,
 * So, we read { from->to } pairs from a file.
 *
 */

static int
matchpats_from_file(mmv_t *mmv)
{
    while (getpat(mmv)) {
        matchpat(mmv);
    }
    return (mmv->paterr);
}

/**
 * @brief Do pattern expansion.  Get patterns from file or form argv.
 *
 */

static int
matchpats(mmv_t *mmv, const char *cfrom, const char *cto)
{
    if (cfrom == NULL) {
        return (matchpats_from_file(mmv));
    }

    return matchpats_from_argv(mmv, cfrom, cto);
}

/**
 * @brief Parse command-line arguments and read or generate from->to pairs
 *
 * @param mmv
 * @param argc   IN  argument count
 * @param argv   IN  arguments
 * @return errno-like status
 *
 * |argc| and |argc| are just like those in main().
 *
 */

int
patgen(mmv_t *mmv, int argc, char *const *argv)
{
    char *frompat, *topat;
    int err;

    mmv->encoding = ENCODE_PAT;
    frompat = NULL;
    topat = NULL;
    err = procargs(mmv, argc, argv, &frompat, &topat);

    if (err) {
        char *cmd = argv[0];

        if ((err >> 8) == EINVAL) {
            printf("Unknown option, -%c\n", err & 0xff);
        }
        printf(USAGE, cmd);
        return (err);
    }

    err = matchpats(mmv, frompat, topat);

    if (err) {
        return (err);
    }

    if (dbgprint_fh) {
        fdump_all_replacement_structures(dbgprint_fh, &mmv->hrep);
    }
    return (0);
}

/**
 * @brief Add a { from->to } pair of patterns to the set of |REP|s.
 *
 * @param mmv
 * @param src_fname  IN  'from' filename
 * @param dst_fname  IN  'to'   filename
 * @return errno-like status
 *
 * If the encoding is ENCODE_PAT, then the patterns are expanded,
 * but for any other encoding, there are no magic characters,
 * so the pair is guaranteed to be a simple pair of filenames.
 *
 */

int
mmv_add_pattern_pair(mmv_t *mmv, const char *src_fname, const char *dst_fname)
{
    int err;

    // XXX Ensure that repeated calls to mmv_add_pattern_pair()
    // XXX will not cause any conflicts and will not leak memory.

    mmv->fromlen = strlen(src_fname);
    mmv->tolen   = strlen(dst_fname);

    if (mmv->fromlen >= MAXPATLEN) {
        fexplain_char_pattern_too_long(stderr, src_fname, MAXPATLEN);
        mmv->paterr = 1;
    }

    if (mmv->tolen >= MAXPATLEN) {
        fexplain_char_pattern_too_long(stderr, dst_fname, MAXPATLEN);
        mmv->paterr = 1;
    }

    strcpy(mmv->from, src_fname);
    strcpy(mmv->to, dst_fname);
    err = mmv->paterr;
    if (err) {
        return (err);
    }

    err = matchpats(mmv, src_fname, dst_fname);
    if (err) {
        return (err);
    }

    if (mmv->debug_fh) {
        fdump_all_replacement_structures(mmv->debug_fh, &mmv->hrep);
    }
    return (0);
}
