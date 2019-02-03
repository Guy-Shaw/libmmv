/*
 * Filename: src/libmmv/mmv-sys.c
 * Library: libmmv
 * Brief: Global data cache of system properties
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

#include <signal.h>
    // Import constant SIGINT
    // Import signal()
#include <stddef.h>
    // Import constant NULL
#include <stdlib.h>
    // Import getenv()
#include <string.h>
    // Import strcmp()
    // Import strlen()
#include <sys/types.h>
    // Import type uid_t
#include <unistd.h>
    // Import geteuid()
    // Import getuid()
    // Import type size_t

typedef void sigreturn_t;    // XXX Unify signal typedefs

extern sigreturn_t breakout(int s);

/*
 * Constants
 */

static char home_empty[] = "";

/*
 * Declare global data
 */

size_t name_max;
uid_t uid, euid;
char *sys_home;
size_t sys_homelen;

/*
 * @brief: Global data cache of system properties
 */

void
init_sys(void)
{
    if (sys_home == NULL) {
        sys_home = getenv("HOME");
    }

    if (sys_home == NULL || strcmp(sys_home, "/") == 0) {
        sys_home = home_empty;
    }
    sys_homelen = strlen(sys_home);

    // XXX Use patchconf(_, _PC_NAME_MAX).
    // XXX but this can be different for different filesystems.
    // XXX So, it should be computed and cached for each filesystem.
    //
    name_max = 255;

    euid = geteuid();
    uid = getuid();
    signal(SIGINT, breakout);
}
