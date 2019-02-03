/*
 * Filename: xnn-decode-str.c
 * Library: libcscript
 * Brief: decode a string that has been xnn-encoded.
 *
 * Description:
 *   Given a source string that has been encoded in xnn format,
 *   and a destination buffer, build the decoded version of that
 *   string.
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

/*
 * Decode a string that has been encoded by xnn_encode_str().
 *
 * Implementation note
 * -------------------
 * It is safe to call xnn_decode_str() with the same address for
 * the source string and the destination buffer, because encoded
 * strings are always the same length or longer than decoded strings.
 *
 */

#include <ctype.h>	// Import isdigit(), isxdigit(), isprint(), isspace()
#include <unistd.h>	// Import size_t
#include <errno.h>	// Import ENAMETOOLONG
#include <stdlib.h>     // Import abort()

extern ssize_t size_to_ssize(size_t sz);

static inline int
hex_nybble(int xd)
{
    if (isdigit(xd)) {
        return (xd - '0');
    }
    else if (xd >= 'a' && xd <= 'f') {
        return (10 + xd - 'a');
    }
    else if (xd >= 'A' && xd <= 'F') {
        return (10 + xd - 'A');
    }
    else {
        abort();
    }
}

ssize_t
xnn_decode_str(char *buf, size_t sz, const char *str)
{
    const char *s;
    size_t len;
    int err;
    int c;

    err = 0;
    len = 0;
    for (s = str; (c = *s) != 0 ; ++s) {
        if (len >= sz) {
            err = ENAMETOOLONG;
            break;
        }
        if (c == '\\' && s[1] == 'x') {
            int hh, hl;
            s += 2;
            hh = *s;
            if (!(hh && isxdigit(hh))) {
                err = EINVAL;
                break;
            }
            ++s;
            hl = *s;
            if (!(hl && isxdigit(hl))) {
                err = EINVAL;
                break;
            }
            c = (hex_nybble(hh) << 4) | hex_nybble(hl);
        }

        buf[len++] = c;
    }

    buf[len] = '\0';
    return (err ? -(ssize_t)err : size_to_ssize(len));
}
