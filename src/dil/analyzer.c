// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/object.c"
#include "dil/source.c"
#include "dil/string.c"
#include "dil/stringset.c"
#include "dil/stringsetmap.c"
#include "dil/tree.c"

/* Context of the analysis process. */
typedef struct {
    DilSource*      source;
    DilTree const*  tree;
    DilStringSet    symbols;
    DilStringSetMap firstReferences;
} DilAnalysisContext;

/* First pass of the analysis. */
void dil_analyze_first_pass(DilAnalysisContext* context)
{
    bool         seenStart    = false;
    size_t const INVALID_SKIP = 0;
    size_t       lastSkip     = INVALID_SKIP;

    // Skip the start symbol.
    for (size_t current = 1; current < dil_tree_size(context->tree);
         current++) {
        DilNode const* node = dil_tree_at(context->tree, current);

        switch (node->object.symbol) {
            case DIL_SYMBOL_SKIP: {
                if (lastSkip == INVALID_SKIP) {
                    if (node->childeren == 2) {
                        dil_source_warning(
                            context->source,
                            &node->object.value,
                            "Redundant no skip directive!");
                    }
                } else {
                    if (dil_tree_equal_sub(context->tree, lastSkip, current)) {
                        dil_source_warning(
                            context->source,
                            &node->object.value,
                            "Redundant skip directive!");
                    }
                }
                lastSkip = current;
                break;
            }
            case DIL_SYMBOL_START: {
                if (seenStart) {
                    dil_source_error(
                        context->source,
                        &node->object.value,
                        "Multiple start symbol directives!");
                }
                seenStart = true;
                break;
            }
            case DIL_SYMBOL_RULE: {
                DilNode const* name = node + 1;
                if (dil_string_set_contains(
                        &context->symbols,
                        &name->object.value)) {
                    dil_source_error(
                        context->source,
                        &name->object.value,
                        "Redefinition of the symbol!");
                    break;
                }

                DilStringSet firstReferences = {0};

                // Pattern.
                DilNode const* ref          = node + 3 + name->childeren;
                size_t         alternatives = ref->childeren;
                // Alternative.
                ref++;

                for (size_t i = 0; i < alternatives; i += 2) {
                    // Unit.
                    ref++;
                    // Reference.
                    ref++;
                    if (ref->object.symbol == DIL_SYMBOL_REFERENCE) {
                        // Identifier.
                        ref++;
                        dil_string_set_add(&firstReferences, ref->object.value);
                    }
                    for (size_t depth = i < alternatives - 1 ? 1 : 0;
                         depth > 0;) {
                        ref++;
                        switch (ref->object.symbol) {
                            case DIL_SYMBOL_ALTERNATIVE:
                                depth--;
                                break;
                            case DIL_SYMBOL_PATTERN:
                                depth += ref->childeren / 2 + 1;
                                break;
                            default:
                                break;
                        }
                    }
                }

                dil_string_set_add(&context->symbols, name->object.value);
                dil_string_set_map_add(
                    &context->firstReferences,
                    name->object.value,
                    firstReferences);
                break;
            }
            default:
                break;
        }
    }

    if (!seenStart) {
        dil_source_error(
            context->source,
            &context->source->contents,
            "Missing start symbol directive!");
    }
}

/* Second pass of the analysis. */
void dil_analyze_second_pass(DilAnalysisContext* context)
{
    // Skip the start symbol.
    for (size_t current = 1; current < dil_tree_size(context->tree);
         current++) {
        DilNode const* node = dil_tree_at(context->tree, current);

        switch (node->object.symbol) {
            case DIL_SYMBOL_RULE: {
                DilNode const*      name            = node + 1;
                DilStringSet const* firstReferences = dil_string_set_map_at(
                    &context->firstReferences,
                    &name->object.value);
                DilString const* selfCall =
                    dil_string_set_at(firstReferences, &name->object.value);
                if (selfCall != NULL) {
                    dil_source_error(
                        context->source,
                        selfCall,
                        "Rule has left recursion!");
                }
                // TODO: Check cycles.
                break;
            }
            case DIL_SYMBOL_IDENTIFIER: {
                DilNode const* previous =
                    dil_tree_at(context->tree, current - 1);
                if (previous->object.symbol == DIL_SYMBOL_REFERENCE) {
                    if (!dil_string_set_contains(
                            &context->symbols,
                            &node->object.value)) {
                        dil_source_error(
                            context->source,
                            &node->object.value,
                            "Reference to an undefined symbol!");
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}

/* Analyze the parse tree of the source file. */
void dil_analyze(DilSource* source, DilTree const* tree)
{
    DilAnalysisContext context = {.source = source, .tree = tree};

    dil_analyze_first_pass(&context);
    dil_analyze_second_pass(&context);

    dil_string_set_free(&context.symbols);
    for (DilStringSetMapPair* i = context.firstReferences.pairs.first;
         i < context.firstReferences.pairs.last;
         i++) {
        dil_string_set_free(&i->value);
    }
    dil_string_set_map_free(&context.firstReferences);
}
