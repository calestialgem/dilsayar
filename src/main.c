// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dil/builder.c"
#include "dil/error.c"
#include "dil/parser.c"
#include "dil/string.c"
#include "dil/tree.c"

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

    FILE*  file = fopen(arguments[1], "r");
    char   buffer[1 << 16];
    size_t length = fread(buffer, 1, 1 << 16, file);
    fclose(file);

    DilFile info = {
        .path     = arguments[1],
        .contents = {.first = buffer, .last = buffer + length}
    };

    DilTree    tree    = {0};
    DilBuilder builder = {.built = &tree};

    dil_parse(&builder, &info);

    dil_builder_free(&builder);
    dil_tree_free(&tree);

    return EXIT_SUCCESS;
}
