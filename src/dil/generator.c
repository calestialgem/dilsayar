// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/tree.c"

#include <Windows.h>
#include <errhandlingapi.h>
#include <fileapi.h>
#include <stdio.h>
#include <time.h>

/* Append the code from the grammar tree to the stream. */
void dil_generate(FILE* stream, DilTree const* tree)
{
    (void)fprintf(stream, "// Auto-generated by Dilsayar.\n");
    time_t    nowStamp   = time(NULL);
    struct tm now        = *localtime(&nowStamp);
    int const START_YEAR = 1900;
    (void)fprintf(
        stream,
        "// Time: %02d.%02d.%02d Date: %02d.%02d.%d \n",
        now.tm_hour,
        now.tm_min,
        now.tm_sec,
        now.tm_mday,
        now.tm_mon + 1,
        now.tm_year + START_YEAR);
}

/* Write the code from the grammar tree to the default file. */
void dil_generate_file(DilTree const* tree, DilString const* path)
{
    DilString const EXTENSION       = dil_string_terminated("_code.c");
    DilString const BUILD_DIRECTORY = dil_string_terminated("build\\");
    DilSplit        pathSplit       = dil_string_split_last(path, '\\');
    DilBuffer       buffer          = {0};

    // Remove extension `.dil`.
    pathSplit.after.last -= 4;

    dil_buffer_reserve(
        &buffer,
        1 + dil_string_size(&BUILD_DIRECTORY) +
            dil_string_size(&pathSplit.before));
    buffer.last += sprintf(
        buffer.last,
        "%.*s%.*s",
        (int)dil_string_size(&BUILD_DIRECTORY),
        BUILD_DIRECTORY.first,
        (int)dil_string_size(&pathSplit.before),
        pathSplit.before.first);

    if (!CreateDirectory(buffer.first, NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS) {
        printf(
            "%.*s: error: Could not create the build directory `%s`!\n",
            (int)dil_string_size(path),
            path->first,
            buffer.first);
        return;
    }

    dil_buffer_reserve(
        &buffer,
        1 + dil_string_size(&EXTENSION) + dil_string_size(&pathSplit.after));
    buffer.last += sprintf(
        buffer.last,
        "%.*s%.*s",
        (int)dil_string_size(&pathSplit.after),
        pathSplit.after.first,
        (int)dil_string_size(&EXTENSION),
        EXTENSION.first);

    FILE* outputStream = fopen(buffer.first, "w");
    if (outputStream == NULL) {
        printf(
            "%.*s: error: Could not open the output file `%s`!\n",
            (int)dil_string_size(path),
            path->first,
            buffer.first);
        return;
    }
    dil_buffer_free(&buffer);
    dil_generate(outputStream, tree);
    (void)fclose(outputStream);
}
