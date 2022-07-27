// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/buffer.c"
#include "dil/builder.c"
#include "dil/object.c"
#include "dil/source.c"
#include "dil/string.c"
#include "dil/tree.c"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/* Context of the parsing process. */
typedef struct {
    /* Builder to parse into. */
    DilBuilder* builder;
    /* Remaining source file contents. */
    DilString* string;
    /* Parsed source file. */
    DilSource* source;
    /* Stack depth further away from the last nonterminal. */
    int depth;
} DilParseContext;

/* Macro for the start of a try-parse function. */
#define dil_parse__create(SYMBOL, TERMINAL)                   \
    char const* untouched = context->string->first;           \
    bool        terminal  = (TERMINAL);                       \
    if (terminal) {                                           \
        context->depth++;                                     \
    }                                                         \
    if (context->depth < 2) {                                 \
        dil_builder_add(                                      \
            context->builder,                                 \
            (DilObject){                                      \
                .symbol = (SYMBOL),                           \
                .value  = {.first = context->string->first}}); \
        dil_builder_push(context->builder);                   \
    }

/* Macro for the return of a try-parse function. */
#define dil_parse__return(ACCEPT)                                     \
    if (terminal) {                                                   \
        context->depth--;                                             \
    }                                                                 \
    if ((ACCEPT)) {                                                   \
        if (context->depth < 1) {                                     \
            dil_builder_parent(context->builder)->object.value.last = \
                context->string->first;                               \
            dil_builder_pop(context->builder);                        \
        }                                                             \
        return true;                                                  \
    }                                                                 \
    context->string->first = untouched;                               \
    if (context->depth < 1) {                                         \
        dil_builder_pop(context->builder);                            \
        dil_tree_remove(context->builder->built);                     \
        dil_tree_at(                                                  \
            context->builder->built,                                  \
            *dil_indices_finish(&context->builder->parents))          \
            ->childeren--;                                            \
    }                                                                 \
    return false;

/* Try to skip a comment. */
bool dil_parse__skip_comment(DilParseContext* context)
{
    DilString const TERMINALS_0 = dil_string_terminated("//");
    DilString const SET_0       = dil_string_terminated("\n");

    if (!dil_string_prefix_check(context->string, &TERMINALS_0)) {
        return false;
    }

    while (dil_string_prefix_not_set(context->string, &SET_0)) {}

    return true;
}

/* Try to skip whitespace. */
bool dil_parse__skip_whitespace(DilParseContext* context)
{
    DilString const SET_0 = dil_string_terminated("\t\n ");
    return dil_string_prefix_set(context->string, &SET_0);
}

/* Try to skip once. */
bool dil_parse__skip_once(DilParseContext* context)
{
    return dil_parse__skip_whitespace(context) ||
           dil_parse__skip_comment(context);
}

/* Skip as much as possible. */
void dil_parse__skip(DilParseContext* context)
{
    while (dil_parse__skip_once(context)) {}
}

/* Skip erronous characters and print them. */
void dil_parse__error(DilParseContext* context, char const* message)
{
    DilString portion = {
        .first = context->string->first,
        .last  = context->string->first};
    while (context->string->first <= context->string->last &&
           !dil_parse__skip_once(context)) {
        context->string->first++;
        portion.last++;
    }
    context->source->error++;
    dil_source_print(context->source, &portion, "error", message);
}

/* Print the expected set and skip the erronous characters. */
void dil_parse__error_set(DilParseContext* context, DilString const* set)
{
    size_t const BUFFER_SIZE = 1024;
    char         buffer[BUFFER_SIZE];
    (void)sprintf_s(
        buffer,
        BUFFER_SIZE,
        "Expected one of `%.*s` in `String`!",
        (int)dil_string_size(set),
        set->first);
    dil_parse__error(context, buffer);
}

/* Try to parse an identifier. */
bool dil_parse_identifier(DilParseContext* context)
{
    dil_parse__create(DIL_SYMBOL_IDENTIFIER, true);

    DilString const SET_0 = dil_string_terminated("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    DilString const SET_1 = dil_string_terminated(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

    if (!dil_string_prefix_set(context->string, &SET_0)) {
        dil_parse__return(false);
    }

    while (dil_string_prefix_set(context->string, &SET_1)) {}

    dil_parse__return(true);
}

/* Try to parse an escaped character. */
bool dil_parse_escaped(DilParseContext* context)
{
    dil_parse__create(DIL_SYMBOL_ESCAPED, true);

    DilString const SET_0 = dil_string_terminated("0123456789abcdefABCDEF");
    DilString const SET_1 = dil_string_terminated("tn\\'~");
    DilString const SET_2 = dil_string_terminated("\\'~");

    if (dil_string_prefix_element(context->string, '\\')) {
        if (dil_string_prefix_set(context->string, &SET_0)) {
            for (size_t i = 0; i < 2 - 1; i++) {
                if (!dil_string_prefix_set(context->string, &SET_0)) {
                    dil_parse__error_set(context, &SET_0);
                    dil_parse__return(false);
                }
            }
            dil_parse__return(true);
        }

        if (dil_string_prefix_set(context->string, &SET_1)) {
            dil_parse__return(true);
        }

        dil_parse__error(context, "Unexpected character in `Escaped`!");
        dil_parse__return(false);
    }

    if (dil_string_prefix_not_set(context->string, &SET_2)) {
        dil_parse__return(true);
    }

    dil_parse__return(false);
}

/* Try to parse a number. */
bool dil_parse_number(DilParseContext* context)
{
    dil_parse__create(DIL_SYMBOL_NUMBER, true);

    DilString const SET_0 = dil_string_terminated("123456789");
    DilString const SET_1 = dil_string_terminated("0123456789");

    if (!dil_string_prefix_set(context->string, &SET_0)) {
        dil_parse__return(false);
    }

    dil_parse__skip(context);

    while (dil_string_prefix_set(context->string, &SET_1)) {}

    dil_parse__return(true);
}

/* Try to parse a set. */
bool dil_parse_set(DilParseContext* context)
{
    dil_parse__create(DIL_SYMBOL_SET, true);

    if (!dil_string_prefix_element(context->string, '\'')) {
        dil_parse__return(false);
    }

    while (dil_parse_escaped(context)) {
        if (!dil_string_prefix_element(context->string, '~')) {
            continue;
        }
        if (!dil_parse_escaped(context)) {
            dil_parse__error(context, "Expected `Escaped` in `Set`!");
            dil_parse__return(false);
        }
    }

    if (!dil_string_prefix_element(context->string, '\'')) {
        dil_parse__error(context, "Expected `'` in `Set`!");
        dil_parse__return(false);
    }

    dil_parse__return(true);
}

/* Try to parse a not set. */
bool dil_parse_not_set(DilParseContext* context)
{
    dil_parse__create(DIL_SYMBOL_NOT_SET, true);

    if (!dil_string_prefix_element(context->string, '!')) {
        dil_parse__return(false);
    }

    if (!dil_parse_set(context)) {
        dil_parse__error(context, "Expected `Set` in `NotSet`!");
        dil_parse__return(false);
    }

    dil_parse__return(true);
}

/* Try to parse a string. */
bool dil_parse_string(DilParseContext* context)
{
    dil_parse__create(DIL_SYMBOL_STRING, true);

    DilString const SET_0 = dil_string_terminated("0123456789abcdefABCDEF");
    DilString const SET_1 = dil_string_terminated("tn\\\"");
    DilString const SET_2 = dil_string_terminated("\\\"");

    if (!dil_string_prefix_element(context->string, '"')) {
        dil_parse__return(false);
    }

    while (dil_string_finite(context->string)) {
        if (dil_string_prefix_element(context->string, '\\')) {
            if (dil_string_prefix_set(context->string, &SET_0)) {
                for (size_t i = 0; i < 2 - 1; i++) {
                    if (!dil_string_prefix_set(context->string, &SET_0)) {
                        dil_parse__error_set(context, &SET_0);
                        dil_parse__return(false);
                    }
                }
                continue;
            }
            if (dil_string_prefix_set(context->string, &SET_1)) {
                continue;
            }
            dil_parse__error(context, "Unexpected character in `String`!");
            dil_parse__return(false);
        }
        if (dil_string_prefix_not_set(context->string, &SET_2)) {
            continue;
        }
        break;
    }

    if (!dil_string_prefix_element(context->string, '"')) {
        dil_parse__error(context, "Expected `\"` in `String`!");
        dil_parse__return(false);
    }

    dil_parse__return(true);
}

/* Try to parse a reference. */
bool dil_parse_reference(DilParseContext* context)
{
    return dil_parse_identifier(context);
}

bool dil_parse_pattern(DilParseContext* context);

/* Try to parse a group. */
bool dil_parse_group(DilParseContext* context)
{
    dil_parse__create(DIL_SYMBOL_GROUP, false);

    if (!dil_string_prefix_element(context->string, '(')) {
        dil_parse__return(false);
    }

    dil_parse__skip(context);

    if (!dil_parse_pattern(context)) {
        dil_parse__error(context, "Expected `Pattern` in `Group`!");
        dil_parse__return(false);
    }

    dil_parse__skip(context);

    while (dil_parse_pattern(context)) {
        dil_parse__skip(context);
    }

    if (!dil_string_prefix_element(context->string, ')')) {
        dil_parse__error(context, "Expected `)` in `Group`!");
        dil_parse__return(false);
    }

    dil_parse__return(true);
}

bool dil_parse_alternative(DilParseContext* context);

/* Try to parse a fixed times. */
bool dil_parse_fixed_times(DilParseContext* context)
{
    dil_parse__create(DIL_SYMBOL_FIXED_TIMES, false);

    if (!dil_parse_number(context)) {
        dil_parse__return(false);
    }

    dil_parse__skip(context);

    if (!dil_parse_alternative(context)) {
        dil_parse__error(context, "Expected `Alternative` in `FixedTimes`!");
        dil_parse__return(false);
    }

    dil_parse__return(true);
}

/* Try to parse a one or more. */
bool dil_parse_one_or_more(DilParseContext* context)
{
    dil_parse__create(DIL_SYMBOL_ONE_OR_MORE, false);

    if (!dil_string_prefix_element(context->string, '+')) {
        dil_parse__return(false);
    }

    dil_parse__skip(context);

    if (!dil_parse_alternative(context)) {
        dil_parse__error(context, "Expected `Alternative` in `OneOrMore`!");
        dil_parse__return(false);
    }

    dil_parse__return(true);
}

/* Try to parse a zero or more. */
bool dil_parse_zero_or_more(DilParseContext* context)
{
    dil_parse__create(DIL_SYMBOL_ZERO_OR_MORE, false);

    if (!dil_string_prefix_element(context->string, '*')) {
        dil_parse__return(false);
    }

    dil_parse__skip(context);

    if (!dil_parse_alternative(context)) {
        dil_parse__error(context, "Expected `Alternative` in `ZeroOrMore`!");
        dil_parse__return(false);
    }

    dil_parse__return(true);
}

/* Try to parse a optional. */
bool dil_parse_optional(DilParseContext* context)
{
    dil_parse__create(DIL_SYMBOL_OPTIONAL, false);

    if (!dil_string_prefix_element(context->string, '?')) {
        dil_parse__return(false);
    }

    dil_parse__skip(context);

    if (!dil_parse_alternative(context)) {
        dil_parse__error(context, "Expected `Alternative` in `Optional`!");
        dil_parse__return(false);
    }

    dil_parse__return(true);
}

/* Try to parse alternatives. */
bool dil_parse_alternative(DilParseContext* context)
{
    return dil_parse_set(context) || dil_parse_not_set(context) ||
           dil_parse_string(context) || dil_parse_reference(context) ||
           dil_parse_group(context) || dil_parse_fixed_times(context) ||
           dil_parse_one_or_more(context) || dil_parse_zero_or_more(context) ||
           dil_parse_optional(context);
}

/* Try to parse a justaposition. */
bool dil_parse_justaposition(DilParseContext* context)
{
    dil_parse__create(DIL_SYMBOL_JUSTAPOSITION, false);

    if (!dil_parse_alternative(context)) {
        dil_parse__return(false);
    }

    dil_parse__skip(context);

    while (dil_parse_alternative(context)) {
        dil_parse__skip(context);
    }

    dil_parse__return(true);
}

/* Try to parse a pattern. */
bool dil_parse_pattern(DilParseContext* context)
{
    dil_parse__create(DIL_SYMBOL_PATTERN, false);

    if (!dil_parse_justaposition(context)) {
        dil_parse__return(false);
    }

    dil_parse__skip(context);

    while (dil_string_prefix_element(context->string, '|')) {
        dil_parse__skip(context);

        if (!dil_parse_justaposition(context)) {
            dil_parse__error(context, "Expected `Justaposition` in `Pattern`!");
            dil_parse__return(false);
        }

        dil_parse__skip(context);
    }

    dil_parse__return(true);
}

/* Try to parse a rule. */
bool dil_parse_rule(DilParseContext* context)
{
    dil_parse__create(DIL_SYMBOL_RULE, false);

    if (!dil_parse_identifier(context)) {
        dil_parse__return(false);
    }

    dil_parse__skip(context);

    if (!dil_string_prefix_element(context->string, '=')) {
        dil_parse__error(context, "Expected `=` in `Rule`!");
        dil_parse__return(false);
    }

    dil_parse__skip(context);

    if (!dil_parse_pattern(context)) {
        dil_parse__error(context, "Expected `Pattern` in `Rule`!");
        dil_parse__return(false);
    }

    dil_parse__skip(context);

    if (!dil_string_prefix_element(context->string, ';')) {
        dil_parse__error(context, "Expected `;` in `Rule`!");
        dil_parse__return(false);
    }

    dil_parse__return(true);
}

/* Try to parse a terminal. */
bool dil_parse_terminal(DilParseContext* context)
{
    dil_parse__create(DIL_SYMBOL_TERMINAL, false);

    DilString const TERMINALS_0 = dil_string_terminated("terminal");

    if (!dil_string_prefix_check(context->string, &TERMINALS_0)) {
        dil_parse__return(false);
    }

    dil_parse__skip(context);

    if (!dil_string_prefix_element(context->string, ';')) {
        dil_parse__error(context, "Expected `;` in `Skip`!");
        dil_parse__return(false);
    }

    dil_parse__return(true);
}

/* Try to parse a skip. */
bool dil_parse_skip(DilParseContext* context)
{
    dil_parse__create(DIL_SYMBOL_SKIP, false);

    DilString const TERMINALS_0 = dil_string_terminated("skip");

    if (!dil_string_prefix_check(context->string, &TERMINALS_0)) {
        dil_parse__return(false);
    }

    dil_parse__skip(context);

    if (dil_parse_pattern(context)) {
        dil_parse__skip(context);
    }

    if (!dil_string_prefix_element(context->string, ';')) {
        dil_parse__error(context, "Expected `;` in `Skip`!");
        dil_parse__return(false);
    }

    dil_parse__return(true);
}

/* Try to parse a start. */
bool dil_parse_start(DilParseContext* context)
{
    dil_parse__create(DIL_SYMBOL_START, false);

    DilString const TERMINALS_0 = dil_string_terminated("start");

    if (!dil_string_prefix_check(context->string, &TERMINALS_0)) {
        dil_parse__return(false);
    }

    dil_parse__skip(context);

    if (!dil_parse_pattern(context)) {
        dil_parse__error(context, "Expected `Pattern` in `Start`!");
        dil_parse__return(false);
    }

    dil_parse__skip(context);

    if (!dil_string_prefix_element(context->string, ';')) {
        dil_parse__error(context, "Expected `;` in `Start`!");
        dil_parse__return(false);
    }

    dil_parse__return(true);
}

/* Try to parse an output. */
bool dil_parse_output(DilParseContext* context)
{
    dil_parse__create(DIL_SYMBOL_OUTPUT, false);

    DilString const TERMINALS_0 = dil_string_terminated("output");

    if (!dil_string_prefix_check(context->string, &TERMINALS_0)) {
        dil_parse__return(false);
    }

    dil_parse__skip(context);

    if (!dil_parse_string(context)) {
        dil_parse__error(context, "Expected `String` in `Output`!");
        dil_parse__return(false);
    }

    dil_parse__skip(context);

    if (!dil_string_prefix_element(context->string, ';')) {
        dil_parse__error(context, "Expected `;` in `Output`!");
        dil_parse__return(false);
    }

    dil_parse__return(true);
}

/* Try to parse a statement. */
bool dil_parse_statement(DilParseContext* context)
{
    return dil_parse_output(context) || dil_parse_start(context) ||
           dil_parse_skip(context) || dil_parse_terminal(context) ||
           dil_parse_rule(context);
}

/* Parses the start symbol. */
void dil_parse(DilBuilder* builder, DilSource* source)
{
    DilString string = source->contents;

    size_t index = dil_tree_size(builder->built);
    dil_tree_add(
        builder->built,
        (DilNode){
            .object = {
                       .symbol = DIL_SYMBOL__START,
                       .value  = {.first = string.first}}
    });
    dil_builder_push(builder);

    DilParseContext context = {
        .builder = builder,
        .string  = &string,
        .source  = source};

    dil_parse__skip(&context);
    while (dil_parse_statement(&context)) {
        dil_parse__skip(&context);
    }

    dil_builder_pop(builder);
    dil_tree_at(builder->built, index)->object.value.last = string.first;

    if (dil_string_finite(&string)) {
        dil_parse__error(&context, "Unexpected characters in the file!");
    }

    if (source->error != 0) {
        printf(
            "%s: error: File had %llu errors.\n",
            source->path,
            source->error);
    }
}
