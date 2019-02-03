/*
 * Filename: src/libcscript/ask-common.c
 * Library: libcscript
 * Brief: Low-level support common to ask-* family: ask-string, ask-yesno, ...
 *
 * Copyright (C) 2016, 2017 Guy Shaw
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
 * Original mmv code by Vladimir Lanin. et al.
 *
 */

#if defined(__GNUC__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE 1
#endif

#include <errno.h>
    // Import var errno
#include <fcntl.h>
    // Import var O_RDONLY
    // Import var O_WRONLY
#include <stddef.h>
    // Import constant NULL
#include <stdio.h>
    // Import type FILE
    // Import fdopen()
    // Import fileno()
#include <stdlib.h>
    // Import abort()
#include <sys/types.h>
#include <sys/stat.h>
    // Import open()
#include <unistd.h>
    // Import fcntl()
    // Import isatty()

#include <eprint.h>

FILE *ttyw_fh = NULL;   // Used to write prompt
FILE *ttyr_fh = NULL;   // Used to read answer

/*
 * Since we are dealing with global data, we can capture any information
 * about errors related to ttyw_fh and ttyr_fh, and store it in a known
 * place, rather than propagating status up a chain of function calls,
 * or something else.
 */

int ttyw_err = 0;
int ttyr_err = 0;

/*
 * We want to make sure that we have suitable file handles for
 * reading and writing to the terminal, if possible.
 *
 * It is reasonably likely that the file handle associated with
 * stderr is a terminal and is open for writing.  But stderr could
 * be redirected.
 *
 * Also, it is reasonably likely that the file handle associated with
 * stdin is a terminal and is open for reading.  But stdin could
 * be the source of filename pairs or pattern pairs.
 *
 * So, you cannot really count on any of that.
 *
 * We do not want to open up new file descriptors unnecessarily.
 * So, the strategy is to do a quick check if we have open file
 * descriptors in the usual places that are connected to a terminal,
 * use them if they are available, but open up out own connections
 * to /dev/tty, if needed.
 *
 * There are two functions for getting a possibly new connection
 * to /dev/tty, one for reading and one for writing.
 *
 * They each try the given stdio file handle, first,
 * but open a new stdio file handle, if needed.
 *
 * If a new connection to /dev/tty is opened, then
 * we make sure it is with a file descriptor >= 3, so that
 * there will not be any interference with other code
 * that counted on being able to use or modify file descriptors
 * for stdin, stout, and stderr.
 *
 */


/*
 * Get a connection to a terminal, suitable for writing.
 * Try the given stdio file handle, first.
 * Open our own stdio file handle, if needed.
 *
 * If we do open our own connection to /dev/tty, then
 * make sure it is with a file descriptor >= 3, so that
 * there will not be any interference with other code
 * that countd on being able to use or modify file descriptors
 * for stdin, stout, and stderr.
 *
 */

FILE *
fopen_ttyw_fh(FILE *df)
{
    int ttyw_fd;

    ttyw_err = 0;
    if (ttyw_fh != NULL) {
        return (ttyw_fh);
    }

    if (isatty(fileno(df))) {
        ttyw_fh = df;
        return (ttyw_fh);
    }

    ttyw_fd = open("/dev/tty", O_WRONLY, 0600);
    if (ttyw_fd >= 3) {
        ttyw_fh = fdopen(ttyw_fd, "w");
        if (ttyw_fh == NULL) {
            ttyw_err = errno;
        }
        return (ttyw_fh);
    }

    if (ttyw_fd == -1) {
        ttyw_err = errno;
        eprintf("Feh!\n");
    }

    if (ttyw_fd >= 0 && ttyw_fd < 3) {
        ttyw_fd = fcntl(ttyw_fd, F_DUPFD, 3);
        if (ttyw_fd == -1) {
            ttyw_err = errno;
            eprintf("Feh!\n");
        }
        else {
            eprintf("Impossible! -- Feh!\n");
            abort();
        }
        ttyw_fh = fdopen(ttyw_fd, "w");
        if (ttyw_fh == NULL) {
            ttyw_err = errno;
        }
    }
    return (ttyw_fh);
}

FILE *
fopen_ttyr_fh(FILE *df)
{
    int ttyr_fd;

    ttyr_err = 0;
    if (ttyr_fh != NULL) {
        return (ttyr_fh);
    }

    if (isatty(fileno(df))) {
        ttyr_fh = df;
        return (ttyr_fh);
    }

    ttyr_fd = open("/dev/tty", O_RDONLY, 0600);
    if (ttyr_fd >= 3) {
        ttyr_fh = fdopen(ttyr_fd, "r");
        if (ttyr_fh == NULL) {
            ttyr_err = errno;
        }
        return (ttyr_fh);
    }

    if (ttyr_fd == -1) {
        ttyr_err = errno;
        eprintf("Feh!\n");
    }

    if (ttyr_fd >= 0 && ttyr_fd < 3) {
        ttyr_fd = fcntl(ttyr_fd, F_DUPFD, 3);
        if (ttyr_fd == -1) {
            ttyr_err = errno;
            eprintf("Feh!\n");
        }
        else {
            eprintf("Impossible! -- Feh!\n");
            abort();
        }
        ttyr_fh = fdopen(ttyr_fd, "r");
        if (ttyr_fh == NULL) {
            ttyr_err = errno;
        }
    }
    return (ttyr_fh);
}

