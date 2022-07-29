// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dil/buffer.c"
#include "dil/builder.c"
#include "dil/indices.c"
#include "dil/object.c"
#include "dil/parser.c"
#include "dil/source.c"
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

    DilBuffer buffer = {0};
    DilSource source = dil_source_load(&buffer, arguments[1]);
    DilTree   tree   = dil_parse(source);

    dil_tree_print_file(&tree);

    dil_tree_free(&tree);
    dil_buffer_free(&buffer);
    return EXIT_SUCCESS;
}
