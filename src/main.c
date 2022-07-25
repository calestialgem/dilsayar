// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sln/lib.c"

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

    // Call library.
    printf("%s\n", message());

    return EXIT_SUCCESS;
}
