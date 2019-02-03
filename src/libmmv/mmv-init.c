/*
 * Filename: src/libmmv/mmv-init.c
 * Library: libmmv
 * Brief: Allocate and initialize an mmv_t object
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

#include <stdio.h>
#include <string.h>             // Import memset()

#include <sys/types.h>          // Import type mode_t for umask()
#include <sys/stat.h>

#define IMPORT_PATTERN
#define IMPORT_REP
#include <mmv-impl.h>
#include <cscript.h>

extern FILE *dbgprint_fh;

/**
 * @brief Initialize all values in an existing mmv_t object.
 *
 * @param mmv  INOUT A context object to control libmmv operations.
 *
 * The given pointer, |mmv|, is assumed to point to sufficient space
 * but is otherwise not trusted.  The most common cases are that
 * mmv points to an uninitialized auto variable on the stack or
 * to a freshly allocated chunk of memory, or that it is being reused.
 * In any case, its contents is not trusted.
 *
 * All fields are initialized to some reasonable default value.
 *
 */

void
mmv_state_init(mmv_t *mmv)
{
    memset(mmv, 0, sizeof (mmv_t));

    mmv->magic    = MMV_MAGIC;
    mmv->debug_fh = dbgprint_fh;
    mmv->verbose  = false;

    mmv->outfile  = stdout;
    // XXX if stdout and stderr are same fd, then make same stdio FILE
    mmv->errfile  = stderr;

    mmv->encoding = ENCODE_NONE;
    mmv->oldumask = umask(0);

    mmv->from     = (char *)guard_malloc(MAXPATLEN);
    mmv->fromsz   = MAXPATLEN;
    mmv->to       = (char *)guard_malloc(MAXPATLEN);
    mmv->tosz     = MAXPATLEN;

    mmv->pathbuf  = (char *)guard_malloc(PATH_MAX);
    mmv->fullrep  = (char *)guard_malloc(PATH_MAX + 1);

    mmv->lastrep  = &mmv->hrep;
    mmv->aux      = NULL;
}

/**
 * @brief Allocate and initialize a context / controlling data structure
 *
 */

mmv_t *
mmv_new(void)
{
    init_sys();
    init_dostage();
    mmv_t *new_mmv = (mmv_t *) guard_malloc(sizeof (mmv_t));
    mmv_state_init(new_mmv);
    return (new_mmv);
}
