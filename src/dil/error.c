// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/string.c"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Print a portion of the input string. */
void dil_message(
    DilString   file,
    DilString   portion,
    char const* type,
    char const* path,
    char const* message)
{
    if (dil_string_finite(&portion)) {
        if (dil_string_starts(&portion, '\n')) {
            portion.first++;
        }
    }
    size_t startLineNumber   = 1;
    size_t startColumnNumber = 1;
    size_t endLineNumber     = 1;
    size_t endColumnNumber   = 1;
    for (char const* i = file.first; i < portion.last; i++) {
        if (i < portion.first) {
            if (*i == '\n') {
                startLineNumber++;
                startColumnNumber = 1;
            } else {
                startColumnNumber++;
            }
        }
        if (*i == '\n') {
            endLineNumber++;
            endColumnNumber = 1;
        } else {
            endColumnNumber++;
        }
    }

    printf(
        "%s:%llu:%llu: %s: %s\n",
        path,
        startLineNumber,
        startColumnNumber,
        type,
        message);

    char lineNumberString[32];

    if (startLineNumber == endLineNumber) {
        char const* lineStart = portion.first;
        while (lineStart > file.first && *lineStart != '\n') {
            lineStart--;
        }
        lineStart++;

        (void)sprintf(lineNumberString, "%8llu", startLineNumber);

        int lineLength = 0;
        for (char const* i = lineStart; i < file.last && *i != '\n'; i++) {
            lineLength++;
        }

        printf("%s | %.*s\n", lineNumberString, lineLength, lineStart);
        lineStart += lineLength + 1;

        size_t start = strlen(lineNumberString) + 3 + startColumnNumber - 1;
        for (size_t i = 0; i < start; i++) {
            printf(" ");
        }
        size_t length = endColumnNumber - startColumnNumber;
        for (size_t i = 0; i < length; i++) {
            printf("~");
        }
    } else {
        char const* lineStart = portion.first;
        while (lineStart > file.first && *lineStart != '\n') {
            lineStart--;
        }
        lineStart++;

        (void)sprintf(lineNumberString, "%8llu", startLineNumber);
        int lineLength = 0;
        for (char const* i = lineStart; i < file.last && *i != '\n'; i++) {
            lineLength++;
        }

        printf("%s | %.*s\n", lineNumberString, lineLength, lineStart);
        lineStart += lineLength + 1;

        printf("     ... |");

        size_t start =
            strlen(lineNumberString) + 3 + startColumnNumber - 1 - 10;
        for (size_t i = 0; i < start; i++) {
            printf(" ");
        }
        size_t length = lineLength - startColumnNumber + 1;
        for (size_t i = 0; i < length; i++) {
            printf("~");
        }

        lineStart = portion.last;
        while (lineStart > file.first && *lineStart != '\n') {
            lineStart--;
        }
        lineStart++;

        (void)sprintf(lineNumberString, "%8llu", endLineNumber);
        lineLength = 0;
        for (char const* i = lineStart; i < file.last && *i != '\n'; i++) {
            lineLength++;
        }

        printf("\n%s | %.*s\n", lineNumberString, lineLength, lineStart);
        lineStart += lineLength + 1;

        start = strlen(lineNumberString) + 3;
        for (size_t i = 0; i < start; i++) {
            printf(" ");
        }
        length = endColumnNumber - 1;
        for (size_t i = 0; i < length; i++) {
            printf("~");
        }
    }
    printf("\n\n");
}
