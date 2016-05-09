/*
 * Filename: inc/dbgprint.h
 * Library: libcscript
 * Brief: Define convenience functions / macros for debugging
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

#ifndef DBGPRINT_H

#define DBGPRINT_H

#if defined(__GNUC__)

extern FILE *dbgprint_fh;

#define dbg_printf(fmt, ...) \
    ({ if (dbgprint_fh) { fprintf(dbgprint_fh, (fmt), ## __VA_ARGS__); }; })

#else

extern int dbg_printf(const char *, ...);

#endif /* __GNUC__ */

#define HERE \
    dbg_printf("\n===> %s, line %u\n", __FILE__, __LINE__)
#define HERE_FN \
    dbg_printf("\n===> %s, line %u, %s\n", __FILE__, __LINE__, __FUNCTION__)

#endif /* DBGPRINT_H */
