/*
 * Filename: qp-decode-str.c
 * Library: libcscript
 * Brief: Decode a string that has been encoded into MIME Quote-Printable (QP)
 *
 * Description:
 *   Decode a string that has been encoded into MIME Quote-Printable (QP)
 *   encoding.
 *
 * See RFC-2045.
 * See https://en.wikipedia.org/wiki/Quoted-printable
 *
 * Copyright (C) 2015-2016 Guy Shaw
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

/**
 * @brief Decode a string that has been encoded into MIME Quote-Printable (QP)
 * @param buf  OUT  Decoded result
 * @param sz   IN   Capacity of |buf|.
 * @param str  IN   qp-encoded string to be decoded.
 * @return size-or-errno
 *
 * Implementation note
 * -------------------
 * It is safe to call qp_decode_str() with the source string
 * and the destination string-buffer the same, because,
 * at each stage during decoding, progressing from low
 * to high address for both source and destination,
 * the partial result is never longer than the source string.
 *
 * A "size-or-errno" return type is an |ssize_t| which is
 * the size of the result (positive or zero) on success,
 * but is negative on failure, and the negative number is typically
 * the negation of an errno value.
 *
 */
ssize_t
qp_decode_str(char *buf, size_t sz, const char *str)
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
        if (c == '=') {
            int hh, hl;
            ++s;
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
            buf[len++] = (hex_nybble(hh) << 4) | hex_nybble(hl);
        }
        else if (isprint(c) || c == 0x20 || c == '\t') {
            buf[len++] = c;
        }
        else if (c == '\r' || c == '\n') {
            // Skip
        }
        else {
            err = EINVAL;
            break;
        }
    }

    buf[len] = '\0';
    return (err ? -(ssize_t)err : size_to_ssize(len));
}
