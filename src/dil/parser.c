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
    /* Tree that is built. */
    DilTree built;
    /* Builder to parse into. */
    DilBuilder builder;
    /* Remaining source file contents. */
    DilString remaining;
    /* Parsed source file. */
    DilSource* source;
    /* Whether the parser is in skip mode. */
    bool skip;
} DilParseContext;

/* Create an object in the tree. */
void dil_parse__create(DilParseContext* context, DilSymbol symbol)
{
    dil_builder_add(
        &context->builder,
        (DilObject){
            .symbol = symbol,
            .value  = {.first = context->remaining.first}});
    dil_builder_push(&context->builder);
}

/* End an object or remove it from the tree. */
bool dil_parse__return(DilParseContext* context, bool accept)
{
    if (context->skip) {
        if (!accept) {
            context->remaining.first =
                dil_builder_parent(&context->builder)->object.value.first;
        }
        dil_builder_remove(&context->builder);
        dil_builder_parent(&context->builder)->childeren--;
        return accept;
    }
    if (accept) {
        dil_builder_parent(&context->builder)->object.value.last =
            context->remaining.first;
        dil_builder_pop(&context->builder);
        return true;
    }
    context->remaining.first =
        dil_builder_parent(&context->builder)->object.value.first;
    dil_builder_remove(&context->builder);
    dil_builder_parent(&context->builder)->childeren--;
    return false;
}

bool dil_parse_comment(DilParseContext* context);
bool dil_parse_whitespace(DilParseContext* context);
bool dil_parse_identifier(DilParseContext* context);
bool dil_parse_escaped(DilParseContext* context);
bool dil_parse_number(DilParseContext* context);
bool dil_parse_set(DilParseContext* context);
bool dil_parse_not_set(DilParseContext* context);
bool dil_parse_string(DilParseContext* context);
bool dil_parse_reference(DilParseContext* context);
bool dil_parse_group(DilParseContext* context);
bool dil_parse_fixed_times(DilParseContext* context);
bool dil_parse_one_or_more(DilParseContext* context);
bool dil_parse_zero_or_more(DilParseContext* context);
bool dil_parse_optional(DilParseContext* context);
bool dil_parse_unit(DilParseContext* context);
bool dil_parse_alternative(DilParseContext* context);
bool dil_parse_pattern(DilParseContext* context);
bool dil_parse_rule(DilParseContext* context);
bool dil_parse_start(DilParseContext* context);
bool dil_parse_skip(DilParseContext* context);
bool dil_parse_statement(DilParseContext* context);

/* Try to skip in style 0 once. */
bool dil_parse__skip_0_once(DilParseContext* context)
{
    context->skip = true;
    bool accept   = dil_parse_whitespace(context) || dil_parse_comment(context);
    context->skip = false;
    return accept;
}

/* Skip in style 0 as much as possible. */
void dil_parse__skip_0(DilParseContext* context)
{
    while (dil_parse__skip_0_once(context)) {}
}

/* Try to parse a character. */
bool dil_parse__character(DilParseContext* context, char element)
{
    dil_parse__create(context, DIL_SYMBOL__CHARACTER);
    return dil_parse__return(
        context,
        dil_string_prefix_element(&context->remaining, element));
}

/* Try to parse a character from a set. */
bool dil_parse__set(DilParseContext* context, DilString const* set)
{
    dil_parse__create(context, DIL_SYMBOL__CHARACTER);
    return dil_parse__return(
        context,
        dil_string_prefix_set(&context->remaining, set));
}

/* Try to parse a character from a not set. */
bool dil_parse__not_set(DilParseContext* context, DilString const* set)
{
    dil_parse__create(context, DIL_SYMBOL__CHARACTER);
    return dil_parse__return(
        context,
        dil_string_prefix_not_set(&context->remaining, set));
}

/* Try to parse a string. */
bool dil_parse__string(DilParseContext* context, DilString const* set)
{
    dil_parse__create(context, DIL_SYMBOL__STRING);
    return dil_parse__return(
        context,
        dil_string_prefix_check(&context->remaining, set));
}

/* Skip over the erronous characters and print them. */
void dil_parse__error_skip(
    DilParseContext* context,
    bool (*skip)(DilParseContext*),
    char const* expected,
    char const* symbol)
{
    size_t const BUFFER_SIZE = 1024;
    char         buffer[BUFFER_SIZE];
    (void)sprintf_s(
        buffer,
        BUFFER_SIZE,
        "Expected `%s` in `%s`!",
        expected,
        symbol);
    DilString portion = {
        .first = context->remaining.first,
        .last  = context->remaining.first};
    while (context->remaining.first <= context->remaining.last &&
           !skip(context)) {
        context->remaining.first++;
        portion.last++;
    }
    dil_source_error(context->source, &portion, buffer);
}

/* Print the expected character. */
void dil_parse__error_character(
    DilParseContext* context,
    char             character,
    char const*      symbol)
{
    size_t const BUFFER_SIZE = 1024;
    char         buffer[BUFFER_SIZE];
    (void)sprintf_s(
        buffer,
        BUFFER_SIZE,
        "Expected `%c` in `%s`!",
        character,
        symbol);
    DilString portion = {
        .first = context->remaining.first,
        .last  = context->remaining.first + 1};
    dil_source_error(context->source, &portion, buffer);
}

/* Print the expected set. */
void dil_parse__error_set(
    DilParseContext* context,
    DilString const* set,
    char const*      symbol)
{
    size_t const BUFFER_SIZE = 1024;
    char         buffer[BUFFER_SIZE];
    (void)sprintf_s(
        buffer,
        BUFFER_SIZE,
        "Expected one of `%.*s` in `%s`!",
        (int)dil_string_size(set),
        set->first,
        symbol);
    DilString portion = {
        .first = context->remaining.first,
        .last  = context->remaining.first + 1};
    dil_source_error(context->source, &portion, buffer);
}

/* Print the expected not set. */
void dil_parse__error_not_set(
    DilParseContext* context,
    DilString const* set,
    char const*      symbol)
{
    size_t const BUFFER_SIZE = 1024;
    char         buffer[BUFFER_SIZE];
    (void)sprintf_s(
        buffer,
        BUFFER_SIZE,
        "Expected none of `%.*s` in `%s`!",
        (int)dil_string_size(set),
        set->first,
        symbol);
    DilString portion = {
        .first = context->remaining.first,
        .last  = context->remaining.first + 1};
    dil_source_error(context->source, &portion, buffer);
}

/* Print the expected string. */
void dil_parse__error_string(
    DilParseContext* context,
    DilString const* string,
    char const*      symbol)
{
    size_t const BUFFER_SIZE = 1024;
    char         buffer[BUFFER_SIZE];
    (void)sprintf_s(
        buffer,
        BUFFER_SIZE,
        "Expected `%.*s` in `%s`!",
        (int)dil_string_size(string),
        string->first,
        symbol);
    DilString portion = {
        .first = context->remaining.first,
        .last  = context->remaining.first + 1};
    dil_source_error(context->source, &portion, buffer);
}

/* Print the expected terminal. */
void dil_parse__error_reference(
    DilParseContext* context,
    char const*      expected,
    char const*      symbol)
{
    size_t const BUFFER_SIZE = 1024;
    char         buffer[BUFFER_SIZE];
    (void)sprintf_s(
        buffer,
        BUFFER_SIZE,
        "Expected `%s` in `%s`!",
        expected,
        symbol);
    DilString portion = {
        .first = context->remaining.first,
        .last  = context->remaining.first + 1};
    dil_source_error(context->source, &portion, buffer);
}

/* Print the unexpected character. */
void dil_parse__error_unexpected(DilParseContext* context, char const* symbol)
{
    size_t const BUFFER_SIZE = 1024;
    char         buffer[BUFFER_SIZE];
    (void)
        sprintf_s(buffer, BUFFER_SIZE, "Unexpected character in `%s`!", symbol);
    DilString portion = {
        .first = context->remaining.first,
        .last  = context->remaining.first + 1};
    dil_source_error(context->source, &portion, buffer);
}

/* Try to parse a comment. */
bool dil_parse_comment(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_COMMENT);

    DilString const TERMINALS_0 = dil_string_terminated("//");
    DilString const SET_0       = dil_string_terminated("\n");
    char const      CHARACTER_0 = '\n';

    if (!dil_parse__string(context, &TERMINALS_0)) {
        return dil_parse__return(context, false);
    }

    while (dil_parse__not_set(context, &SET_0)) {}

    if (!dil_parse__character(context, CHARACTER_0)) {
        dil_parse__error_character(context, CHARACTER_0, "Comment");
        return dil_parse__return(context, true);
    }

    return dil_parse__return(context, true);
}

/* Try to parse a whitespace. */
bool dil_parse_whitespace(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_WHITESPACE);

    DilString const SET_0 = dil_string_terminated("\t\n ");

    if (!dil_parse__set(context, &SET_0)) {
        return dil_parse__return(context, false);
    }

    return dil_parse__return(context, true);
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
                    dil_parse__error_set(context, &SET_0, "Escaped");
                    return dil_parse__return(context, true);
                }
            }
            return dil_parse__return(context, true);
        }

        if (dil_parse__set(context, &SET_1)) {
            return dil_parse__return(context, true);
        }

        dil_parse__error_unexpected(context, "Escaped");
        return dil_parse__return(context, true);
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
            dil_parse__error_reference(context, "Escaped", "Set");
            return dil_parse__return(context, true);
        }
    }

    if (!dil_parse__character(context, '\'')) {
        dil_parse__error_character(context, '\'', "Set");
        return dil_parse__return(context, true);
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
        dil_parse__error_reference(context, "Set", "NotSet");
        return dil_parse__return(context, true);
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

    while (dil_string_finite(&context->remaining)) {
        if (dil_parse__character(context, '\\')) {
            if (dil_parse__set(context, &SET_0)) {
                for (size_t i = 0; i < 2 - 1; i++) {
                    if (!dil_parse__set(context, &SET_0)) {
                        dil_parse__error_set(context, &SET_0, "String");
                        return dil_parse__return(context, true);
                    }
                }
                continue;
            }
            if (dil_parse__set(context, &SET_1)) {
                continue;
            }
            dil_parse__error_unexpected(context, "String");
            return dil_parse__return(context, true);
        }
        if (dil_parse__not_set(context, &SET_2)) {
            continue;
        }
        break;
    }

    if (!dil_parse__character(context, '"')) {
        dil_parse__error_character(context, '"', "String");
        return dil_parse__return(context, true);
    }

    return dil_parse__return(context, true);
}

/* Try to parse a reference. */
bool dil_parse_reference(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_REFERENCE);
    return dil_parse__return(context, dil_parse_identifier(context));
}

/* Try to parse a group. */
bool dil_parse_group(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_GROUP);

    if (!dil_parse__character(context, '(')) {
        return dil_parse__return(context, false);
    }

    dil_parse__skip_0(context);

    if (!dil_parse_pattern(context)) {
        dil_parse__error_skip(
            context,
            &dil_parse__skip_0_once,
            "Pattern",
            "Group");
        return dil_parse__return(context, true);
    }

    dil_parse__skip_0(context);

    while (dil_parse_pattern(context)) {
        dil_parse__skip_0(context);
    }

    if (!dil_parse__character(context, ')')) {
        dil_parse__error_character(context, ')', "Group");
        return dil_parse__return(context, true);
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

    dil_parse__skip_0(context);

    if (!dil_parse_unit(context)) {
        dil_parse__error_skip(
            context,
            &dil_parse__skip_0_once,
            "Unit",
            "FixedTimes");
        return dil_parse__return(context, true);
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

    dil_parse__skip_0(context);

    if (!dil_parse_unit(context)) {
        dil_parse__error_skip(
            context,
            &dil_parse__skip_0_once,
            "Unit",
            "OneOrMore");
        return dil_parse__return(context, true);
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

    dil_parse__skip_0(context);

    if (!dil_parse_unit(context)) {
        dil_parse__error_skip(
            context,
            &dil_parse__skip_0_once,
            "Unit",
            "ZeroOrMore");
        return dil_parse__return(context, true);
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

    dil_parse__skip_0(context);

    if (!dil_parse_unit(context)) {
        dil_parse__error_skip(
            context,
            &dil_parse__skip_0_once,
            "Unit",
            "Optional");
        return dil_parse__return(context, true);
    }

    return dil_parse__return(context, true);
}

/* Try to parse a unit. */
bool dil_parse_unit(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_UNIT);
    return dil_parse__return(
        context,
        dil_parse_set(context) || dil_parse_not_set(context) ||
            dil_parse_string(context) || dil_parse_reference(context) ||
            dil_parse_group(context) || dil_parse_fixed_times(context) ||
            dil_parse_one_or_more(context) || dil_parse_zero_or_more(context) ||
            dil_parse_optional(context));
}

/* Try to parse alternatives. */
bool dil_parse_alternative(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_ALTERNATIVE);

    if (!dil_parse_unit(context)) {
        return dil_parse__return(context, false);
    }

    dil_parse__skip_0(context);

    while (dil_parse_unit(context)) {
        dil_parse__skip_0(context);
    }

    return dil_parse__return(context, true);
}

/* Try to parse a pattern. */
bool dil_parse_pattern(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_PATTERN);

    if (!dil_parse_alternative(context)) {
        return dil_parse__return(context, false);
    }

    dil_parse__skip_0(context);

    while (dil_parse__character(context, '|')) {
        dil_parse__skip_0(context);

        if (!dil_parse_alternative(context)) {
            dil_parse__error_skip(
                context,
                &dil_parse__skip_0_once,
                "Alternative",
                "Pattern");
            return dil_parse__return(context, true);
        }

        dil_parse__skip_0(context);
    }

    return dil_parse__return(context, true);
}

/* Try to parse a rule. */
bool dil_parse_rule(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_RULE);

    if (!dil_parse_identifier(context)) {
        return dil_parse__return(context, false);
    }

    dil_parse__skip_0(context);

    if (!dil_parse__character(context, '=')) {
        dil_parse__error_character(context, '=', "Rule");
        return dil_parse__return(context, true);
    }

    dil_parse__skip_0(context);

    if (!dil_parse_pattern(context)) {
        dil_parse__error_skip(
            context,
            &dil_parse__skip_0_once,
            "Pattern",
            "Rule");
        return dil_parse__return(context, true);
    }

    dil_parse__skip_0(context);

    if (!dil_parse__character(context, ';')) {
        dil_parse__error_character(context, ';', "Rule");
        return dil_parse__return(context, true);
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

    dil_parse__skip_0(context);

    if (!dil_parse_pattern(context)) {
        dil_parse__error_skip(
            context,
            &dil_parse__skip_0_once,
            "Pattern",
            "Start");
        return dil_parse__return(context, true);
    }

    dil_parse__skip_0(context);

    if (!dil_parse__character(context, ';')) {
        dil_parse__error_character(context, ';', "Start");
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

    dil_parse__skip_0(context);

    if (dil_parse_pattern(context)) {
        dil_parse__skip_0(context);
    }

    if (!dil_parse__character(context, ';')) {
        dil_parse__error_character(context, ';', "Skip");
        return dil_parse__return(context, true);
    }

    return dil_parse__return(context, true);
}

/* Try to parse a statement. */
bool dil_parse_statement(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_STATEMENT);
    return dil_parse__return(
        context,
        dil_parse_skip(context) || dil_parse_start(context) ||
            dil_parse_rule(context));
}

/* Parses the __start__ symbol. */
void dil_parse__start(DilParseContext* context)
{
    dil_tree_add(
        &context->built,
        (DilNode){
            .object = {
                       .symbol = DIL_SYMBOL__START,
                       .value  = {.first = context->remaining.first}}
    });
    dil_builder_push(&context->builder);

    dil_parse__skip_0(context);
    while (dil_parse_statement(context)) {
        dil_parse__skip_0(context);
    }

    dil_builder_parent(&context->builder)->object.value.last =
        context->remaining.first;
    dil_builder_pop(&context->builder);

    if (dil_string_finite(&context->remaining)) {
        dil_source_error(
            context->source,
            &context->remaining,
            "There are unexpected characters left in the file!");
    }
}

/* Parses the source file. */
DilTree dil_parse(DilSource* source)
{
    DilParseContext initial = {
        .builder   = {.built = &initial.built},
        .remaining = source->contents,
        .source    = source};

    dil_parse__start(&initial);

    dil_builder_free(&initial.builder);
    return initial.built;
}
