/*
 * Filename: src/libmmv/ask-yesno.c
 * Library: libmmv
 * Brief: Prompt, then ask for a strict yes or no answer.
 *
 * Original mmv code by Vladimir Lanin. et al.
 * See the the file, LICENSE.md at the top of the libmmv tree.
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

#include <stdbool.h>
#include <stdio.h>

#include <eprint.h>

extern void quit(void);

static char TTY[] = "/dev/tty";

/**
 * @brief Prompt, then ask for a strict yes or no answer.
 *
 * @param  prompt  IN  The prompt string -- sent to tty before reading answer.
 * @param  failact IN  What to do in case of failure to get an answer
 * @return 0 for 'No', 1 for 'Yes'.
 *
 * failact:
 *   -1 means quit the program entirely, if we fail to get an answer.
 *   any other value will be the value returned (instead of 0 or 1),
 *   if anything goes wrong.
 *
 */

int
ask_yesno(const char *prompt, int failact)
{
    static FILE *tty = NULL;
    int c, r;

    eprint(prompt);
    if (tty == NULL && (tty = fopen(TTY, "r")) == NULL) {
        eprintf("Cannot open '%s' to get reply.\n", TTY);
        if (failact == -1) {
            quit();
        }
        else {
            return (failact);
        }
    }

    for (;;) {
        r = fgetc(tty);
        if (r == EOF) {
            eprint("Can not get reply.\n");
            if (failact == -1) {
                quit();
            }
            else {
                return (failact);
            }
        }
        if (r != '\n') {
            while ((c = fgetc(tty)) != '\n' && c != EOF);
        }
        if (r == 'Y' || r == 'y') {
            return (1);
        }
        if (r == 'N' || r == 'n') {
            return (0);
        }
        eprint("Yes or No? ");
    }
}
