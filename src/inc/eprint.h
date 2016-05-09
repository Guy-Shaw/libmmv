/*
 * Filename: inc/eprint.h
 * Library: libcscript
 * Brief: Define convenience functions / macros for printing error messages
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

#ifndef EPRINT_H

#define EPRINT_H

#define eprint(str) fputs((str), stderr)
#define eputchar(c) putc((c), stderr)

#if defined(__GNUC__)

#define eprintf(fmt, ...) \
    fprintf(stderr, (fmt), ## __VA_ARGS__)

#else

extern int eprintf(const char *, ...);

#endif /* __GNUC__ */

#endif /* EPRINT_H */
