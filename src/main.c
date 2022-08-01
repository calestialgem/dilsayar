// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dil/analyzer.c"
#include "dil/buffer.c"
#include "dil/builder.c"
#include "dil/generator.c"
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

    if (argumentCount < 2) {
        printf("Provide a `.dil` file!\n");
        return EXIT_FAILURE;
    }

    DilBuffer buffer = {0};

    for (int i = 1; i < argumentCount; i++) {
        dil_buffer_clear(&buffer);

        DilString path   = dil_string_terminated(arguments[i]);
        DilSource source = dil_source_load(&buffer, &path);
        DilTree   tree   = dil_parse(&source);

        if (source.errors == 0) {
            dil_tree_print_file(&tree, &path);
        }

        dil_analyze(&source, &tree);

        if (source.errors != 0) {
            printf(
                "%.*s: error: File had %llu errors.\n",
                (int)dil_string_size(&source.path),
                source.path.first,
                source.errors);
        } else {
            dil_generate_file(&tree, &path);
        }

        if (source.warnings != 0) {
            printf(
                "%.*s: warning: File had %llu warnings.\n",
                (int)dil_string_size(&source.path),
                source.path.first,
                source.warnings);
        }

        dil_tree_free(&tree);

        printf("\n");
    }

    dil_buffer_free(&buffer);
    return EXIT_SUCCESS;
}
