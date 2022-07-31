// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/tree.c"

#include <Windows.h>
#include <errhandlingapi.h>
#include <fileapi.h>
#include <stdio.h>

/* Append the code from the grammar tree to the stream. */
void dil_generate(FILE* stream, DilTree const* tree) {}

/* Write the code from the grammar tree to the default file. */
void dil_generate_file(DilTree const* tree)
{
    char PATH[] = "build\\output.c";
    if (!CreateDirectory("build", NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS) {
        printf("Could not create the build directory!\n");
        return;
    }
    FILE* outputStream = fopen(PATH, "w");
    if (outputStream == NULL) {
        printf("Could not open the output file %s!\n", PATH);
        return;
    }
    dil_generate(outputStream, tree);
    (void)fclose(outputStream);
}
