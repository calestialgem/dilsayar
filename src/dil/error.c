// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/string.c"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Print a portion of the input string. */
void dil_message(DilString file, DilString portion)
{
    size_t    lineNumber   = 1;
    size_t    columnNumber = 1;
    DilString line         = file;
    for (char const* i = file.first; i < portion.first; i++) {
        if (*i == '\n') {
            lineNumber++;
            columnNumber = 1;
            line.first   = i;
        } else {
            columnNumber++;
        }
    }

    line.last = line.first;
    while (line.last < file.last && *line.last != '\n') {
        line.last++;
    }

    char lineNumberString[32];
    (void)sprintf(lineNumberString, "%llu", lineNumber);

    printf(
        "%s | %.*s\n",
        lineNumberString,
        (int)dil_string_size(&line),
        line.first);

    size_t start = strlen(lineNumberString) + 3 + columnNumber;
    for (size_t i = 0; i < start; i++) {
        printf(" ");
    }
    for (size_t i = 0; i < dil_string_size(&portion); i++) {
        printf("^");
    }
    printf("\n");
}
