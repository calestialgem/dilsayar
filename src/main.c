// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dil/error.c"
#include "dil/string.c"

#include <stdio.h>
#include <stdlib.h>

/* Start the program. */
int main(int argumentCount, char const* const* arguments)
{
    // Print the arguments.
    printf("Running with arguments:\n");
    for (int i = 0; i < argumentCount; i++) {
        printf("[%d] {%s}\n", i, arguments[i]);
    }
    printf("\n");

    DilString file =
        dil_string_terminated("Some multi\nline\nstring\n\nwhich\nhas\nlines");
    DilSplit split = dil_string_split_first(&file, '\n');

    split.after.first += 3;
    split.after.last -= 2;

    dil_message(
        &(DilFile){.path = "imaginary.dil", .contents = file},
        split.after,
        "info",
        "There is something here!");

    file = dil_string_terminated(
        "A file that is alot longer! This would mean\nthat we will see alot "
        "more on the console.\nOk?");
    DilString portion = {.first = file.first + 25, .last = file.first + 85};

    dil_message(
        &(DilFile){.path = "imaginary.dil", .contents = file},
        portion,
        "info",
        "There is something here!");

    return EXIT_SUCCESS;
}
