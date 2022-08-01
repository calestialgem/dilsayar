// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/object.c"
#include "dil/source.c"
#include "dil/tree.c"

/* Analyze the parse tree of the source file. */
void dil_analyze(DilSource* source, DilTree const* tree)
{
    bool seenStart = false;

    // Skip the start symbol.
    for (size_t current = 1; current < dil_tree_size(tree); current++) {
        DilNode const* node = dil_tree_at(tree, current);
        if (node->object.symbol == DIL_SYMBOL_START) {
            if (seenStart) {
                dil_source_print(
                    source,
                    &node->object.value,
                    "error",
                    "Multiple start symbol directives!");
                source->error++;
            }
            seenStart = true;
        }
    }

    if (!seenStart) {
        dil_source_print(
            source,
            &source->contents,
            "error",
            "Missing start symbol directive!");
        source->error++;
    }
}
