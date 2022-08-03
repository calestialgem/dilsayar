// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/indexmap.c"
#include "dil/object.c"
#include "dil/source.c"
#include "dil/string.c"
#include "dil/stringset.c"
#include "dil/stringsetmap.c"
#include "dil/tree.c"

#include <stdio.h>

/* Context of the analysis process. */
typedef struct {
    DilSource*      source;
    DilTree const*  tree;
    DilStringSet    symbols;
    DilStringSetMap firstReferences;
    DilIndexMap     rules;
} DilAnalysisContext;

/* Goes to the next alternative of the current pattern. */
DilNode const* dil_analyze_next_alternative(DilNode const* node)
{
    for (size_t depth = 1; depth > 0;) {
        node++;
        switch (node->object.symbol) {
            case DIL_SYMBOL_ALTERNATIVE:
                depth--;
                break;
            case DIL_SYMBOL_PATTERN:
                depth += node->childeren / 2 + 1;
                break;
            default:
                break;
        }
    }
    return node;
}

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

                for (size_t i = 0; i < alternatives; i += 2) {
                    // Alternative.
                    ref = dil_analyze_next_alternative(ref);
                    // Unit.
                    ref++;
                    // Reference.
                    ref++;
                    if (ref->object.symbol == DIL_SYMBOL_REFERENCE) {
                        // Identifier.
                        ref++;
                        dil_string_set_add(&firstReferences, ref->object.value);
                    }
                }

                dil_string_set_add(&context->symbols, name->object.value);
                dil_string_set_map_add(
                    &context->firstReferences,
                    name->object.value,
                    firstReferences);
                dil_index_map_add(&context->rules, name->object.value, current);
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

/* For use by the left recursion. */
void dil_analyze_left_recursion_callees(
    DilAnalysisContext const* context,
    DilString const*          definition,
    DilString const*          reference,
    DilStringSet*             checked);

/* Check for left recursion. */
void dil_analyze_left_recursion(
    DilAnalysisContext const* context,
    DilString const*          definition,
    DilString const*          reference,
    DilStringSet*             checked)
{
    if (dil_string_equal(reference, definition)) {
        dil_source_error(
            context->source,
            reference,
            "Rule has left recursion!");
        return;
    }
    dil_analyze_left_recursion_callees(context, definition, reference, checked);
}

/* Check for left recursion from the callees. */
void dil_analyze_left_recursion_callees(
    DilAnalysisContext const* context,
    DilString const*          definition,
    DilString const*          reference,
    DilStringSet*             checked)
{
    DilStringSet const* firstReferences =
        dil_string_set_map_at(&context->firstReferences, reference);
    for (DilString const* i = firstReferences->values.first;
         i < firstReferences->values.last;
         i++) {
        if (dil_string_set_contains(checked, i)) {
            continue;
        }
        dil_string_set_add(checked, *i);
        dil_analyze_left_recursion(context, definition, i, checked);
    }
}

/* Check for left recursion from the callees. Provides a string set by itself.
 */
void dil_analyze_left_recursion_callees_allocated(
    DilAnalysisContext const* context,
    DilString const*          definition,
    DilString const*          reference)
{
    DilStringSet checked = {0};
    dil_analyze_left_recursion_callees(
        context,
        definition,
        reference,
        &checked);
    dil_string_set_free(&checked);
}

/* Find the first unit in the unit or its callees. Returns NULL if the rule
 * has left recursion or referes an undefined symbol. */
DilNode const* dil_analyze_first_unit(
    DilAnalysisContext const* context,
    DilNode const*            unit,
    DilStringSet*             checked)
{
    // Reference.
    DilNode const* reference = unit + 1;
    if (reference->object.symbol != DIL_SYMBOL_REFERENCE) {
        return unit;
    }
    // Identifier.
    reference++;
    if (dil_string_set_contains(checked, &reference->object.value)) {
        return NULL;
    }
    dil_string_set_add(checked, reference->object.value);
    size_t const* callee =
        dil_index_map_at(&context->rules, &reference->object.value);
    if (callee == NULL) {
        return NULL;
    }
    // Rule.
    DilNode const* refered = dil_tree_at(context->tree, *callee);
    // Identifier.
    refered++;
    // Equal sign.
    refered += refered->childeren + 1;
    // Pattern.
    refered++;
    // Alternative.
    refered++;
    // Unit.
    refered++;
    return dil_analyze_first_unit(context, refered, checked);
}

/* Find the first unit in the unit or its calles. Provides a string set by
 * itself. */
DilNode const* dil_analyze_first_unit_allocated(
    DilAnalysisContext const* context,
    DilNode const*            unit)
{
    DilStringSet checked = {0};
    unit                 = dil_analyze_first_unit(context, unit, &checked);
    dil_string_set_free(&checked);
    return unit;
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
                DilNode const* name = node + 1;
                dil_analyze_left_recursion_callees_allocated(
                    context,
                    &name->object.value,
                    &name->object.value);
                break;
            }
            case DIL_SYMBOL_PATTERN: {
                // Pattern.
                DilNode const* refi = node;

                for (size_t i = 0; i + 2 < node->childeren; i += 2) {
                    // Alternative.
                    refi = dil_analyze_next_alternative(refi);
                    // Unit.
                    refi++;

                    DilNode const* uniti =
                        dil_analyze_first_unit_allocated(context, refi);
                    if (uniti == NULL) {
                        continue;
                    }

                    // Unit.
                    DilNode const* refj = refi;

                    for (size_t j = i + 2; j < node->childeren; j += 2) {
                        // Alternative.
                        refj = dil_analyze_next_alternative(refj);
                        // Unit.
                        refj++;

                        DilNode const* unitj =
                            dil_analyze_first_unit_allocated(context, refj);
                        if (unitj == NULL) {
                            continue;
                        }

                        if (dil_tree_equal(unitj, uniti)) {
                            dil_source_error(
                                context->source,
                                &refi->object.value,
                                "Alternatives need left factoring!");
                            dil_source_error(
                                context->source,
                                &refj->object.value,
                                "Alternatives need left factoring!");
                        }
                    }
                }
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
    dil_index_map_free(&context.rules);
}
