/*
 * Filename: conv-size-to-ssize.c
 * Library: libcscript
 * Brief: Cast size_t to ssize_t while being paranoid about possible loss
 *
 * Description:
 *   Cast a size_t to ssize_t.  Ensure that the given size_t will fit
 *   into a ssize_t, without loss.
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

#include <cscript.h>

#include <stdlib.h>
    // Import abort()
#include <unistd.h>
    // Import type size_t


/*
 *
 * This module holds all of the miscellaneous safe casting functions.
 *
 * Too often, assignments or casts that could possibly lose data,
 * or inadvertently change semantics are ignored.  But here we try
 * to be paranoid.
 *
 * What assignments or casts apply?
 *
 * Any trivial assignments or casts that involve two data types,
 * where the destination type is smaller than the source type,
 * or where the underlying bits are the same, but the cast could
 * cause the meaning of bits to change.
 *
 * For any such assignments, write an explicit conversion function.
 *
 */


/**
 *
 * @brief Convert type |size_t| to type |ssize_t| with error checking.
 *
 * @param sz IN The given size (unsigned).
 * @return The equivalent signed size.
 *
 * A cast of a size_t that is too big is a fatal error.
 * That is, if a size is given that cannot fit into a signed size,
 * without changing sign, an error message will be printed and
 * the program will be aborted.
 *
 */
ssize_t
size_to_ssize(size_t sz)
{
    ssize_t ssz = (ssize_t)sz;
    size_t nnsz = (size_t)ssz;
    if (nnsz != sz) {
        eprintf("size too big.\n");
        abort();
    }
    return (ssz);
}
