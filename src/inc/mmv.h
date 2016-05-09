/*
 * Filename: src/inc/mmv.h
 * Library: libmmv
 * Brief: libmmv interface
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

#ifndef MMV_H
#define MMV_H

struct mmv_state;
typedef struct mmv_state mmv_t;

extern mmv_t *mmv_new(void);
extern int mmv_get_pairs_nul(mmv_t *mmv, FILE *);
extern int mmv_get_pairs_qp(mmv_t *mmv, FILE *);
extern int mmv_get_pairs_xnn(mmv_t *mmv, FILE *);
extern int mmv_add_pair(mmv_t *mmv, char const *src_fname, char const *dst_fname);
extern int mmv_pairs(mmv_t *mmv);
extern int mmv_setopt(mmv_t *mmv, int);
extern int patgen(mmv_t *mmv, int argc, char *const *argv);

#endif /* MMV_H */
