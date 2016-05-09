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

static inline char *
mydup(char *s)
{
    return (strcpy((char *)challoc(strlen(s) + 1, 0), (s)));
}

static char MOVENAME[]   = "mmv";
static char COPYNAME[]   = "mcp";
static char APPENDNAME[] = "mad";
static char LINKNAME[]   = "mln";

static size_t name_max = 0;
static size_t maxwild = 20;

/*
 * Declare external functions.
 */

extern void mmv_set_default_options(mmv_t *mmv);
extern int mmv_setopt(mmv_t *mmv, int opt);
extern int getpat(mmv_t *mmv);
extern void quit(void);

static char USAGE[] =
    "Usage: %s [-m|x|r|c|o|a|l] [-h] [-d|p] [-g|t] [-v|n] [from to]\n"
    "\n"
    "Use =[l|u]N in the ``to'' pattern to get the [lowercase|uppercase of the]\n"
    "string matched by the N'th ``from'' pattern wildcard.\n"
    "\n"
    "A ``from'' pattern containing wildcards should be quoted when given\n"
    "on the command line.\n";

static char TOOLONG[] = "(too long)";
static DIRID cwdd = (DIRID)-1;
static DEVID cwdv = (DIRID)-1;

static char EMPTY[] = "(empty)";
static char SLASHSTR[] = {SLASH, '\0'};
static int direrr;
static DIRINFO **dirs;
static size_t ndirs = 0;
static size_t dirroom;
static uid_t uid, euid;

static char **stagel, **firstwild, **stager, **start;
static int *len;
static int *nwilds;

static int nstages;
static HANDLE **handles;
static size_t nhandles = 0;
static size_t handleroom;
static char badhandle_name[] = "\200";
static HANDLE badhandle = {badhandle_name, NULL, 0};
static HANDLE *(lasthandle[2]) = {&badhandle, &badhandle};

static int repbad;

static char dir_self[] = ".";
static char home_empty[] = "";
static char *home;
static size_t homelen;

static void
init_backrefs(void)
{
    firstwild = (char **)myalloc(maxwild * sizeof (char *));
    stagel    = (char **)myalloc(maxwild * sizeof (char *));
    stager    = (char **)myalloc(maxwild * sizeof (char *));
    start     = (char **)myalloc(maxwild * sizeof (char *));
    len       = (int *)myalloc(maxwild * sizeof (int));
    nwilds    = (int *)myalloc(maxwild * sizeof (int));
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
init_patgen_static_data(void)
{
    struct stat dstat;

    if ((home = getenv("HOME")) == NULL || strcmp(home, SLASHSTR) == 0) {
        home = home_empty;
    }
    homelen = strlen(home);

    // XXX Use patchconf(_, _PC_NAME_MAX).
    // XXX but this can be different for different filesystems.
    // XXX So, it should be computed and cached for each filesystem.
    //
    name_max = 255;

    if (stat(".", &dstat) == 0) {
        cwdd = dstat.st_ino;
        cwdv = dstat.st_dev;
    }
    euid = geteuid();
    uid = getuid();
    signal(SIGINT, breakout);

    dirroom = handleroom = INITROOM;
    dirs = (DIRINFO **) myalloc(dirroom * sizeof (DIRINFO *));
    handles = (HANDLE **) myalloc(handleroom * sizeof (HANDLE *));
    ndirs = nhandles = 0;
    init_backrefs();
}

/**
 * @brief Parse mmv-classic arguments
 *
 * @param  argc      IN   Together with argv ...
 * @param  argv      IN   the main() program command-line arguments
 * @param  pfrompat  OUT  Set to the 'from' pattern, if any
 * @param  ptopat    OUT  Set to the 'to'   pattern, if any
 * @return errno-style -- 0 == success, non-zero is some type of error
 *
 */

static int
procargs(mmv_t *mmv, int argc, char *const *argv, char **pfrompat, char **ptopat)
{
    char *p;
    char *cmdname = argv[0];

    mmv_set_default_options(mmv);

    // XXX Use GNU getopt() or getopt_long()

    for (--argc, ++argv; argc > 0 && **argv == '-'; --argc, ++argv) {
        for (p = *argv + 1; *p != '\0'; ++p) {
            int err;
            int c;

            // XXX c = mylower(*p);
            c = *p;
            err = mmv_setopt(mmv, c);
            if (err) {
                return ((EINVAL << 8) + c);
            }
        }
    }

    if (mmv->op == DFLT) {
        if (strcmp(cmdname, MOVENAME) == 0)
            mmv->op = XMOVE;
        else if (strcmp(cmdname, COPYNAME) == 0)
            mmv->op = NORMCOPY;
        else if (strcmp(cmdname, APPENDNAME) == 0)
            mmv->op = NORMAPPEND;
        else if (strcmp(cmdname, LINKNAME) == 0)
            mmv->op = HARDLINK;
        else
            mmv->op = DFLTOP;
    }

    if (euid != uid && !(mmv->op & DIRMOVE)) {
        int rv;
        gid_t gid;
        int err;

        rv = setuid(uid);
        if (rv) {
            err = errno;
            fprintf(stderr, "setuid(%d) failed.\n", uid);
            explain_err(err);
            return (err);
        }
        gid = getgid();
        rv = setgid(gid);
        if (rv) {
            err = errno;
            fprintf(stderr, "setgid(%d) failed.\n", gid);
            explain_err(err);
            return (err);
        }
    }

    if (mmv->badstyle != ASKBAD && mmv->delstyle == ASKDEL) {
        mmv->delstyle = NODEL;
    }

    if (argc == 0) {
        *pfrompat = NULL;
        *ptopat   = NULL;
    }
    else if (argc == 2) {
        *pfrompat = argv[0];
        *ptopat   = argv[1];
    }
    else {
        return (EINVAL);
    }

    return (0);
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

static bool
check_encoding(enum encode e)
{
    switch(e) {
    case ENCODE_NONE:
    case ENCODE_PAT:
    case ENCODE_QP:
    case ENCODE_NUL:
        return (true);
    default:
        eprintf("Invalid encoding, '%u'.\n", (unsigned int)e);
        return (false);
    }
}

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

// XXX Move to parser object
//
static size_t totwilds;

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
 * @brief Parse 'from' pattern; record substrings for back-references, etc.
 *
 * @param mmv
 * @return (-1/0) style status
 *
 */

static int
parse_from_pattern(mmv_t *mmv)
{
    char *p, *lastname;
    int instage;
    int cclass, c;
    sbuf_t m_fname;
    cstr_t m_home;
    char *fn;
    int rv;

    assert(check_encoding(mmv->encoding));

    // XXX Use new initializer or assignment notation to set all fields
    // XXX in one statement.
    // XXX OR plan B: use a macro to set all fields of |sbuf_t| and |cstr_t|.
    fn = mmv->from;
    m_fname.s    = fn;
    m_fname.size = MAXPATLEN;
    m_fname.len  = mmv->fromlen;
    m_home.s = home;
    m_home.len = homelen;

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

    totwilds = nstages = instage = 0;
    for (p = lastname; (c = *p) != '\0'; ++p) {
        cclass = (mmv->encoding == ENCODE_PAT) ? c : 'A';
        if (c == SLASH) {
            cclass = c;
        }
        switch (cclass) {
        case SLASH:
            lastname = p + 1;
            if (instage) {
                if (firstwild[nstages] == NULL) {
                    firstwild[nstages] = p;
                }
                stager[nstages++] = p;
                instage = 0;
            }
            break;
        case ';':
            if (lastname != p) {
                printf("%s -> %s : badly placed ;.\n",
                    mmv->from, mmv->to);
                return (-1);
            }
        case '!':
        case '*':
        case '?':
        case '[':
            if (totwilds++ == maxwild) {
                printf("%s -> %s : too many wildcards.\n",
                    mmv->from, mmv->to);
                return (-1);
            }
            if (instage) {
                nwilds[nstages]++;
                if (firstwild[nstages] == NULL) {
                    firstwild[nstages] = p;
                }
            }
            else {
                stagel[nstages] = lastname;
                firstwild[nstages] = (c == ';' ? NULL : p);
                nwilds[nstages] = 1;
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
        if (firstwild[nstages] == NULL) {
            firstwild[nstages] = p;
        }
        stager[nstages++] = p;
    }
    else {
        stagel[nstages] = lastname;
        nwilds[nstages] = 0;
        firstwild[nstages] = p;
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

static int
parse_to_pattern(mmv_t *mmv)
{
    char *p, *lastname;
    int c;

    assert(check_encoding(mmv->encoding));

    /*
     * Scan 'to' path -- expand any leading ~/
     */
    lastname = mmv->to;
    if (mmv->to[0] == '~' && mmv->to[1] == SLASH) {
        if (homelen + mmv->tolen > MAXPATLEN) {
            fexplain_char_pattern_too_long(stderr, mmv->to, MAXPATLEN);
            return (-1);
        }
        memmove(mmv->to + homelen, mmv->to + 1, mmv->tolen);
        memmove(mmv->to, home, homelen);
        lastname += homelen + 1;
    }

    /*
     * Scan 'to' path -- record the positions of any backreferences
     * (e.g. '#1') in the replacement pattern.
     */
    for (p = lastname; (c = *p) != '\0'; ++p) {
        int cclass;
        size_t backref_nr;

        // SLASH is special even for encodings other than ENCODE_PAT.
        cclass = (mmv->encoding == ENCODE_PAT || c == SLASH) ? c : 'A';
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
        case BACKREF:
            c = *(++p);
            if (c == 'l' || c == 'u') {
                c = *(++p);
            }
            if (!isdigit(c)) {
                printf("%s -> %s : expected digit (not '%c') after '%c'.\n",
                    mmv->from, mmv->to, c, cclass);
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
            if (backref_nr < 1 || backref_nr > totwilds) {
                printf("%s -> %s : wildcard %zu does not exist.\n",
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
 * @brief Does a file name match a pattern?
 *
 * @param mmv
 * @param ffrom  IN  'from' filename to match
 * @param pat    IN  pattern that |ffrom| is to matched against
 * @return (-1/0) style status
 *
 */

static int
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
 * @param dirs       ???  ???
 * @param fils       ???  ???
 * @return 0/1 status
 *
 *
 */

static int
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
    di->di_fils = fils = (FILEINFO **) myalloc(room * sizeof (FILEINFO *));
    cnt = 0;
    while ((dp = readdir(dirp)) != NULL) {
        if (cnt == room) {
            room *= 2;
            fils = (FILEINFO **) myalloc(room * sizeof (FILEINFO *));
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
        newhandles = (HANDLE **) myalloc(handleroom * sizeof (HANDLE *));
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
        newdirs = (DIRINFO **) myalloc(dirroom * sizeof (DIRINFO *));
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

static HANDLE *
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

static unsigned int
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

static int
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

static int
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


/**
 * @brief Do glob-style pattern match of a given sting and pattern
 *
 * @param pat     IN   pattern to match
 * @param s       IN   string to match agains |pattern|
 * @param start1  OUT  Wildcard start positions
 * @param len1    OUT  Wildcard lengths
 * @return 0/1 status: 1 = match, 0 = not match
 *
 */

static int
match(char *pat, char *s, char **start1, int *len1)
{
    char c;

    *start1 = 0;
    for (;;) {
        switch (c = *pat) {
        case '\0':
        case SLASH:
            return (*s == '\0');
        case '*':
            *start1 = s;
            if ((c = *(++pat)) == '\0') {
                *len1 = strlen(s);
                return (1);
            }
            else {
                for (*len1 = 0; !match(pat, s, start1 + 1, len1 + 1); (*len1)++, ++s) {
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
            *(start1++) = s;
            *(len1++) = 1;
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
                *(start1++) = s;
                *(len1++) = 1;
                ++pat;
                ++s;
            }
            break;
        case ESC:
            c = *(++pat);
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
 * @brief Do patten expansion for one source file and one 'to' pattern.
 *
 * @param mmv
 *
 * At this stage, |mmv| already contains a source filename
 * and a replacement pattern.  Do pattern expansion on this pair.
 *
 * The 'from' pattern must have already been parsed, because
 * if there are any back-references in the 'to' pattern, all information
 * about them is in static data: start[], len[], nwilds, etc.
 *
 */

static void
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
            --backref_nr;
            if (l + len[backref_nr] >= PATH_MAX) {
                goto toolong;
            }

            switch (cnv) {
            case '=':
                memmove(p, start[backref_nr], len[backref_nr]);
                break;
            case 'l':
                memmove_lc(p, start[backref_nr], len[backref_nr]);
                break;
            case 'u':
                memmove_uc(p, start[backref_nr], len[backref_nr]);
                break;
            }
            p += len[backref_nr];
            l += len[backref_nr];
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
 * @brief  dostage ???
 *
 * @param mmv
 * @param lastend  ???  ???
 * @param pathend  ???  ???
 * @param start1   ???  ???
 * @param len1     ???  ???
 * @param stage    ???  ???
 * @param anylev   IN   ???
 *
 */

static int
dostage(mmv_t *mmv, char *lastend, char *pathend, char **start1, int *len1, int stage, int anylev)
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

    if (dbgprint_fh) {
        const char *maybe_start1 = (start1 != NULL) ? *start1 : "NULL";
        fprintf(dbgprint_fh, "%s:\n", __FUNCTION__);
        fprintf(dbgprint_fh, "  lastend='%s'\n", lastend);
        fprintf(dbgprint_fh, "  pathend='%s'\n", pathend);
        fprintf(dbgprint_fh, "  *start1='%s'\n", maybe_start1);
        fprintf(dbgprint_fh, "  *len1=%d\n", *len1);
        fprintf(dbgprint_fh, "  stage=%d\n", stage);
        fprintf(dbgprint_fh, "  anylev=%d\n", anylev);
    }

    ret = 1;
    laststage = (stage + 1 == nstages);
    wantdirs = !laststage || (mmv->op & (DIRMOVE | SYMLINK)) || (nwilds[nstages - 1] == 0);

    if (!anylev) {
        prelen = stagel[stage] - lastend;
        if (pathend - mmv->pathbuf + prelen >= PATH_MAX) {
            printf("%s -> %s : search path after %s too long.\n",
                mmv->from, mmv->to, mmv->pathbuf);
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

    if (*lastend == ';') {
        anylev = 1;
        *start1 = pathend;
        *len1 = 0;
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
    if (firstesc == NULL || firstesc > firstwild[stage]) {
        firstesc = firstwild[stage];
    }
    litlen = firstesc - lastend;
    pf = di->di_fils + (i = ffirst(lastend, litlen, di));
    if (i < nfils) {
        do {
            if ((match_rv = trymatch(mmv, *pf, lastend)) != 0 && (match_rv == 1 || match(lastend + litlen, (*pf)->fi_name + litlen, start1 + anylev, len1 + anylev)) && keepmatch(mmv, *pf, pathend, &k, 0, wantdirs, laststage)) {
                if (!laststage) {
                    ret &= dostage(mmv, stager[stage], pathend + k,
                                   start1 + nwilds[stage], len1 + nwilds[stage], stage + 1, 0);
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
                *len1 = pathend - *start1 + k;
                ret &= dostage(mmv, lastend, pathend + k, start1, len1, stage, 1);
            }
        }
    }

    return (ret);
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
    if (parse_from_pattern(mmv)) {
        mmv->paterr = 1;
        return;
    }

    if (parse_to_pattern(mmv)) {
        mmv->paterr = 1;
        return;
    }

    if (dostage(mmv, mmv->from, mmv->pathbuf, start, len, 0, 0)) {
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
mmv_add_pair(mmv_t *mmv, const char *src_fname, const char *dst_fname)
{
    int err;

    // XXX Make idempotent so that there can be repeated calls to mmv_add_pair()

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
