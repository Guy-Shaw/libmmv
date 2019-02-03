/*
 * Filename: src/cmd/mmv-pairs/mmv-pairs.c
 * Project: libmmv
 * Brief: Like mmv, but just takes in raw from->to filenames, not patterns.
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

#include <ctype.h>
    // Import isprint()
#include <errno.h>
    // Import var EINVAL
#include <stdbool.h>
    // Import type bool
    // Import constant false
    // Import constant true
#include <stddef.h>
    // Import constant NULL
#include <stdio.h>
    // Import type FILE
    // Import fputc()
    // Import fputs()
    // Import snprintf()
    // Import var stdin
    // Import var stdout
#include <stdlib.h>
    // Import exit()
#include <string.h>
    // Import strcmp()
#include <unistd.h>
    // Import getopt_long()
    // Import type size_t

#include <dbgprint.h>
#include <cscript.h>
#include <mmv.h>

const char *program_path;
const char *program_name;

size_t filec;               // Count of elements in filev
char **filev;               // Non-option elements of argv

FILE *errprint_fh;
FILE *dbgprint_fh;

bool verbose = false;
bool debug   = false;
bool pairs_from_argv = false;

const char *encoding_opt = NULL;

static struct option long_options[] = {
    {"help",           no_argument,       0,  'h'},
    {"version",        no_argument,       0,  'V'},
    {"verbose",        no_argument,       0,  'v'},
    {"debug",          no_argument,       0,  'd'},
    {"argv",           no_argument,       0,  'A'},
    {"encoding",       required_argument, 0,  'E'},
    {0, 0, 0, 0}
};

static const char usage_text[] =
    "Options:\n"
    "  --help|-h|-?         Show this help message and exit\n"
    "  --version            Show version information and exit\n"
    "  --verbose|-v         verbose\n"
    "  --debug|-d           debug\n"
    "  --argv|-A            Filename pairs are on command line.\n"
    "                       Default is that pairs are read in from stdin.\n"
    "  --encoding=<E>       Filename pairs are encoded <E>\n"
    "      Encoding is one of: { null, qp, vis, xnn }.\n"
    "\n"
    ;

static const char version_text[] =
    "0.1\n"
    ;

static const char copyright_text[] =
    "Copyright (C) 2016-2017 Guy Shaw\n"
    "Written by Guy Shaw\n"
    ;

static const char license_text[] =
    "License GPLv3+: GNU GPL version 3 or later"
    " <http://gnu.org/licenses/gpl.html>.\n"
    "This is free software: you are free to change and redistribute it.\n"
    "There is NO WARRANTY, to the extent permitted by law.\n"
    ;

static void
fshow_program_version(FILE *f)
{
    fputs(version_text, f);
    fputc('\n', f);
    fputs(copyright_text, f);
    fputc('\n', f);
    fputs(license_text, f);
    fputc('\n', f);
}

static void
show_program_version(void)
{
    fshow_program_version(stdout);
}

static void
usage(void)
{
    eprintf("usage: %s [ <options> ]\n", program_name);
    eprint(usage_text);
}

static inline bool
is_long_option(const char *s)
{
    return (s[0] == '-' && s[1] == '-');
}

static inline char *
vischar_r(char *buf, size_t sz, int c)
{
    if (isprint(c)) {
        buf[0] = c;
        buf[1] = '\0';
    }
    else {
        snprintf(buf, sz, "\\x%02x", c);
    }
    return (buf);
}

static int
mmv_pairs_stdin(mmv_t *mmv, const char *encoding_opt)
{
    int err;

    if (strcmp(encoding_opt, "nul") == 0) {
        err = mmv_get_pairs_nul(mmv, stdin);
    }
    else if (strcmp(encoding_opt, "null") == 0) {
        err = mmv_get_pairs_nul(mmv, stdin);
    }
    else if (strcmp(encoding_opt, "qp") == 0) {
        err = mmv_get_pairs_qp(mmv, stdin);
    }
    else if (strcmp(encoding_opt, "vis") == 0) {
        err = mmv_get_pairs_vis(mmv, stdin);
    }
    else if (strcmp(encoding_opt, "xnn") == 0) {
        err = mmv_get_pairs_xnn(mmv, stdin);
    }
    else {
        eprintf("Unknown encoding, '%s'.\n", encoding_opt);
        err = EINVAL;
    }

    if (err) {
        return (err);
    }

    if (dbgprint_fh) {
        fputs("op=", dbgprint_fh);
        fdump_mmv_op(dbgprint_fh, mmv);
    }

    return (mmv_execute(mmv));
}

int
main(int argc, char **argv)
{
    extern char *optarg;
    extern int optind, opterr, optopt;
    int option_index;
    int err_count;
    int optc;
    int rv;

    mmv_t *mmv;

    set_eprint_fh();
    program_path = *argv;
    program_name = sname(program_path);
    option_index = 0;
    err_count = 0;
    opterr = 0;

    while (true) {
        int this_option_optind;

        if (err_count > 10) {
            eprintf("%s: Too many option errors.\n", program_name);
            break;
        }

        this_option_optind = optind ? optind : 1;
        optc = getopt_long(argc, argv, "+hVdvAE:", long_options, &option_index);
        if (optc == -1) {
            break;
        }

        rv = 0;
        if (optc == '?' && optopt == '?') {
            optc = 'h';
        }

        switch (optc) {
        case 'V':
            show_program_version();
            exit(0);
            break;
        case 'h':
            fputs(usage_text, stdout);
            exit(0);
            break;
        case 'd':
            debug = true;
            set_debug_fh(NULL);
            break;
        case 'v':
            verbose = true;
            break;
        case 'A':
            pairs_from_argv = true;
            break;
        case 'E':
            // XXX complain if more than 1 --encoding
            encoding_opt = optarg;
            break;
        case '?':
            eprint(program_name);
            eprint(": ");
            if (is_long_option(argv[this_option_optind])) {
                eprintf("unknown long option, '%s'\n",
                    argv[this_option_optind]);
            }
            else {
                char chrbuf[10];
                eprintf("unknown short option, '%s'\n",
                    vischar_r(chrbuf, sizeof (chrbuf), optopt));
            }
            ++err_count;
            break;
        default:
            eprintf("%s: INTERNAL ERROR: unknown option, '%c'\n",
                program_name, optopt);
            exit(2);
            break;
        }
    }

    verbose = verbose || debug;

    if (optind < argc) {
        filec = (size_t) (argc - optind);
        filev = argv + optind;
    }
    else {
        filec = 0;
        filev = NULL;
    }

    if (verbose) {
        fshow_str_array(errprint_fh, filec, filev);
    }

    if (verbose && optind < argc) {
        eprintf("non-option ARGV-elements:\n");
        while (optind < argc) {
            eprintf("    %s\n", argv[optind]);
            ++optind;
        }
    }

    if (err_count != 0) {
        usage();
        exit(1);
    }

    set_debug_fh(NULL);
    mmv = mmv_new();
    mmv_set_default_options(mmv);
    mmv_setopt(mmv, 'x');

    if (pairs_from_argv) {
        if (encoding_opt != NULL) {
            eprintf("Options conflict: --argv and --encoding='%s'.\n", encoding_opt);
            eprintf("Options --argv implies no encoding.\n");
            exit(2);
        }
        rv = mmv_add_fname_pairs(mmv, filec, filev);
        if (rv) {
            exit(rv);
        }
        rv = mmv_compile(mmv);
        if (rv) {
            exit(rv);
        }
        rv = mmv_execute(mmv);
    }
    else {
        rv = mmv_pairs_stdin(mmv, encoding_opt);
    }

    exit(rv);
}
