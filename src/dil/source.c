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
    DilString path;
    /* Contents of the info.contents. */
    DilString contents;
    /* Amount of errors in the file. */
    size_t errors;
    /* Amount of warnings in the file. */
    size_t warnings;
} DilSource;

/* Location of a character in the source file. */
typedef struct {
    /* As pointer. */
    char const* position;
    /* Line number, starting from 1. */
    size_t line;
    /* Column number, starting from 1. */
    size_t column;
} DilSourceLocation;

/* Portion of a file. */
typedef struct {
    /* Border before. */
    DilSourceLocation start;
    /* Border after. */
    DilSourceLocation end;
} DilSourcePortion;

/* Load the source file at the path to the memory to the buffer. */
DilSource dil_source_load(DilBuffer* buffer, DilString const* path)
{
    DilSource result = {.path = *path};

    DilString const VALID_EXTENSION = dil_string_terminated("dil");
    DilString       extension       = dil_string_split_last(path, '.').after;

    if (!dil_string_equal(&VALID_EXTENSION, &extension)) {
        printf(
            "%.*s: error: File extension should be `dil` not `%.*s`!\n",
            (int)dil_string_size(path),
            path->first,
            (int)dil_string_size(&extension),
            extension.first);
        result.errors++;
        return result;
    }

    DilBuffer pathBuffer = {0};

    dil_buffer_reserve(&pathBuffer, 1 + dil_string_size(path));
    pathBuffer.last += sprintf(
        pathBuffer.last,
        "%.*s",
        (int)dil_string_size(path),
        path->first);

    FILE* stream = fopen(pathBuffer.first, "r");

    if (stream == NULL) {
        printf(
            "%s: error: Could not open the file at path!\n",
            pathBuffer.first);
        result.errors++;
        return result;
    }
    dil_buffer_free(&pathBuffer);

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

/* Find the location of the character at the position in the source file. */
DilSourceLocation
dil_source_locate(DilSource const* source, char const* position)
{
    DilSourceLocation result = {.position = position, .line = 1, .column = 1};
    for (char const* i = source->contents.first; i < position; i++) {
        if (*i == '\n') {
            result.line++;
            result.column = 1;
        } else {
            result.column++;
        }
    }
    return result;
}

/* Find the location of the begining of the line the location is in. */
DilSourceLocation dil_source_locate_start(DilSourceLocation const* location)
{
    return (DilSourceLocation){
        .position = location->position - location->column + 1,
        .line     = location->line,
        .column   = 1};
}

/* Find the location of the end of the line the location is in. */
DilSourceLocation dil_source_locate_end(
    DilSource const*         source,
    DilSourceLocation const* location)
{
    DilSourceLocation result = *location;
    while (result.position < source->contents.last &&
           *result.position != '\n') {
        result.position++;
    }
    result.column += result.position - location->position;
    return result;
}

/* Find the portion of the string in the source file. */
DilSourcePortion
dil_source_find(DilSource const* source, DilString const* string)
{
    return (DilSourcePortion){
        .start = dil_source_locate(source, string->first),
        .end   = dil_source_locate(source, string->last)};
}

/* Find the line the location is in. */
DilSourcePortion
dil_source_find_line(DilSource const* source, DilSourceLocation const* location)
{
    return (DilSourcePortion){
        .start = dil_source_locate_start(location),
        .end   = dil_source_locate_end(source, location)};
}

/* Print the single line portion underlined. */
void dil_source_underline(
    DilSource const*        source,
    DilSourcePortion const* portion,
    bool                    dots)
{
    size_t const BUFFER_SIZE = 32;
    char         buffer[BUFFER_SIZE];
    (void)sprintf(buffer, "%8llu", portion->start.line);

    DilSourcePortion line = dil_source_find_line(source, &portion->start);
    printf(
        "%s | %.*s\n",
        buffer,
        (int)(line.end.position - line.start.position),
        line.start.position);

    if (dots) {
        printf("     ... | ");
    } else {
        printf("           ");
    }
    size_t column = 1;
    for (; column < portion->start.column; column++) {
        printf(" ");
    }
    for (; column < portion->end.column; column++) {
        printf("~");
    }
    printf("\n");
}

/* Print a portion of the source file. */
void dil_source_print(
    DilSource const* source,
    DilString const* string,
    char const*      type,
    char const*      message)
{
    DilSourcePortion portion = dil_source_find(source, string);
    printf(
        "%.*s:%llu:%llu:%llu:%llu: %s: %s\n",
        (int)dil_string_size(&source->path),
        source->path.first,
        portion.start.line,
        portion.start.column,
        portion.end.line,
        portion.end.column,
        type,
        message);

    if (portion.start.line == portion.end.line) {
        dil_source_underline(source, &portion, false);
    } else {
        DilSourcePortion startPortion = {
            .start = portion.start,
            .end   = dil_source_locate_end(source, &portion.start)};
        DilSourcePortion endPortion = {
            .start = dil_source_locate_start(&portion.end),
            .end   = portion.end};
        dil_source_underline(source, &startPortion, true);
        dil_source_underline(source, &endPortion, false);
    }
    printf("\n");
}

/* Report an error in the source file. */
void dil_source_error(
    DilSource*       source,
    DilString const* string,
    char const*      message)
{
    dil_source_print(source, string, "error", message);
    source->errors++;
}

/* Report a warning in the source file. */
void dil_source_warning(
    DilSource*       source,
    DilString const* string,
    char const*      message)
{
    dil_source_print(source, string, "warning", message);
    source->warnings++;
}
