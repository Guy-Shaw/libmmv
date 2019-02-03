/*
 * Filename: src/libmmv/mmv-dostage-common.c
 * Library: libmmv
 * Brief: Low-level functions that support dostage_patterns(), dostage_fnames()
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

#include <mmv-impl.h>

#define INITROOM 10

static inline char *
mydup(char *s)
{
    return (strcpy((char *)challoc(strlen(s) + 1, 0), (s)));
}

/*
 * Constants
 */

static char TOOLONG[] = "(too long)";
static char SLASHSTR[] = {SLASH, '\0'};
static char dir_self[] = ".";

/*
 * system
 */

extern size_t name_max;
extern uid_t uid;

/*
 * Declare global data.
 */

int direrr;
DIRINFO **dirs;
size_t ndirs;

HANDLE **handles;
size_t nhandles;
size_t handleroom;
char badhandle_name[] = "\200";
HANDLE badhandle = {badhandle_name, NULL, 0};
HANDLE *(lasthandle[2]) = {&badhandle, &badhandle};
int repbad;

/*
 * Private to file mmv-dostage-common.c
 */

static size_t dirroom;

void
init_dostage(void)
{
    if (dirs) {
        return;
    }

    dirroom = handleroom = INITROOM;
    dirs = (DIRINFO **) mmv_alloc(dirroom * sizeof (DIRINFO *));
    ndirs = 0;

    handles = (HANDLE **) mmv_alloc(handleroom * sizeof (HANDLE *));
    nhandles = 0;
}

/**
 * @brief Validate an encoding.
 * @param e  IN  encoding value to be validated
 *
 * @return true=valid, false=invlaid
 *
 * On failure, print an error message, then return false.
 *
 */

bool
check_encoding(enum encode e)
{
    switch(e) {
    case ENCODE_NONE:
    case ENCODE_PAT:
    case ENCODE_QP:
    case ENCODE_VIS:
    case ENCODE_XNN:
    case ENCODE_NUL:
        return (true);
    default:
        eprintf("Invalid encoding, '%u'.\n", (unsigned int)e);
        return (false);
    }
}

/*
 * @brief Perform shell-style tilde expansion on a given file name.
 * @param home    IN     The home directory, use to expand '~/'
 * @param fname   INOUT  The filename to be expanded (maybe).
 * @return errno-style status  (0: success, non-zero: errno)
 *
 */
int
tilde_expand(cstr_t *home, sbuf_t *fname)
{
    char *fn = fname->s;

    if (!(fn[0] == '~' && fn[1] == SLASH)) {
        return (0);
    }

    if (home->len + fname->len > fname->size) {
        fexplain_sbuf_pattern_too_long(stderr, fname);
        return (ENAMETOOLONG);
    }

    // Shift the filename to the right to make room for |home|.
    // Then, move |home| to its place as the start of |fname|.
    memmove(fn + home->len, fn + 1, fname->len);
    memmove(fn, home->s, home->len);
    fname->len += home->len;
    return (0);
}

/**
 * @brief Does a file name match a pattern?
 *
 * @param mmv
 * @param ffrom  IN  'from' filename to match
 * @param pat    IN  pattern that |ffrom| is to matched against
 * @return (-1/0) style status
 *
 */

int
trymatch(mmv_t *mmv, FILEINFO *ffrom, char *pat)
{
    char *p;

    if (ffrom->fi_rep != NULL) {
        return (0);
    }

    p = ffrom->fi_name;

    if (*p == '.') {
        if (p[1] == '\0' || (p[1] == '.' && p[2] == '\0')) {
            return (strcmp(pat, p) == 0);
        }
        else if (!mmv->matchall && *pat != '.') {
            return (0);
        }
    }
    return (-1);
}

/**
 * @brief  getstat  ???
 *
 * @param ffull   ???  ???
 * @param f       ???  ???
 * @return exit-style status.  0 = OK, non-zero = error.
 *
 */

static int
getstat(char *ffull, FILEINFO *f)
{
    struct stat fstat;
    unsigned int flags;

    if ((flags = f->fi_stflags) & FI_STTAKEN) {
        return ((flags & FI_LINKERR) != 0);
    }
    flags |= FI_STTAKEN;
    if (stat(ffull, &fstat)) {
        eprintf("Strange, couldn't stat %s.\n", ffull);
        // XXX Use libexplain
        quit();
    }
    if (S_ISDIR(fstat.st_mode)) {
        flags |= FI_ISDIR;
    }
    f->fi_stflags = flags;
    f->fi_mode = fstat.st_mode;
    return (0);
}

/**
 * @brief keepmatch()  ???
 *
 * @param mmv
 * @param ffrom      ???  ???
 * @param pathend    ???  ???
 * @param pk         ???  ???
 * @param needslash  ???  ???
 * @param dirs       C-int-bool  Whether we want directories
 * @param fils       C-int-bool  Whether we want regular files
 * @return 0/1 status
 *
 *
 */

int
keepmatch(mmv_t *mmv, FILEINFO *ffrom, char *pathend, int *pk, int needslash, int dirs, bool fils)
{
    *pk = strlen(ffrom->fi_name);
    if (pathend - mmv->pathbuf + *pk + needslash >= PATH_MAX) {
        *pathend = '\0';
        printf("%s -> %s : search path %s%s too long.\n",
            mmv->from, mmv->to, mmv->pathbuf, ffrom->fi_name);
        mmv->paterr = 1;
        return (0);
    }
    strcpy(pathend, ffrom->fi_name);
    getstat(mmv->pathbuf, ffrom);
    if ((ffrom->fi_stflags & FI_ISDIR) ? !dirs : !fils) {
        return (0);
    }

    if (needslash) {
        strcpy(pathend + *pk, SLASHSTR);
        (*pk)++;
    }
    return (1);
}

/**
 * @brief getpath()  ???
 *
 * @param mmv
 * @param tpath  ???  ???
 *
 */

static char *
getpath(mmv_t *mmv, char *tpath)
{
    char *pathstart, *pathend, c;

    pathstart = mmv->fullrep;

    pathend = pathstart + strlen(pathstart) - 1;
    while (pathend >= pathstart && *pathend != SLASH) {
        --pathend;
    }
    ++pathend;

    c = *pathend;
    *pathend = '\0';
    strcpy(tpath, mmv->fullrep);
    *pathend = c;
    return (pathend);
}

/*
 * @brief Search a sorted array of filenames.
 * @param s    IN  key: the filename to search for
 * @param d    IN  A sorted list of directory entries to be searched
 * @return The FILEINFO of the found filename, or NULL if not found.
 *
 * Method:
 *   The filenames to be searched are in an array and are sorted,
 *   so do a binary search.
 *
 */
static FILEINFO *
fsearch(const char *s, DIRINFO const *d)
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
 * @brief  Compare two |FILEINFO| structures by filename.
 *
 * @param  vp1  IN  comparand
 * @param  vp2  IN  comparand
 * @return (-,0,+) comparison result (a la strcmp())
 *
 * This is a callback function used by qsort().
 * qsort() takes void pointers.  We use pointers to |FILEINFO| structures.
 *
 */

static int
fcmp(const void *vp1, const void *vp2)
{
    const FILEINFO * const *pf1 = (const FILEINFO * const *)vp1;
    const FILEINFO * const *pf2 = (const FILEINFO * const *)vp2;
    return (strcmp((*pf1)->fi_name, (*pf2)->fi_name));
}

/**
 * @brief Snarf info on all files in a directory.
 *
 * @param p       IN   Path to directory
 * @param di      OUT  Directory information to be populated
 * @param sticky  IN   ???
 *
 * After the directory is scanned, entries in the array, |di|
 * is dorted by simple filename.
 *
 */

static void
takedir(const char *p, DIRINFO *di, int sticky)
{
    int cnt, room;
    DIRENTRY *dp;
    FILEINFO *f, **fils;
    DIR *dirp;

    if ((dirp = opendir(p)) == NULL) {
        eprintf("Strange, can't scan %s.\n", p);
        // XXX use libexplain
        quit();
    }
    room = INITROOM;
    di->di_fils = fils = (FILEINFO **) mmv_alloc(room * sizeof (FILEINFO *));
    cnt = 0;
    while ((dp = readdir(dirp)) != NULL) {
        if (cnt == room) {
            room *= 2;
            fils = (FILEINFO **) mmv_alloc(room * sizeof (FILEINFO *));
            memcpy(fils, di->di_fils, cnt * sizeof (FILEINFO *));
            chgive(di->di_fils, cnt * sizeof (FILEINFO *));
            di->di_fils = fils;
            fils = di->di_fils + cnt;
        }
        *fils = f = (FILEINFO *) challoc(sizeof (FILEINFO), 1);
        f->fi_name = mydup(dp->d_name);
        f->fi_stflags = sticky;
        f->fi_rep = NULL;
        ++cnt;
        ++fils;
    }
    closedir(dirp);
    qsort(di->di_fils, cnt, sizeof (FILEINFO *), fcmp);
    di->di_nfils = cnt;
}

/**
 * @brief Add a new handle to |handles| array.
 *
 * @param new_name  IN  New filename to be added
 * @return pointer to new, initialized handle
 *
 * Allocation failure is not an option.
 *
 */

static HANDLE *
hadd(const char *new_name)
{
    HANDLE **newhandles, *h;

    if (nhandles == handleroom) {
        handleroom *= 2;
        newhandles = (HANDLE **) mmv_alloc(handleroom * sizeof (HANDLE *));
        memcpy(newhandles, handles, nhandles * sizeof (HANDLE *));
        chgive(handles, nhandles * sizeof (HANDLE *));
        handles = newhandles;
    }
    handles[nhandles++] = h = (HANDLE *) challoc(sizeof (HANDLE), 1);
    h->h_name = (char *)challoc(strlen(new_name) + 1, 0);
    strcpy(h->h_name, new_name);
    h->h_di = NULL;
    return (h);
}

/**
 * @brief Search |handles| by name.
 *
 * @param s_name  IN   name of handle to search for
 * @param which   IN   >>>
 * @param pret    OUT  >>>
 *
 */
static int
hsearch(const char *s_name, int which, HANDLE **pret)
{
    size_t i;
    HANDLE **ph;

    assert(which == 0 || which == 1);

    if (strcmp(s_name, lasthandle[which]->h_name) == 0) {
        *pret = lasthandle[which];
        return (1);
    }

    for (i = 0, ph = handles; i < nhandles; ++ph, ++i)
        if (strcmp(s_name, (*ph)->h_name) == 0) {
            lasthandle[which] = *pret = *ph;
            return (1);
        }

    lasthandle[which] = *pret = hadd(s_name);
    return (0);
}


/**
 * @brief Add a |DIRINFO| to |dirs| array; allocate more space if needed.
 *
 * @param v   IN  devid
 * @param d   IN  dirid
 * @return  pointer to new, initialized |DIRINFO|.
 *
 * Allocation failure is not an option.
 *
 */

static DIRINFO *
dadd(DEVID v, DIRID d)
{
    DIRINFO *di;
    DIRINFO **newdirs;

    if (ndirs == dirroom) {
        dirroom *= 2;
        newdirs = (DIRINFO **) mmv_alloc(dirroom * sizeof (DIRINFO *));
        memcpy(newdirs, dirs, ndirs * sizeof (DIRINFO *));
        chgive(dirs, ndirs * sizeof (DIRINFO *));
        dirs = newdirs;
    }
    dirs[ndirs++] = di = (DIRINFO *) challoc(sizeof (DIRINFO), 1);
    di->di_vid = v;
    di->di_did = d;
    di->di_nfils = 0;
    di->di_fils = NULL;
    di->di_flags = 0;
    return (di);
}


/**
 * @brief Search all |DIRINFO| struct for matching the given { devid, dirid }.
 *
 * @param v  IN  desired devid
 * @param d  IN  desired dirid
 * @return DIRINFO *, if found; NULL if not found.
 *
 */

static DIRINFO *
dsearch(DEVID v, DIRID d)
{
    size_t i;
    DIRINFO *di;

    for (i = 0, di = *dirs; i < ndirs; ++di, ++i) {
        if (v == di->di_vid && d == di->di_did) {
            return (di);
        }
    }
    return (NULL);
}

/**
 * @brief checkdir
 *
 * @param p        ???  ???
 * @param pathend  ???  ???
 * @param which    ???  ???
 * @return  HANDLE * if success, NULL if check failed
 *
 */

HANDLE *
checkdir(const char *p, char *pathend, int which)
{
    struct stat dstat;
    DIRID d;
    DEVID v;
    DIRINFO *di;
    const char *myp;
    char *lastslash;
    int sticky;
    HANDLE *h;

    lastslash = NULL;
    if (hsearch(p, which, &h)) {
        if (h->h_di == NULL) {
            direrr = h->h_err;
            return (NULL);
        }
        else {
            return (h);
        }
    }

    if (*p == '\0') {
        myp = dir_self;
    }
    else if (pathend == p + 1) {
        myp = SLASHSTR;
    }
    else {
        lastslash = pathend - 1;
        *lastslash = '\0';
        myp = p;
    }

    if (stat(myp, &dstat) || !S_ISDIR(dstat.st_mode)) {
        direrr = h->h_err = H_NODIR;
    }
    else if (access(myp, R_OK | X_OK)) {
        direrr = h->h_err = H_NOREADDIR;
    }
    else {
        direrr = 0;
        sticky = (dstat.st_mode & S_ISVTX) && uid != 0 && uid != dstat.st_uid ? FI_INSTICKY : 0;
        v = dstat.st_dev;
        d = dstat.st_ino;

        if ((di = dsearch(v, d)) == NULL) {
            takedir(myp, di = dadd(v, d), sticky);
        }
    }

    if (lastslash != NULL) {
        *lastslash = SLASH;
    }
    if (direrr != 0) {
        return (NULL);
    }
    h->h_di = di;
    return (h);
}

/* end of Un*x checkdir, takedir; back to general program */

/**
 * @brief checkto()  ???
 *
 * @param mmv
 * @param hfrom  ???  ???
 * @param f      ???  ???
 * @param phto   ???  ???
 * @param pnto   ???  ???
 * @param pfdel  ???  ???
 * @return -1/0 status
 *
 */

static int
checkto(mmv_t *mmv, HANDLE *hfrom, char *f, HANDLE **phto, char **pnto, FILEINFO **pfdel)
{
    char tpath[PATH_MAX + 1];
    char *pathend;
    FILEINFO *fdel;
    int hlen, tlen;

    fdel = NULL;  // XXX Compare to original mmv code.
    if (mmv->op & DIRMOVE) {
        *phto = hfrom;
        hlen = strlen(hfrom->h_name);
        pathend = mmv->fullrep + hlen;
        memmove(pathend, mmv->fullrep, strlen(mmv->fullrep) + 1);
        memmove(mmv->fullrep, hfrom->h_name, hlen);
        if ((fdel = *pfdel = fsearch(pathend, hfrom->h_di)) != NULL) {
            *pnto = fdel->fi_name;
            getstat(mmv->fullrep, fdel);
        }
        else {
            *pnto = mydup(pathend);
        }
    }
    else {
        pathend = getpath(mmv, tpath);
        hlen = pathend - mmv->fullrep;
        *phto = checkdir(tpath, tpath + hlen, 1);
        if (*phto != NULL &&
            *pathend != '\0' &&
            (fdel = *pfdel = fsearch(pathend, (*phto)->h_di)) != NULL &&
            (getstat(mmv->fullrep, fdel), fdel->fi_stflags & FI_ISDIR)) {
            tlen = strlen(pathend);
            strcpy(pathend + tlen, SLASHSTR);
            ++tlen;
            strcpy(tpath + hlen, pathend);
            pathend += tlen;
            hlen += tlen;
            *phto = checkdir(tpath, tpath + hlen, 1);
        }

        if (*pathend == '\0') {
            *pnto = f;
            if (pathend - mmv->fullrep + strlen(f) >= PATH_MAX) {
                strcpy(mmv->fullrep, TOOLONG);
                return (-1);
            }
            strcat(pathend, f);
            if (*phto != NULL) {
                fdel = *pfdel = fsearch(f, (*phto)->h_di);
                if (fdel != NULL) {
                    getstat(mmv->fullrep, fdel);
                }
            }
        }
        else if (fdel != NULL) {
            *pnto = fdel->fi_name;
        }
        else {
            *pnto = mydup(pathend);
        }
    }

    return (0);
}

/**
 * @brief  Names that, for one reason or another, are not valid for moves.
 *
 * @param s  IN  A simple filename in question
 * @return 0/1 status
 *
 */

static int
badname(const char *s)
{
    return ((*s == '.' && (s[1] == '\0' || strcmp(s, "..") == 0)) || strlen(s) > name_max);
}


/**
 * @brief Determine if a directory is writable and keep a record of it.
 *
 * @param h  IN  Directory |HANDLE| in question
 * @return 0/1 status
 *
 */

unsigned int
dwritable(HANDLE *h)
{
    char *p;
    char *myp;
    char *lastslash;
    char *pathend;
    unsigned int r;
    unsigned int *pw = &(h->h_di->di_flags);

    p = h->h_name;
    lastslash = NULL;
    if (uid == 0) {
        return (1);
    }

    if (*pw & DI_KNOWWRITE) {
        return (*pw & DI_CANWRITE);
    }

    pathend = p + strlen(p);
    if (*p == '\0') {
        myp = dir_self;
    }
    else if (pathend == p + 1) {
        myp = SLASHSTR;
    }
    else {
        lastslash = pathend - 1;
        *lastslash = '\0';
        myp = p;
    }
    r = !access(myp, W_OK) ? DI_CANWRITE : 0;
    *pw |= DI_KNOWWRITE | r;

    if (lastslash != NULL) {
        *lastslash = SLASH;
    }
    return (r);
}

/**
 * @brief Report any errors encountered while trying a move operation.
 *
 * @param mmv
 * @param hfrom   ???  ???
 * @param ffrom   ???  ???
 * @param phto    ???  ???
 * @param pnto    ???  ???
 * @param pfdel   ???  ???
 * @param pflags  IN   ???
 *
 */

int
badrep(mmv_t *mmv, HANDLE *hfrom, FILEINFO *ffrom, HANDLE **phto, char **pnto, FILEINFO **pfdel, int *pflags)
{
    char *f = ffrom->fi_name;

    *pflags = 0;
    if ((ffrom->fi_stflags & FI_ISDIR) && !(mmv->op & (DIRMOVE | SYMLINK))) {
        printf("%s -> %s : source file is a directory.\n",
            mmv->pathbuf, mmv->fullrep);
    }
    else if ((mmv->op & (COPY | APPEND)) && access(mmv->pathbuf, R_OK)) {
        printf("%s -> %s : no read permission for source file.\n",
            mmv->pathbuf, mmv->fullrep);
    }
    else if (*f == '.' && (f[1] == '\0' || strcmp(f, "..") == 0) && !(mmv->op & SYMLINK)) {
        printf("%s -> %s : . and .. can't be renamed.\n",
            mmv->pathbuf, mmv->fullrep);
    }
    else if (repbad || checkto(mmv, hfrom, f, phto, pnto, pfdel) || badname(*pnto)) {
        printf("%s -> %s : bad new name.\n",
            mmv->pathbuf, mmv->fullrep);
    }
    else if (*phto == NULL) {
        const char *problem;

        problem = (direrr == H_NOREADDIR)
            ? "no read or search permission for target directory"
            : "target directory does not exist";
        printf("%s -> %s : %s.\n", mmv->pathbuf, mmv->fullrep, problem);
    }
    else if (!dwritable(*phto)) {
        printf("%s -> %s : no write permission for target directory.\n",
            mmv->pathbuf, mmv->fullrep);
    }
    else if ((*phto)->h_di->di_vid != hfrom->h_di->di_vid && (*pflags = R_ISX, (mmv->op & (NORMMOVE | HARDLINK)))) {
        printf("%s -> %s : cross-device move.\n",
            mmv->pathbuf, mmv->fullrep);
    }
    else if (*pflags && (mmv->op & MOVE) && !(ffrom->fi_stflags & FI_ISLNK) && access(mmv->pathbuf, R_OK)) {
        printf("%s -> %s : no read permission for source file.\n",
            mmv->pathbuf, mmv->fullrep);
    }
    else {
        return (0);
    }
    ++mmv->badreps;
    return (-1);
}

/**
 * @brief ???
 *
 */

int
ffirst(char *s, int n, DIRINFO *d)
{
    int first, k, last, res;
    FILEINFO **fils = d->di_fils;
    int nfils = d->di_nfils;

    if (nfils == 0 || n == 0) {
        return (0);
    }
    first = 0;
    last = nfils - 1;
    for (;;) {
        k = (first + last) >> 1;
        res = strncmp(s, fils[k]->fi_name, n);
        if (first == last) {
            return (res == 0 ? k : nfils);
        }
        else if (res > 0) {
            first = k + 1;
        }
        else {
            last = k;
        }
    }
}
