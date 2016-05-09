/*
 * Filename: src/libmmv/mmv-pairs.c
 * Library: libmmv
 * Brief: Process { from->to } pairs, however they got here.
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


// We use stdio::fileno(), so define _POSIX_SOURCE
//
#if !defined(_POSIX_SOURCE)
#define _POSIX_SOURCE 1
#endif

#define _GNU_SOURCE 1

#include <stdbool.h>
#include <stdio.h>
#include <linux/limits.h>       // Import PATH_MAX
#include <ctype.h>
#include <string.h>

/* For various flavors of Unix */

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include <dirent.h>
typedef struct dirent DIRENTRY;

#include <eprint.h>
#include <dbgprint.h>

#define IMPORT_POLICY
#define IMPORT_RFLAGS
#define IMPORT_OPS
#define IMPORT_REP
#define IMPORT_DIRINFO
#define IMPORT_FILEINFO
#define IMPORT_ALLOC
#define IMPORT_DEBUG
#include <mmv-impl.h>

static inline bool
streq(const char *s1, const char *s2)
{
    return (strcmp(s1, s2) == 0);
}

extern FILE *ask_filename(const char *prompt, char *fname_buf, size_t bufsz);
extern int ask_yesno(const char *prompt, int failact);
extern void quit(void);
extern int mmv_copy(mmv_t *mmv, FILEINFO *f, size_t len);
extern int mmv_unlink(char *fname);
extern void eprint_filename(char *fname);
extern void eexplain_err(int err);

extern FILE *dbgprint_fh;
extern int gotsig;

static char TEMP[] = "$$mmvtmp.";

/**
 * @brief Compare 2 REPDICT structures
 *  @param vp1  pointer to 1st REPDICT
 *  @param vp2  pointer to 2nd REPDICT
 *  @return  { negative , 0 , positive } int, suitable for qsort().
 *
 *  Comparison is by dto, then nto, then by rd_i.
 */

static int
rdcmp(const void *vp1, const void *vp2)
{
    const REPDICT *rd1 = (const REPDICT *)vp1;
    const REPDICT *rd2 = (const REPDICT *)vp2;
    int ret;

    ret = rd1->rd_dto - rd2->rd_dto;

    if (ret == 0) {
        ret = strcmp(rd1->rd_nto, rd2->rd_nto);
    }
    if (ret == 0) {
        ret = rd1->rd_i - rd2->rd_i;
    }
    return (ret);
}

/**
 * @brief Do two REPDICTs refer to the same target?
 *
 */
static bool
rd_same_target(const REPDICT *rd1, const REPDICT *rd2)
{
    return (rd1->rd_dto == rd2->rd_dto && streq(rd1->rd_nto, rd2->rd_nto));
}

/**
 * @brief Populate a repdict with information from the list of replacement
 *
 * @param mmv
 * @param rd   OUT   A newly allocate |REPDICT| to be initialized.
 *
 * A |REPDICT| is an array, partly because we need to sort its entries.
 * The list of { from->to } patterns is a linked list.
 * We populate the |REPDICT| with the information needed to sort
 * and index its entries.  That information is in the corresponding
 * entries in the linked list (mmv->hrep).
 *
 */

static void
init_repdict(mmv_t *mmv, REPDICT *rd)
{
    REPDICT *prd;
    REP *p;
    size_t i;

    p = (&mmv->hrep)->r_next;
    prd = rd;
    i = 0;
    while (p != NULL) {
        prd->rd_p = p;
        prd->rd_dto = p->r_hto->h_di;
        prd->rd_nto = p->r_nto;
        prd->rd_i = i;
        p = p->r_next;
        ++prd;
        ++i;
    }
}

/**
 * @brief Mark a REPDICT as bad
 *
 * @param mmv
 * @param rd  OUT   The |REPDICT| to be marked
 *
 * Mark a REPDICT as bad.
 * Also, update counts of number of valid REPs and of bad REPs.
 */

static void
mark_collision(mmv_t *mmv, REPDICT *rd)
{
    rd->rd_p->r_flags |= R_SKIP;
    rd->rd_p->r_ffrom->fi_rep = &mmv->mistake;
    --mmv->nreps;
    ++mmv->badreps;
}

/**
 * @brief Sort all REPDICT structures; then check for duplicates
 *
 * @param rd  An array of REPDICT structures
 * @return void
 *
 * Note:
 *   There is no paramter given for the size of the array,
 *   because that is in the global variable, 'nreps'.
 *
 * Sort the array so that checking for duplicates is as easy
 * as comparing two consecutive array elements.
 *
 */

static void
check_duplicates(mmv_t *mmv, REPDICT *rd)
{
    REPDICT *prd;
    int oldnreps;
    int mult;
    int i;

    qsort(rd, mmv->nreps, sizeof (REPDICT), rdcmp);

    /*
     * nreps can change while we are checking for collisions,
     * so keep the old value to tell how many elements to visit.
     */
    oldnreps = mmv->nreps;

    /*
     * Scan the entire sorted REPDICT array, visiting and comparing each pair.
     * If any pairs are equal, then there is a collision.
     * Sorting the array also means that an entire run of collisions
     * with the same target can be grouped together into one report,
     * with multiple source names, but reporting the common target
     * only once.
     *
     * Since we are visiting pairs, we stop just shy of attempting
     * to visit the last element in the array.
     */
    mult = 0;
    for (i = 0, prd = rd; i < oldnreps; ++prd, ++i) {
        if (i < oldnreps - 1 && rd_same_target(prd, prd + 1)) {
            /*
             * A pair have the same target
             * report the source name only,
             * because this could be a run of collisions.
             */
            if (!mult) {
                mult = 1;
            }
            else {
                printf(" , ");
            }
            printf("%s%s",
                prd->rd_p->r_hfrom->h_name,
                prd->rd_p->r_ffrom->fi_name);
            mark_collision(mmv, prd);
        } else if (mult) {
            /*
             * This pair does not have the same target,
             * either because we are at the last element
             * or because the targets are unequal.
             * But, we are at the end of a run of collisions,
             * so report the last source name in this run,
             * and then report the target name that is common
             * to all the { source name , target name } pairs.
             */
            printf(" , %s%s -> %s%s : collision.\n",
                   prd->rd_p->r_hfrom->h_name,
                   prd->rd_p->r_ffrom->fi_name,
                   prd->rd_p->r_hto->h_name,
                   prd->rd_nto);
            mark_collision(mmv, prd);
            mult = 0;
        }
    }
}

/**
 * @ brief analyze all the replacement structures; check for collisions
 *
 * @param mmv
 *
 */

static void
check_collisions(mmv_t *mmv)
{
    REPDICT *rd;
    size_t rd_size;

    if (mmv->nreps == 0) {
        return;
    }

    // Remember the size of this allocation
    rd_size = mmv->nreps * sizeof (REPDICT);
    rd = (REPDICT *) myalloc(rd_size);
    init_repdict(mmv, rd);
    check_duplicates(mmv, rd);
    chgive(rd, rd_size);
}

/**
 * @brief Walk all |REP|s and determine a proper order of operations
 *
 * Some moves can only be accomplished by first moving some targets
 * out of the way by renaming them to temporary filenames.
 *
 * Otherwise, cycles, adn even simple swaps would not be feasible.
 *
 */

static void
findorder(mmv_t *mmv)
{
    REP *p, *q, *t, *first, *pred;
    FILEINFO *fi;

    for (q = &mmv->hrep, p = q->r_next; p != NULL; q = p, p = p->r_next) {
        if (p->r_flags & R_SKIP) {
            q->r_next = p->r_next;
            p = q;
        }
        else if ((fi = p->r_fdel) == NULL || (pred = fi->fi_rep) == NULL || pred == &mmv->mistake) {
            continue;
        }
        else if ((first = pred->r_first) == p) {
            p->r_flags |= R_ISCYCLE;
            pred->r_flags |= R_ISALIASED;
            if (mmv->op & MOVE) {
                p->r_fdel = NULL;
            }
        }
        else {
            if (mmv->op & MOVE) {
                p->r_fdel = NULL;
            }
            while (pred->r_thendo != NULL) {
                pred = pred->r_thendo;
            }
            pred->r_thendo = p;
            for (t = p; t != NULL; t = t->r_thendo) {
                t->r_first = first;
            }
            q->r_next = p->r_next;
            p = q;
        }
    }
}

/**
 * @brief Print out a chain of move operations
 *
 * @param p  IN
 *
 */

static void
printchain(mmv_t *mmv, REP *p)
{
    if (p->r_thendo != NULL) {
        printchain(mmv, p->r_thendo);
    }
    printf("%s%s -> ", p->r_hfrom->h_name, p->r_ffrom->fi_name);
    ++mmv->badreps;
    --mmv->nreps;
    p->r_ffrom->fi_rep = &mmv->mistake;
}

/**
 * @brief Chains are allowed for mv, but not for copy or link ops; explain.
 *
 */

static void
nochains(mmv_t *mmv)
{
    REP *p, *q;

    for (q = &mmv->hrep, p = q->r_next; p != NULL; q = p, p = p->r_next) {
        if (p->r_flags & R_ISCYCLE || p->r_thendo != NULL) {
            printchain(mmv, p);
            printf("%s%s : no chain copies allowed.\n",
                p->r_hto->h_name, p->r_nto);
            q->r_next = p->r_next;
            p = q;
        }
    }
}


/**
 * @brief Scan all |REP|s; take care of files marked for deletion.
 *
 * @param mmv
 * @param pkilldel   IN  Function to call for each  delete op
 *
 */

static void
scandeletes(mmv_t *mmv, int (*pkilldel) (mmv_t *, REP *))
{
    REP *p, *q, *n;

    for (q = &mmv->hrep, p = q->r_next; p != NULL; q = p, p = p->r_next) {
        if (p->r_fdel != NULL) {
            while ((*pkilldel) (mmv, p)) {
                --mmv->nreps;
                p->r_ffrom->fi_rep = &mmv->mistake;
                if ((n = p->r_thendo) != NULL) {
                    if (mmv->op & MOVE) {
                        n->r_fdel = p->r_ffrom;
                    }
                    n->r_next = p->r_next;
                    q->r_next = p = n;
                }
                else {
                    q->r_next = p->r_next;
                    p = q;
                    break;
                }
            }
        }
    }
}

/**
 * @brief Determine if a file is writable; keep a record of it
 *
 * @param mmv
 * @param hname   IN  ???
 * @param f       IN  ???
 * @return status
 *
 */

static int
fwritable(mmv_t *mmv, const char *hname, FILEINFO *f)
{
    unsigned int r;

    if (f->fi_stflags & FI_KNOWWRITE) {
        return ((f->fi_stflags & FI_CANWRITE) != 0);
    }

    strcpy(mmv->fullrep, hname);
    strcat(mmv->fullrep, f->fi_name);
    r = !access(mmv->fullrep, W_OK) ? FI_CANWRITE : 0;
    f->fi_stflags |= FI_KNOWWRITE | r;
    return (r != 0);
}

/**
 * @brief Report the problem when a delete goes badly.
 *
 * @param mmv
 * @param p    IN  The |REP| in question
 *
 */

static int
baddel(mmv_t *mmv, REP *p)
{
    HANDLE *hfrom = p->r_hfrom, *hto = p->r_hto;
    FILEINFO *fto = p->r_fdel;
    char *t = fto->fi_name, *f = p->r_ffrom->fi_name;
    char *hnf = hfrom->h_name, *hnt = hto->h_name;

    if (mmv->delstyle == NODEL && !(p->r_flags & R_DELOK) && !(mmv->op & APPEND)) {
        printf("%s%s -> %s%s : old %s%s would have to be %s.\n",
            hnf, f, hnt, t, hnt, t,
            (mmv->op & OVERWRITE) ? "overwritten" : "deleted");
    }
    else if (fto->fi_rep == &mmv->mistake) {
        printf("%s%s -> %s%s : old %s%s was to be done first.\n",
            hnf, f, hnt, t, hnt, t);
    }
    else if (fto->fi_stflags & FI_ISDIR) {
        printf("%s%s -> %s%s : %s%s%s is a directory.\n",
            hnf, f, hnt, t, (mmv->op & APPEND) ? "" : "old ", hnt, t);
    }
    else if ((fto->fi_stflags & FI_NODEL) && !(mmv->op & (APPEND | OVERWRITE))) {
        printf("%s%s -> %s%s : old %s%s lacks delete permission.\n",
            hnf, f, hnt, t, hnt, t);
    }
    else if ((mmv->op & (APPEND | OVERWRITE)) && !fwritable(mmv, hnt, fto)) {
        printf("%s%s -> %s%s : %s%s %s.\n",
            hnf, f, hnt, t, hnt, t, "lacks write permission");
    }
    else {
        return (0);
    }
    ++mmv->badreps;
    return (1);
}

/**
 * @brief When there is a problem, ask whether to delete or overwrite.
 *
 * @param mmv
 * @param p    IN  The |REP| in question.
 *
 * This only makes sense when ASKDEL is set.
 *
 */

static int
skipdel(mmv_t *mmv, REP *p)
{
    if (p->r_flags & R_DELOK) {
        return (0);
    }

    eprintf("%s%s -> %s%s : ",
        p->r_hfrom->h_name, p->r_ffrom->fi_name, p->r_hto->h_name, p->r_nto);

    if (!fwritable(mmv, p->r_hto->h_name, p->r_fdel)) {
        eprintf("old %s%s lacks write permission. delete it",
            p->r_hto->h_name, p->r_nto);
    }
    else {
        eprintf("%s old %s%s",
            (mmv->op & OVERWRITE) ? "overwrite" : "delete",
            p->r_hto->h_name,
            p->r_nto);
    }
    return (!ask_yesno("? ", -1));
}

/**
 * @brief In case of a problem, ask whether to proceed, or quit
 *
 * @param mmv
 *
 */

static void
goonordie(mmv_t *mmv)
{
    if ((mmv->paterr || mmv->badreps) && mmv->nreps > 0) {
        eprint("Not everything specified can be done.");
        if (mmv->badstyle == ABORTBAD) {
            eprint(" Aborting.\n");
            exit(1);
        }
        else if (mmv->badstyle == SKIPBAD) {
            eprint(" Proceeding with the rest.\n");
        }
        else if (!ask_yesno(" Proceed with the rest? ", -1)) {
            exit(1);
        }
    }
}


/**
 * @brief Show a single { from->to } pair (|REP|) that has been done.
 *
 * @param f    IN  stdio stream; where to print
 * @param mmv
 * @param p    IN  |REP| to be printed
 *
 */

static void
fshow_done_rep(FILE *f, mmv_t *mmv, REP *p)
{
    fprint_filename(f, p->r_hfrom->h_name);
    fprint_filename(f, p->r_ffrom->fi_name);

    fprintf(f, " %c%c ", 
        p->r_flags & R_ISALIASED ? '=' : '-',
        p->r_flags & R_ISCYCLE ? '^' : '>');
    fprint_filename(f, p->r_hto->h_name);
    fprintf(f, "%s : done", p->r_nto);
    if (p->r_fdel != NULL && !(mmv->op & APPEND)) {
        fputs(" (*)", f);
    }
    fputs("\n", f);
}


/**
 * @brief Show what operations have been done.
 *
 * @param mmv
 * @param fin  IN  |REP| that marks the end of what has already been done.
 *
 */

static void
show_done_all(mmv_t *mmv, REP *fin)
{
    REP *first, *p;

    for (first = mmv->hrep.r_next; first != NULL; first = first->r_next) {
        for (p = first; p != NULL && p != fin; p = p->r_thendo) {
            fshow_done_rep(mmv->outfile, mmv, p);
        }
    }
}

/**
 * @brief ???
 *
 * @param mmv
 * @param first  IN  ???
 * @param p      IN  ???
 *
 */

static int
snap(mmv_t *mmv, REP *first, REP *p)
{
    char fname[PATH_MAX];
    int redirected = 0;

    if (mmv->noex) {
        exit(1);
    }

    mmv->failed = 1;
    signal(SIGINT, breakstat);
    if (mmv->badstyle == ASKBAD && isatty(fileno(stdout)) && ask_yesno("Redirect standard output to file? ", 0)) {
        redirected = 1;
        umask(mmv->oldumask);
        while (true) {
            int err;

            mmv->outfile = ask_filename("File name> ", fname, sizeof (fname));
            if (mmv->outfile != NULL) {
                break;
            }
            err = errno;
            eprint("open('");
            eprint_filename(fname);
            eprint("') failed.\n");
            eexplain_err(err);
        }
    }
    if (redirected || !mmv->verbose) {
        show_done_all(mmv, p);
    }
    fprintf(mmv->outfile, "The following left undone:\n");
    mmv->noex = 1;
    return (first != p);
}

/**
 * @brief Search for a given simple filename in a directory of |FILEINFO|s.
 *
 * @param s  IN   filename to find
 * @param d  IN   directory of |FILEINFO|
 * @return  FILEINFO of found filename, or NULL if not found
 *
 * Method:
 *   |FILEINFO| structures in a |DIRINFO| are sorted by name,
 *   so we can do a binary search, not just a linear search.
 *
 */

static FILEINFO *
fsearch(const char *s, DIRINFO *d)
{
    FILEINFO **fils = d->di_fils;
    int nfils = d->di_nfils;
    int first, k, last, res;

    for (first = 0, last = nfils - 1; ; ) {
        if (last < first) {
            return (NULL);
        }
        k = (first + last) >> 1;
        if ((res = strcmp(s, fils[k]->fi_name)) == 0) {
            return (fils[k]);
        }
        if (res < 0) {
            last = k - 1;
        }
        else {
            first = k + 1;
        }
    }
}

/**
 * @brief Construct a unique (not existing) temporary filename
 *
 * @param mmv
 * @param p    IN  |REP|, contains the info we need to generate a temp file
 * @return The sequence number used to make the unique name.
 *
 * The returned sequence number, combined with the given |REP|,
 * is sufficient to recreate the full name of the temp file (AKA the alias).
 *
 */

static int
make_alias_fname(mmv_t *mmv, REP *p)
{
    char *fstart;
    char *seqstart;
    int seq;

    fstart = mmv->pathbuf + strlen(mmv->pathbuf);
    strcpy(fstart, TEMP);
    seqstart = fstart + strlen(TEMP);
    seq = 0;
    while (true) {
        sprintf(seqstart, "%03d", seq);
        if (fsearch(fstart, p->r_hto->h_di) == NULL) {
            break;
        }
        ++seq;
    }
    return (seq);
}

/**
 * @brief Rename an alias (temporary file) back to where it belongs.
 *
 * @param mmv
 * @param first  IN  |REP| marking the start of a chain  (???)
 * @param p      IN  |REP| containing the alias to be moved back
 * @return The sequence number of the temporary filename
 *
 */

static int
movealias(mmv_t *mmv, REP *first, REP *p, int *pprintaliased)
{
    int seq;
    int rv;
    int err;

    strcpy(mmv->pathbuf, p->r_hto->h_name);
    seq = make_alias_fname(mmv, p);
    rv = rename(mmv->fullrep, mmv->pathbuf);
    if (rv) {
        err = errno;
        eprint_filename(mmv->fullrep);
        fputs(" -> ", stderr);
        eprint_filename(mmv->pathbuf);
        fputs(" has failed.\n", stderr);
        eexplain_err(err);
        *pprintaliased = snap(mmv, first, p);
    }
    return (seq);
}


/**
 * @brief ???
 *
 * @param mmv
 * @param first          IN  ???
 * @param p              IN  ???
 * @param pprintaliased  IN  ???
 *
 */

static size_t
appendalias(mmv_t *mmv, REP *first, REP *p, int *pprintaliased)
{
    struct stat fstat;
    size_t ret = SIZE_UNLIMITED;

    if (stat(mmv->fullrep, &fstat)) {
        eprintf("append cycle stat on '%s' has failed.\n", mmv->fullrep);
        *pprintaliased = snap(mmv, first, p);
    }
    else {
        ret = fstat.st_size;
    }

    return (ret);
}

/*
 * @brief Copy one file to another, and if successful, remove the original.
 *
 * @param mmv
 * @param p   IN |REP| specifying source and destination
 * @return status
 *
 * This copy-then-move is needed in cases where rename() does not work,
 * such as cross-filesystem moves.
 *
 */

static int
copymove(mmv_t *mmv, REP *p)
{
    return (mmv_copy(mmv, p->r_ffrom, -1) || mmv_unlink(mmv->pathbuf));
}

/**
 * @brief Do replacements
 *
 * @param mmv
 *
 * Perform the appropriate move/copy/append/link ops on all |REP|s.
 *
 */

static void
doreps(mmv_t *mmv)
{
    REP *first;
    int k;
    int alias = 0;

    signal(SIGINT, breakrep);

    for (first = mmv->hrep.r_next, k = 0; first != NULL; first = first->r_next) {
        REP *p;
        int printaliased;

        printaliased = 0;
        for (p = first; p != NULL; p = p->r_thendo, ++k) {
            size_t aliaslen;
            char *fstart;

            if (gotsig) {
                fflush(stdout);
                eprint("User break.\n");
                printaliased = snap(mmv, first, p);
                gotsig = 0;
            }
            strcpy(mmv->fullrep, p->r_hto->h_name);
            strcat(mmv->fullrep, p->r_nto);
            if (!mmv->noex && (p->r_flags & R_ISCYCLE)) {
                if (mmv->op & APPEND) {
                    aliaslen = appendalias(mmv, first, p, &printaliased);
                }
                else {
                    alias = movealias(mmv, first, p, &printaliased);
                }
            }
            strcpy(mmv->pathbuf, p->r_hfrom->h_name);
            fstart = mmv->pathbuf + strlen(mmv->pathbuf);
            if ((p->r_flags & R_ISALIASED) && !(mmv->op & APPEND)) {
                sprintf(fstart, "%s%03d", TEMP, alias);
            }
            else {
                strcpy(fstart, p->r_ffrom->fi_name);
            }

            if (!mmv->noex) {
                const char *op_str;
                int rv;

                if (p->r_fdel != NULL && !(mmv->op & (APPEND | OVERWRITE))) {
                    op_str = "unlink";
                    rv = mmv_unlink(mmv->fullrep);
                }
                if (mmv->op & (COPY | APPEND)) {
                    size_t copy_len;

                    op_str = "copy";
                    copy_len = p->r_flags & R_ISALIASED ? aliaslen : SIZE_UNLIMITED;
                    rv = mmv_copy(mmv, p->r_ffrom, copy_len);
                }
                else if (mmv->op & HARDLINK) {
                    op_str = "link";
                    rv = link(mmv->pathbuf, mmv->fullrep);
                }
                else if (mmv->op & SYMLINK) {
                    op_str = "symlink";
                    rv = symlink(mmv->pathbuf, mmv->fullrep);
                }
                else if (p->r_flags & R_ISX) {
                    op_str = "copymove";
                    rv = copymove(mmv, p);
                }
                else {
                    op_str = "rename";
                    rv = rename(mmv->pathbuf, mmv->fullrep);
                }

                dbg_printf("op_str=%s\n", op_str);

                if (rv) {
                    int err;

                    err = errno;
                    eprint_filename(mmv->pathbuf);
                    fputs(" -> ", stderr);
                    eprint_filename(mmv->fullrep);
                    fputs(" ", stderr);
                    fputs(op_str, stderr);
                    fputs(" has failed.\n", stderr);
                    eexplain_err(err);
                    printaliased = snap(mmv, first, p);
                }
            }

            if (mmv->verbose || mmv->noex) {
                if (p->r_flags & R_ISALIASED && !printaliased) {
                    strcpy(fstart, p->r_ffrom->fi_name);
                }
                fprintf(mmv->outfile, "%s %c%c %s%s%s\n",
                        mmv->pathbuf,
                        p->r_flags & R_ISALIASED ? '=' : '-',
                        p->r_flags & R_ISCYCLE ? '^' : '>',
                        mmv->fullrep,
                        (p->r_fdel != NULL && !(mmv->op & APPEND)) ? " (*)" : "", mmv->noex ? "" : " : done");
            }
        }
    }

    if (k != mmv->nreps) {
        eprintf("Strange, did %d reps; %d were expected.\n", k, mmv->nreps);
    }
    if (k == 0) {
        eprint("Nothing done.\n");
    }
}

/**
 * @brief Given a set of { from->to } pairs, validate, analyze, and perform
 *
 * @param mmv
 *
 * Given that somehow a set of { from->to } pairs have been read
 * or generated -- we do not care how -- do all the heavy lifting.
 * That is, analyze the set of replacements, validate them,
 * find proper order of operations, check for collisions,
 * check for cycles, etc.  then do all the operations.
 *
 */

int
mmv_pairs(mmv_t *mmv)
{
    if (!(mmv->op & APPEND)) {
        check_collisions(mmv);
    }

    findorder(mmv);
    if (mmv->op & (COPY | LINK)) {
        nochains(mmv);
    }
    scandeletes(mmv, baddel);
    goonordie(mmv);
    if (!(mmv->op & APPEND) && mmv->delstyle == ASKDEL) {
        scandeletes(mmv, skipdel);
    }
    doreps(mmv);
    return (mmv->failed ? 2 : mmv->nreps == 0 && (mmv->paterr || mmv->badreps));
}
