/*
 * Filename: src/inc/strbuf.h
 * Library: libcscript
 * Brief: Define data types for a mutable string buffer and an immutable string.
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

#ifndef STRBUF_H

#define STRBUF_H

#include <sys/types.h>


/*
 * "primitive" data types for:
 *   1) a mutable string buffer, with a capacity and a current length
 *   2) an immutable string, with just a current length.
 *
 */

struct string_buffer {
    char *s;
    size_t size;
    size_t len;
};

typedef struct string_buffer sbuf_t;


struct cstr {
    const char *s;
    size_t len;
};

typedef struct cstr cstr_t;

#endif /* STRBUF_H */
