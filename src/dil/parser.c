// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/buffer.c"
#include "dil/builder.c"
#include "dil/object.c"
#include "dil/parsecontext.c"
#include "dil/source.c"
#include "dil/string.c"
#include "dil/tree.c"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/* Create an object in the tree. */
void dil_parse__create(DilParseContext* context, DilSymbol symbol)
{
    dil_builder_add(
        context->builder,
        (DilObject){
            .symbol = symbol,
            .value  = {.first = context->string->first}});
    dil_builder_push(context->builder);
}

/* End an object or remove it from the tree. */
bool dil_parse__return(DilParseContext* context, bool accept)
{
    if (accept) {
        dil_builder_parent(context->builder)->object.value.last =
            context->string->first;
        dil_builder_pop(context->builder);
        return true;
    }
    context->string->first =
        dil_builder_parent(context->builder)->object.value.first;
    dil_builder_remove(context->builder);
    dil_builder_parent(context->builder)->childeren--;
    return false;
}

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

/* Try to parse a character. */
bool dil_parse__character(DilParseContext* context, char element)
{
    dil_parse__create(context, DIL_SYMBOL__CHARACTER);
    return dil_parse__return(
        context,
        dil_string_prefix_element(context->string, element));
}

/* Try to parse a character from a set. */
bool dil_parse__set(DilParseContext* context, DilString const* set)
{
    dil_parse__create(context, DIL_SYMBOL__CHARACTER);
    return dil_parse__return(
        context,
        dil_string_prefix_set(context->string, set));
}

/* Try to parse a character from a not set. */
bool dil_parse__not_set(DilParseContext* context, DilString const* set)
{
    dil_parse__create(context, DIL_SYMBOL__CHARACTER);
    return dil_parse__return(
        context,
        dil_string_prefix_not_set(context->string, set));
}

/* Try to parse a string. */
bool dil_parse__string(DilParseContext* context, DilString const* set)
{
    dil_parse__create(context, DIL_SYMBOL__STRING);
    return dil_parse__return(
        context,
        dil_string_prefix_check(context->string, set));
}

/* Try to parse an identifier. */
bool dil_parse_identifier(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_IDENTIFIER);

    DilString const SET_0 = dil_string_terminated("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    DilString const SET_1 = dil_string_terminated(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

    if (!dil_parse__set(context, &SET_0)) {
        return dil_parse__return(context, false);
    }

    while (dil_parse__set(context, &SET_1)) {}

    return dil_parse__return(context, true);
}

/* Try to parse an escaped character. */
bool dil_parse_escaped(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_ESCAPED);

    DilString const SET_0 = dil_string_terminated("0123456789abcdefABCDEF");
    DilString const SET_1 = dil_string_terminated("tn\\'~");
    DilString const SET_2 = dil_string_terminated("\\'~");

    if (dil_parse__character(context, '\\')) {
        if (dil_parse__set(context, &SET_0)) {
            for (size_t i = 0; i < 2 - 1; i++) {
                if (!dil_parse__set(context, &SET_0)) {
                    dil_parse__error_set(context, &SET_0);
                    return dil_parse__return(context, false);
                }
            }
            return dil_parse__return(context, true);
        }

        if (dil_parse__set(context, &SET_1)) {
            return dil_parse__return(context, true);
        }

        dil_parse__error(context, "Unexpected character in `Escaped`!");
        return dil_parse__return(context, false);
    }

    if (dil_parse__not_set(context, &SET_2)) {
        return dil_parse__return(context, true);
    }

    return dil_parse__return(context, false);
}

/* Try to parse a number. */
bool dil_parse_number(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_NUMBER);

    DilString const SET_0 = dil_string_terminated("123456789");
    DilString const SET_1 = dil_string_terminated("0123456789");

    if (!dil_parse__set(context, &SET_0)) {
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    while (dil_parse__set(context, &SET_1)) {}

    return dil_parse__return(context, true);
}

/* Try to parse a set. */
bool dil_parse_set(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_SET);

    if (!dil_parse__character(context, '\'')) {
        return dil_parse__return(context, false);
    }

    while (dil_parse_escaped(context)) {
        if (!dil_parse__character(context, '~')) {
            continue;
        }
        if (!dil_parse_escaped(context)) {
            dil_parse__error(context, "Expected `Escaped` in `Set`!");
            return dil_parse__return(context, false);
        }
    }

    if (!dil_parse__character(context, '\'')) {
        dil_parse__error(context, "Expected `'` in `Set`!");
        return dil_parse__return(context, false);
    }

    return dil_parse__return(context, true);
}

/* Try to parse a not set. */
bool dil_parse_not_set(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_NOT_SET);

    if (!dil_parse__character(context, '!')) {
        return dil_parse__return(context, false);
    }

    if (!dil_parse_set(context)) {
        dil_parse__error(context, "Expected `Set` in `NotSet`!");
        return dil_parse__return(context, false);
    }

    return dil_parse__return(context, true);
}

/* Try to parse a string. */
bool dil_parse_string(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_STRING);

    DilString const SET_0 = dil_string_terminated("0123456789abcdefABCDEF");
    DilString const SET_1 = dil_string_terminated("tn\\\"");
    DilString const SET_2 = dil_string_terminated("\\\"");

    if (!dil_parse__character(context, '"')) {
        return dil_parse__return(context, false);
    }

    while (dil_string_finite(context->string)) {
        if (dil_parse__character(context, '\\')) {
            if (dil_parse__set(context, &SET_0)) {
                for (size_t i = 0; i < 2 - 1; i++) {
                    if (!dil_parse__set(context, &SET_0)) {
                        dil_parse__error_set(context, &SET_0);
                        return dil_parse__return(context, false);
                    }
                }
                continue;
            }
            if (dil_parse__set(context, &SET_1)) {
                continue;
            }
            dil_parse__error(context, "Unexpected character in `String`!");
            return dil_parse__return(context, false);
        }
        if (dil_parse__not_set(context, &SET_2)) {
            continue;
        }
        break;
    }

    if (!dil_parse__character(context, '"')) {
        dil_parse__error(context, "Expected `\"` in `String`!");
        return dil_parse__return(context, false);
    }

    return dil_parse__return(context, true);
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
    dil_parse__create(context, DIL_SYMBOL_GROUP);

    if (!dil_parse__character(context, '(')) {
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    if (!dil_parse_pattern(context)) {
        dil_parse__error(context, "Expected `Pattern` in `Group`!");
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    while (dil_parse_pattern(context)) {
        dil_parse__skip(context);
    }

    if (!dil_parse__character(context, ')')) {
        dil_parse__error(context, "Expected `)` in `Group`!");
        return dil_parse__return(context, false);
    }

    return dil_parse__return(context, true);
}

/* Try to parse a fixed times. */
bool dil_parse_fixed_times(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_FIXED_TIMES);

    if (!dil_parse_number(context)) {
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    if (!dil_parse_pattern(context)) {
        dil_parse__error(context, "Expected `Pattern` in `FixedTimes`!");
        return dil_parse__return(context, false);
    }

    return dil_parse__return(context, true);
}

/* Try to parse a one or more. */
bool dil_parse_one_or_more(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_ONE_OR_MORE);

    if (!dil_parse__character(context, '+')) {
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    if (!dil_parse_pattern(context)) {
        dil_parse__error(context, "Expected `Pattern` in `OneOrMore`!");
        return dil_parse__return(context, false);
    }

    return dil_parse__return(context, true);
}

/* Try to parse a zero or more. */
bool dil_parse_zero_or_more(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_ZERO_OR_MORE);

    if (!dil_parse__character(context, '*')) {
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    if (!dil_parse_pattern(context)) {
        dil_parse__error(context, "Expected `Pattern` in `ZeroOrMore`!");
        return dil_parse__return(context, false);
    }

    return dil_parse__return(context, true);
}

/* Try to parse a optional. */
bool dil_parse_optional(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_OPTIONAL);

    if (!dil_parse__character(context, '?')) {
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    if (!dil_parse_pattern(context)) {
        dil_parse__error(context, "Expected `Pattern` in `Optional`!");
        return dil_parse__return(context, false);
    }

    return dil_parse__return(context, true);
}

/* Try to parse a justaposition. */
bool dil_parse_justaposition(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_JUSTAPOSITION);

    if (!dil_parse_pattern(context)) {
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    while (dil_parse_pattern(context)) {
        dil_parse__skip(context);
    }

    return dil_parse__return(context, true);
}

/* Try to parse alternatives. */
bool dil_parse_alternative(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_ALTERNATIVE);

    if (!dil_parse_pattern(context)) {
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    if (!dil_parse__character(context, '|')) {
        dil_parse__error(context, "Expected `|` in `Alternative`!");
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    if (!dil_parse_pattern(context)) {
        dil_parse__error(context, "Expected `Pattern` in `Alternative`!");
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    while (dil_parse__character(context, '|')) {
        dil_parse__skip(context);

        if (!dil_parse_pattern(context)) {
            dil_parse__error(context, "Expected `Pattern` in `Alternative`!");
            return dil_parse__return(context, false);
        }

        dil_parse__skip(context);
    }

    return dil_parse__return(context, true);
}

/* Try to parse a pattern. */
bool dil_parse_pattern(DilParseContext* context)
{
    return dil_parse_set(context) || dil_parse_not_set(context) ||
           dil_parse_string(context) || dil_parse_reference(context) ||
           dil_parse_group(context) || dil_parse_fixed_times(context) ||
           dil_parse_one_or_more(context) || dil_parse_zero_or_more(context) ||
           dil_parse_optional(context) || dil_parse_justaposition(context) ||
           dil_parse_alternative(context);
}

/* Try to parse a rule. */
bool dil_parse_rule(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_RULE);

    if (!dil_parse_identifier(context)) {
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    if (!dil_parse__character(context, '=')) {
        dil_parse__error(context, "Expected `=` in `Rule`!");
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    if (!dil_parse_pattern(context)) {
        dil_parse__error(context, "Expected `Pattern` in `Rule`!");
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    if (!dil_parse__character(context, ';')) {
        dil_parse__error(context, "Expected `;` in `Rule`!");
        return dil_parse__return(context, false);
    }

    return dil_parse__return(context, true);
}

/* Try to parse a terminal. */
bool dil_parse_terminal(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_TERMINAL);

    DilString const TERMINALS_0 = dil_string_terminated("terminal");

    if (!dil_parse__string(context, &TERMINALS_0)) {
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    if (!dil_parse__character(context, ';')) {
        dil_parse__error(context, "Expected `;` in `Skip`!");
        return dil_parse__return(context, false);
    }

    return dil_parse__return(context, true);
}

/* Try to parse a skip. */
bool dil_parse_skip(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_SKIP);

    DilString const TERMINALS_0 = dil_string_terminated("skip");

    if (!dil_parse__string(context, &TERMINALS_0)) {
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    if (dil_parse_pattern(context)) {
        dil_parse__skip(context);
    }

    if (!dil_parse__character(context, ';')) {
        dil_parse__error(context, "Expected `;` in `Skip`!");
        return dil_parse__return(context, false);
    }

    return dil_parse__return(context, true);
}

/* Try to parse a start. */
bool dil_parse_start(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_START);

    DilString const TERMINALS_0 = dil_string_terminated("start");

    if (!dil_parse__string(context, &TERMINALS_0)) {
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    if (!dil_parse_pattern(context)) {
        dil_parse__error(context, "Expected `Pattern` in `Start`!");
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    if (!dil_parse__character(context, ';')) {
        dil_parse__error(context, "Expected `;` in `Start`!");
        return dil_parse__return(context, false);
    }

    return dil_parse__return(context, true);
}

/* Try to parse an output. */
bool dil_parse_output(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_OUTPUT);

    DilString const TERMINALS_0 = dil_string_terminated("output");

    if (!dil_parse__string(context, &TERMINALS_0)) {
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    if (!dil_parse_string(context)) {
        dil_parse__error(context, "Expected `String` in `Output`!");
        return dil_parse__return(context, false);
    }

    dil_parse__skip(context);

    if (!dil_parse__character(context, ';')) {
        dil_parse__error(context, "Expected `;` in `Output`!");
        return dil_parse__return(context, false);
    }

    return dil_parse__return(context, true);
}

/* Try to parse a statement. */
bool dil_parse_statement(DilParseContext* context)
{
    return dil_parse_output(context) || dil_parse_start(context) ||
           dil_parse_skip(context) || dil_parse_terminal(context) ||
           dil_parse_rule(context);
}

/* Parses the __start__ symbol. */
bool dil_parse__start(DilParseContext* context)
{
    dil_tree_add(
        context->builder->built,
        (DilNode){
            .object = {
                       .symbol = DIL_SYMBOL__START,
                       .value  = {.first = context->string->first}}
    });
    dil_builder_push(context->builder);
    dil_parse__skip(context);
    while (dil_parse_statement(context)) {
        dil_parse__skip(context);
    }

    dil_builder_parent(context->builder)->object.value.last =
        context->string->first;
    dil_builder_pop(context->builder);

    if (dil_string_finite(context->string)) {
        dil_parse__error(context, "Unexpected characters in the file!");
        return false;
    }

    return true;
}

/* Parses the source file. */
void dil_parse(DilBuilder* builder, DilSource* source)
{
    DilString       string  = source->contents;
    DilParseContext context = {
        .builder = builder,
        .string  = &string,
        .source  = source};

    dil_parse__start(&context);

    if (source->error != 0) {
        printf(
            "%s: error: File had %llu errors.\n",
            source->path,
            source->error);
    }
}
