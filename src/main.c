// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dil/builder.c"
#include "dil/error.c"
#include "dil/indices.c"
#include "dil/object.c"
#include "dil/parser.c"
#include "dil/string.c"
#include "dil/tree.c"

#include <stddef.h>
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

    DilFile info = dil_file_load(arguments[1]);
    DilTree tree = {0};

    DilBuilder builder = {.built = &tree};
    dil_parse(&builder, &info);
    dil_builder_free(&builder);

    dil_tree_print(&tree);
    dil_tree_free(&tree);

    return EXIT_SUCCESS;
}
