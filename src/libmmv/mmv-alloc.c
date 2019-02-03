/*
 * Filename: src/libmmv/mmv-alloc.c
 * Library: libmmv
 * Brief: Special memory allocator specific to mmv.
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


/*
 * XXX
 * To do
 * -----
 * Convert to use a general-purpose allocator.
 * What might have been needed in 1990 is probably
 * not needed or desirable on modern operating systems.
 *
 * Also, the direct use of challoc() and chgive() might
 * be done better with GNU libiberty-style Obstacks,
 * or a slab allocator a la Solaris kmem_cache_alloc(),
 * or since we are in userland, umem_cache_alloc().
 *
 * Priority: low
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include <mmv-impl.h>

struct memchunk;
struct slicer;

typedef struct memchunk memchunk_t;
typedef struct slicer slicer_t;

struct memchunk {
    memchunk_t *ch_next;
    size_t ch_len;
};

struct slicer {
    memchunk_t *sl_first;
    char *  sl_unused;
    size_t  sl_len;
};

#define CHUNKSIZE 2048

static memchunk_t *freechunks = NULL;
static slicer_t slicer[2] = {
    {NULL, NULL, 0},
    {NULL, NULL, 0}
};

void *
mmv_alloc(size_t sz)
{
    void *new_ptr;

    if (sz == 0) {
        return (NULL);
    }
    new_ptr = malloc(sz);
    if (new_ptr == NULL) {
        fprintf(stderr, "Insufficient memory.\n");
        mmv_abort();
    }
    return (new_ptr);
}

void *
mmv_realloc(void *ptr, size_t sz)
{
    void *new_ptr;

    if (sz == 0) {
        return (NULL);
    }
    new_ptr = realloc(ptr, sz);
    if (new_ptr == NULL) {
        fprintf(stderr, "Insufficient memory.\n");
        mmv_abort();
    }
    return (new_ptr);
}

void *
challoc(size_t sz, unsigned int which)
{
    void *ret;
    slicer_t *sl = &(slicer[which]);

    if (sz > sl->sl_len) {
        memchunk_t *p, *q;

        q = NULL;
        p = freechunks;
        while (p != NULL && (sl->sl_len = p->ch_len) < sz) {
            q = p;
            p = p->ch_next;
        }

        if (p == NULL) {
            sl->sl_len = CHUNKSIZE - sizeof (memchunk_t *);
            p = (memchunk_t *) mmv_alloc(CHUNKSIZE);
        }
        else if (q == NULL) {
            freechunks = p->ch_next;
        }
        else {
            q->ch_next = p->ch_next;
        }
        p->ch_next = sl->sl_first;
        sl->sl_first = p;
        sl->sl_unused = (char *)&(p->ch_len);
    }
    sl->sl_len -= sz;
    ret = (void *)sl->sl_unused;
    sl->sl_unused += sz;
    return (ret);
}

void
chgive(void *vp, size_t sz)
{
    memchunk_t *mp = (memchunk_t *)vp;

    mp->ch_len = sz - sizeof (memchunk_t *);
    mp->ch_next = freechunks;
    freechunks = mp;
}
