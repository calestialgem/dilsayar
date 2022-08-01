// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/object.c"
#include "dil/source.c"
#include "dil/stringlist.c"
#include "dil/tree.c"

/* Context of the analysis process. */
typedef struct {
    DilSource*     source;
    DilTree const* tree;
    DilStringList  symbols;
} DilAnalysisContext;

/* First pass of the analysis. */
DilStringList dil_analyze_first_pass(DilSource* source, DilTree const* tree)
{
    DilStringList symbols   = {0};
    bool          seenStart = false;

    // Skip the start symbol.
    for (size_t current = 1; current < dil_tree_size(tree); current++) {
        DilNode const* node = dil_tree_at(tree, current);

        switch (node->object.symbol) {
            case DIL_SYMBOL_START: {
                if (seenStart) {
                    dil_source_error(
                        source,
                        &node->object.value,
                        "Multiple start symbol directives!");
                }
                seenStart = true;
                break;
            }
            case DIL_SYMBOL_IDENTIFIER: {
                DilNode const* previous = dil_tree_at(tree, current - 1);
                if (previous->object.symbol == DIL_SYMBOL_RULE) {
                    if (dil_string_list_contains(
                            &symbols,
                            &node->object.value)) {
                        dil_source_error(
                            source,
                            &node->object.value,
                            "Redefinition of the symbol!");
                    } else {
                        dil_string_list_add(&symbols, node->object.value);
                    }
                }
            }
            default:
                break;
        }
    }

    if (!seenStart) {
        dil_source_error(
            source,
            &source->contents,
            "Missing start symbol directive!");
    }

    return symbols;
}

/* Second pass of the analysis. */
void dil_analyze_second_pass(
    DilSource*           source,
    DilTree const*       tree,
    DilStringList const* symbols)
{
    // Skip the start symbol.
    for (size_t current = 1; current < dil_tree_size(tree); current++) {
        DilNode const* node = dil_tree_at(tree, current);

        switch (node->object.symbol) {
            case DIL_SYMBOL_IDENTIFIER: {
                DilNode const* previous = dil_tree_at(tree, current - 1);
                if (previous->object.symbol == DIL_SYMBOL_REFERENCE) {
                    if (!dil_string_list_contains(
                            symbols,
                            &node->object.value)) {
                        dil_source_error(
                            source,
                            &node->object.value,
                            "Reference to an undefined symbol!");
                    }
                }
            }
            default:
                break;
        }
    }
}

/* Analyze the parse tree of the source file. */
void dil_analyze(DilSource* source, DilTree const* tree)
{
    DilStringList symbols = dil_analyze_first_pass(source, tree);
    dil_analyze_second_pass(source, tree, &symbols);
    dil_string_list_free(&symbols);
}
