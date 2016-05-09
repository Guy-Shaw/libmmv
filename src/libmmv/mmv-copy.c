/*
 * Filename: src/libmmv/mmv-copy.c
 * Library: libmmv
 * Brief: Prompt, then ask for a filename, then open that file
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <utime.h>
#include <errno.h>
#include <string.h>

#include <cscript.h>

#define IMPORT_OPS
#define IMPORT_REP
#define IMPORT_FILEINFO
#include <mmv-impl.h>
#include <mmv-state.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

// XXX Get buffer size from preferred block size of the filesystem(s) involved.
// XXX Also, get bufsize from config file.
// XXX Allow bufsize to be set low, for testing.

static size_t bufsize = 64 * 1024;

#define IRWMASK (S_IRUSR | S_IWUSR)
#define RWMASK (IRWMASK | (IRWMASK >> 3) | (IRWMASK >> 6))

struct file_copy {
    const char *src_fname;
    const char *dst_fname;
    int dst_o_flags;
    mode_t dst_o_perm;
    int src_fd;
    int dst_fd;
    int op;
    char *buf;
    size_t bufsize;
    int src_err;
    int dst_err;
    size_t fsize;
};

typedef struct file_copy file_copy_t;

/*
 * @brief Copy atime and mtime from src to dst
 * @param cpy  INOUT  A |file_copy_t| containing source and destination info
 *
 */

int
copy_ftimes(file_copy_t *cpy)
{
    struct stat statb;
    int rv;

    rv = stat(cpy->src_fname, &statb);
    if (rv) {
        eprintf("stat('%s') failed.\n", cpy->src_fname);
        cpy->src_err = errno;
        return (rv);
    }

    rv = utimensat(AT_FDCWD, cpy->dst_fname, &statb.st_atim, 0);
    if (rv) {
        cpy->dst_err = errno;
        eprintf("utimensat('%s') failed.\n", cpy->dst_fname);
        return (rv);
    }

    return (0);
}

/*
 * @brief Copy (or append) one file to another.  fds are already open.
 * @param cpy  INOUT  A |file_copy_t| containing source and destination info
 *
 */
int
file_copy_fds(file_copy_t *cpy)
{
    size_t len;

    len = cpy->fsize;
    if (cpy->op & APPEND) {
        lseek(cpy->dst_fd, 0, 2);
    }

    while (len != 0) {
        size_t rsize;
        ssize_t rlen;
        ssize_t wlen;

        if (len == SIZE_UNLIMITED || len > cpy->bufsize) {
            rsize = cpy->bufsize;
        }
        else {
            rsize = len;
        }
        rlen = read(cpy->src_fd, cpy->buf, rsize);
        if (rlen < 0) {
            cpy->src_err = errno;
            return (-1);
        }
        if (rlen == 0) {
            break;
        }
        wlen = write(cpy->dst_fd, cpy->buf, rlen);
        if (wlen < 0) {
            cpy->dst_err = errno;
            return (-1);
        }
        if (wlen != rlen) {
            cpy->dst_err = EIO;
            return (-1);
        }
        if (rlen == 0) {
            break;
        }
        len -= rlen;
    }

    return (0);
}

/*
 * @brief Copy one file to another
 *
 * 1) open source and destination files
 * 2) allocate the transfer I/O buffer
 * 3) call file_copy_fds() to do the copy on open file descriptors
 * 4) free the buffer
 * 5) close both files
 * 6) possible copy atime and mtime
 */
int
file_copy(file_copy_t *cpy)
{
    int rv_copy;
    int rv_close;
    int rv;

    cpy->src_fd = open(cpy->src_fname, O_RDONLY | O_BINARY, 0);
    if (cpy->src_fd < 0) {
        cpy->src_err = errno;
        return (-1);
    }

    cpy->dst_fd = open(cpy->dst_fname, cpy->dst_o_flags, cpy->dst_o_perm);
    if (cpy->dst_fd < 0) {
        cpy->dst_err = errno;
        close(cpy->src_fd);
        return (-1);
    }

    cpy->buf = (char *)guard_malloc(cpy->bufsize);
    rv_copy = file_copy_fds(cpy);
    free(cpy->buf);
    cpy->buf = NULL;

    if (cpy->dst_fd >= 0) {
        rv_close = close(cpy->dst_fd);
        if (rv_close) {
            cpy->dst_err = errno;
            eprintf("close(%s) failed.\n", cpy->dst_fname);
            // XXX explain err
        }
        else {
            cpy->dst_fd = -1;
        }
    }

    if (cpy->src_fd >= 0) {
        rv_close = close(cpy->src_fd);
        if (rv_close) {
            eprintf("close(%s) failed.\n", cpy->src_fname);
            // XXX explain err
        }
        else {
            cpy->dst_fd = -1;
        }
    }

    rv = 0;
    if (rv_copy || cpy->src_err || cpy->dst_err) {
        rv = -1;
    }

    if (rv == 0) {
        if (!(cpy->op & (APPEND | OVERWRITE))) {
            rv = copy_ftimes(cpy);
        }
    }

    if (rv) {
        if (!(cpy->op & APPEND)) {
            unlink(cpy->dst_fname);
        }
    }

    return (rv);
}

int
mmv_copy(mmv_t *mmv, FILEINFO *ff, size_t len)
{
    file_copy_t cpy;
    int rv;

    memset(&cpy, 0, sizeof (file_copy_t));
    cpy.src_fname = mmv->pathbuf;
    cpy.dst_fname = mmv->fullrep;

    cpy.dst_o_flags = O_CREAT | O_WRONLY;
    if (!(mmv->op & APPEND)) {
        cpy.dst_o_flags |= O_TRUNC;
    }
    cpy.dst_o_perm = (mmv->op & (APPEND | OVERWRITE))
            ? (~mmv->oldumask & RWMASK) | (ff->fi_mode & ~RWMASK)
            : ff->fi_mode;
    cpy.op = mmv->op;
    cpy.fsize = len;
    cpy.buf = NULL;
    cpy.bufsize = bufsize;

    rv = file_copy(&cpy);
    return (rv);
}
