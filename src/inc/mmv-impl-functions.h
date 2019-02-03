/*
 * Filename: src/inc/mmv-impl-functions.h
 * Library: libmmv
 * Brief: Define the functions used to _implement_ libmmv library
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

#ifndef MMV_IMPL_FUNCTIONS_H
#define MMV_IMPL_FUNCTIONS_H

/*
 * Define the functions used to _implement_ libmmv.
 *
 * Note that data types are defined in mmv-impl-data.h
 *
 * The signatures of _all_ functions defined here do not rely
 * on any of the data types defined in mmv-impl-data.h,
 * only on "standard" data types and pointers to opaque data types.
 * What structure pointers are used, make use only of the
 * structure declaration.  No functions defined here actually
 * read or write (or even mention) structure member names,
 * and do not to know the size of any structure, only how to pass
 * pointer.
 *
 */

#include <sys/types.h>
#include <strbuf.h>


struct backref;
typedef struct backref backref_t;

struct stage;
typedef struct stage stage_t;

struct pattern;
typedef struct pattern pattern_t;


extern void *mmv_alloc(size_t sz);
extern void *mmv_realloc(void *ptr, size_t sz);
extern void *challoc(size_t sz, unsigned int which);
extern void chgive(void *p, size_t sz);

#ifdef IMPORT_DEBUG

extern void fdump_replacement_structure(FILE *f, REP *rp);
extern void fdump_all_replacement_structures(FILE *f, REP *head);

#endif /* IMPORT_DEBUG */

extern void fexplain_sbuf_pattern_too_long(FILE *f, sbuf_t *pattern);
extern void fexplain_char_pattern_too_long(FILE *f, const char *pattern, size_t maxlen);

extern void fprint_filename(FILE *, const char *fname);
extern void fexplain_err(FILE *f, int err);
extern void explain_err(int err);
extern void eexplain_err(int err);

// ********** mmv-dostage-main.c

// ********** mmv-dostage-common.c

extern void init_dostage(void);
extern bool check_encoding(enum encode e);
extern int tilde_expand(cstr_t *home, sbuf_t *fname);
extern int trymatch(mmv_t *mmv, FILEINFO *ffrom, char *pat);
extern int keepmatch(mmv_t *mmv, FILEINFO *ffrom, char *pathend, int *pk, int needslash, int dirs, bool fils);
extern int badrep(mmv_t *mmv, HANDLE *hfrom, FILEINFO *ffrom, HANDLE **phto, char **pnto, FILEINFO **pfdel, int *pflags);
extern int ffirst(char *s, int n, DIRINFO *d);
extern HANDLE *checkdir(const char *p, char *pathend, int which);
extern unsigned int dwritable(HANDLE *h);

// ********** mmv-dostage-patterns.c

extern int match(char *pat, char *s, backref_t *bkref);
extern int dostage_patterns(mmv_t *mmv, char *lastend, char *pathend, backref_t *bkref, int stage, int anylev);

// ********** mmv-dostage-fnames.c

extern int dostage_fnames(mmv_t *mmv, char *lastend, char *pathend, int stage, int anylev);

// ********** mmv-debug.c

extern void fdump_all_replacement_structures(FILE *f, REP *head);
extern void fprint_vis(FILE *f, const char *str);
extern void fprint_str(FILE *f, const char *var, const char *str);
extern void fprintln_str(FILE *f, const char *var, const char *str);
extern void dbg_println_str(const char *var, const char *str);

// ********** mmv-sys.c

extern void init_sys(void);

// ********** mmv-util.c

extern void quit(void);
extern void mmv_abort(void);


typedef void sigreturn_t;

extern sigreturn_t breakout(int s);
extern sigreturn_t breakrep(int s);
extern sigreturn_t breakstat(int s);

#endif /* MMV_IMPL_FUNCTIONS_H */
