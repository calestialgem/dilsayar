// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/buffer.c"
#include "dil/indexmap.c"
#include "dil/object.c"
#include "dil/source.c"
#include "dil/string.c"
#include "dil/stringset.c"
#include "dil/stringsetmap.c"
#include "dil/tree.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    // Under.
    unit++;
    switch (unit->object.symbol) {
        case DIL_SYMBOL_OPTIONAL:
        case DIL_SYMBOL_ZERO_OR_MORE:
        case DIL_SYMBOL_ONE_OR_MORE:
            // Repeat character.
            unit++;
            // Unit.
            unit++;
            break;
        case DIL_SYMBOL_FIXED_TIMES:
            // Number.
            unit++;
            // Unit.
            unit += unit->childeren + 1;
            break;
        case DIL_SYMBOL_GROUP:
            // Opening bracket.
            unit++;
            // Pattern.
            unit++;
            // Alternative.
            unit++;
            // Unit.
            unit++;
            break;
        case DIL_SYMBOL_REFERENCE: {
            // Identifier.
            unit++;
            if (dil_string_set_contains(checked, &unit->object.value)) {
                return NULL;
            }
            dil_string_set_add(checked, unit->object.value);
            size_t const* callee =
                dil_index_map_at(&context->rules, &unit->object.value);
            if (callee == NULL) {
                return NULL;
            }
            // Rule.
            unit = dil_tree_at(context->tree, *callee);
            // Identifier.
            unit++;
            // Equal sign.
            unit += unit->childeren + 1;
            // Pattern.
            unit++;
            // Alternative.
            unit++;
            // Unit.
            unit++;
            break;
        }
        default:
            // Unit.
            return --unit;
    }
    return dil_analyze_first_unit(context, unit, checked);
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

/* Extract the set of first characters from the terminal unit, and whether the
 * set is characters not to have. */
void dil_analyze_first_character(
    DilAnalysisContext const* context,
    DilNode const*            unit,
    DilBuffer*                buffer,
    bool * not )
{
    *not = false;
    // Under.
    unit++;
    switch (unit->object.symbol) {
        case DIL_SYMBOL_NOT_SET:
            *not = true;
            // Exclamation mark.
            unit++;
            // Set.
            unit++;
        case DIL_SYMBOL_SET:
            for (char const* i = unit->object.value.first + 1;
                 i < unit->object.value.last - 1;
                 i++) {
                size_t remaining = unit->object.value.last - 1 - i;
                if (remaining < 3 || *(i + 1) != '~') {
                    dil_buffer_add(buffer, *i);
                    continue;
                }
                for (char j = *i; j < *(i += 2); j++) {
                    dil_buffer_add(buffer, j);
                }
            }
            break;
        case DIL_SYMBOL_STRING:
            for (char const* i = unit->object.value.first + 1;
                 i < unit->object.value.last - 1;
                 i++) {
                dil_buffer_add(buffer, *i);
                break;
            }
            break;
        default:
            DilBuffer    messageBuffer     = {0};
            char const*  file              = __FILE__;
            unsigned     line              = __LINE__;
            char const*  message           = "Unexpected nonterminal unit!";
            size_t const MAX_NUMBER_LENGTH = 32;
            dil_buffer_reserve(
                &messageBuffer,
                3 + strlen(file) + MAX_NUMBER_LENGTH + strlen(message) + 1);
            messageBuffer.last +=
                sprintf(messageBuffer.first, "%s:%u: %s", file, line, message);
            dil_source_print(
                context->source,
                &unit->object.value,
                "internal error",
                messageBuffer.first);
            dil_buffer_free(&messageBuffer);
    }
}

/* Whether the first units are the same. Checks for the string and character
 * terminals as well. Assumes the passed nodes are first units. */
bool dil_analyze_first_unit_equal(
    DilAnalysisContext const* context,
    DilNode const*            lhs,
    DilNode const*            rhs)
{
    if (dil_tree_equal(lhs, rhs)) {
        return true;
    }
    DilBuffer lhsBuffer = {0};
    DilBuffer rhsBuffer = {0};
    bool      lhsNot    = false;
    bool      rhsNot    = false;

    dil_analyze_first_character(context, lhs, &lhsBuffer, &lhsNot);
    dil_analyze_first_character(context, rhs, &rhsBuffer, &rhsNot);

    DilString lhsSet = {.first = lhsBuffer.first, .last = lhsBuffer.last};
    DilString rhsSet = {.first = rhsBuffer.first, .last = rhsBuffer.last};
    bool      result = false;

    if (!dil_string_finite(&lhsSet) || !dil_string_finite(&rhsSet)) {
        goto end;
    }
    if (lhsNot == rhsNot) {
        for (char const* i = lhsSet.first; i < lhsSet.last; i++) {
            if (dil_string_contains(&rhsSet, *i)) {
                result = true;
                goto end;
            }
        }
    } else {
        if (lhsNot) {
            rhsNot        = true;
            lhsNot        = false;
            DilString tmp = lhsSet;
            lhsSet        = rhsSet;
            rhsSet        = tmp;
        }
        for (char const* i = lhsSet.first; i < lhsSet.last; i++) {
            if (!dil_string_contains(&rhsSet, *i)) {
                result = true;
                goto end;
            }
        }
    }
end:
    dil_buffer_free(&lhsBuffer);
    dil_buffer_free(&rhsBuffer);
    return result;
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

                        if (dil_analyze_first_unit_equal(
                                context,
                                unitj,
                                uniti)) {
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
