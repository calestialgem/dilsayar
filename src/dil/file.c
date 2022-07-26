// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/buffer.c"
#include "dil/string.c"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Information of a info.contents. */
typedef struct {
    /* Path to the info.contents. */
    char const* path;
    /* Contents of the info.contents. */
    DilString contents;
    /* Whether the file has errors. */
    size_t error;
} DilFile;

/* Load the file at the path to the memory to the buffer. */
DilFile dil_file_load(DilBuffer* buffer, char const* path)
{
    FILE*   stream = fopen(path, "r");
    DilFile result = {.path = path};

    if (stream == NULL) {
        printf("Could not open file %s!\n", path);
        result.error++;
        return result;
    }

    size_t       start = dil_buffer_size(buffer);
    size_t const CHUNK = 1024;
    size_t       read  = CHUNK;

    while (read == CHUNK) {
        dil_buffer_reserve(buffer, CHUNK);
        read = fread(buffer->last, sizeof(char), CHUNK, stream);
        buffer->last += read;
    }
    (void)fclose(stream);

    result.contents.first = buffer->first + start;
    result.contents.last  = buffer->last;
    return result;
}

/* Print a portion of the file. */
void dil_file_print(
    DilFile*    file,
    DilString   portion,
    char const* type,
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
    for (char const* i = file->contents.first; i < portion.last; i++) {
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
        file->path,
        startLineNumber,
        startColumnNumber,
        type,
        message);

    char lineNumberString[32];

    if (startLineNumber == endLineNumber) {
        char const* lineStart = portion.first;
        while (lineStart > file->contents.first && *lineStart != '\n') {
            lineStart--;
        }
        lineStart++;

        (void)sprintf(lineNumberString, "%8llu", startLineNumber);

        int lineLength = 0;
        for (char const* i = lineStart; i < file->contents.last && *i != '\n';
             i++) {
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
        while (lineStart > file->contents.first && *lineStart != '\n') {
            lineStart--;
        }
        lineStart++;

        (void)sprintf(lineNumberString, "%8llu", startLineNumber);
        int lineLength = 0;
        for (char const* i = lineStart; i < file->contents.last && *i != '\n';
             i++) {
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
        while (lineStart > file->contents.first && *lineStart != '\n') {
            lineStart--;
        }
        lineStart++;

        (void)sprintf(lineNumberString, "%8llu", endLineNumber);
        lineLength = 0;
        for (char const* i = lineStart; i < file->contents.last && *i != '\n';
             i++) {
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
