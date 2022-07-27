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

/* Macro for the start of a try-parse function. */
#define dil_parse__create(SYMBOL)                                            \
    size_t index = dil_tree_size(builder->built);                            \
    dil_builder_add(                                                         \
        builder,                                                             \
        (DilObject){.symbol = (SYMBOL), .value = {.first = string->first}}); \
    dil_builder_push(builder)

/* Macro for the return of a try-parse function. */
#define dil_parse__return(ACCEPT)                                              \
    dil_builder_pop(builder);                                                  \
    if ((ACCEPT)) {                                                            \
        dil_tree_at(builder->built, index)->object.value.last = string->first; \
        return true;                                                           \
    }                                                                          \
    dil_tree_remove(builder->built);                                           \
    dil_tree_at(builder->built, *dil_indices_finish(&builder->parents))        \
        ->childeren--;                                                         \
    return false;

/* Try to skip a comment. */
bool dil_parse__skip_comment(DilString* string)
{
    DilString const TERMINALS_0 = dil_string_terminated("//");
    DilString const SET_0       = dil_string_terminated("\n");

    if (!dil_string_prefix_check(string, &TERMINALS_0)) {
        return false;
    }

    while (dil_string_prefix_not_set(string, &SET_0)) {}

    return true;
}

/* Try to skip whitespace. */
bool dil_parse__skip_whitespace(DilString* string)
{
    DilString const SET_0 = dil_string_terminated("\t\n ");
    return dil_string_prefix_set(string, &SET_0);
}

/* Try to skip 0 once. */
bool dil_parse__skip_0_once(DilString* string)
{
    return dil_parse__skip_whitespace(string) ||
           dil_parse__skip_comment(string);
}

/* Skip 0 as much as possible. */
void dil_parse__skip_0(DilString* string)
{
    while (dil_parse__skip_0_once(string)) {}
}

/* Skip erronous characters and print them. */
void dil_parse__error(DilString* string, DilSource* source, char const* message)
{
    DilString portion = {.first = string->first, .last = string->first};
    while (string->first <= string->last && !dil_parse__skip_0_once(string)) {
        string->first++;
        portion.last++;
    }
    source->error++;
    dil_source_print(source, &portion, "error", message);
}

/* Print the expected set and skip the erronous characters. */
void dil_parse__error_set(
    DilString*       string,
    DilSource*       source,
    DilString const* set)
{
    size_t const BUFFER_SIZE = 1024;
    char         buffer[BUFFER_SIZE];
    (void)sprintf_s(
        buffer,
        BUFFER_SIZE,
        "Expected one of `%.*s` in `String`!",
        (int)dil_string_size(set),
        set->first);
    dil_parse__error(string, source, buffer);
}

/* Try to parse a specific terminal. */
bool dil_parse__terminal(DilBuilder* builder, DilString* string, char terminal)
{
    dil_parse__create(DIL_SYMBOL_TERMINAL);
    dil_parse__return(dil_string_prefix_element(string, terminal));
}

/* Try to parse a set terminal. */
bool dil_parse__terminal_set(
    DilBuilder*      builder,
    DilString*       string,
    DilString* const set)
{
    dil_parse__create(DIL_SYMBOL_TERMINAL);
    dil_parse__return(dil_string_prefix_set(string, set));
}

/* Try to parse a not set terminal. */
bool dil_parse__terminal_not_set(
    DilBuilder*      builder,
    DilString*       string,
    DilString* const set)
{
    dil_parse__create(DIL_SYMBOL_TERMINAL);
    dil_parse__return(dil_string_prefix_not_set(string, set));
}

/* Try to parse a string of terminals. */
bool dil_parse__terminal_string(
    DilBuilder*      builder,
    DilString*       string,
    DilString const* terminals)
{
    dil_parse__create(DIL_SYMBOL_TERMINAL);
    dil_parse__return(dil_string_prefix_check(string, terminals));
}

/* Try to parse an identifier. */
bool dil_parse_identifier(DilBuilder* builder, DilString* string)
{
    dil_parse__create(DIL_SYMBOL_IDENTIFIER);

    DilString const SET_0 = dil_string_terminated("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    DilString const SET_1 = dil_string_terminated(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

    if (!dil_string_prefix_set(string, &SET_0)) {
        dil_parse__return(false);
    }

    while (dil_string_prefix_set(string, &SET_1)) {}

    dil_parse__return(true);
}

/* Try to parse an escaped character. */
bool dil_parse_escaped(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    dil_parse__create(DIL_SYMBOL_ESCAPED);

    DilString const SET_0 = dil_string_terminated("0123456789abcdefABCDEF");
    DilString const SET_1 = dil_string_terminated("tn\\'~");
    DilString const SET_2 = dil_string_terminated("\\'~");

    if (dil_parse__terminal(builder, string, '\\')) {
        if (dil_string_prefix_set(string, &SET_0)) {
            for (size_t i = 0; i < 2 - 1; i++) {
                if (!dil_string_prefix_set(string, &SET_0)) {
                    dil_parse__error_set(string, source, &SET_0);
                    dil_parse__return(true);
                }
            }
            dil_parse__return(true);
        }

        if (dil_string_prefix_set(string, &SET_1)) {
            dil_parse__return(true);
        }

        dil_parse__error(string, source, "Unexpected character in `Escaped`!");
        dil_parse__return(true);
    }

    if (dil_string_prefix_not_set(string, &SET_2)) {
        dil_parse__return(true);
    }

    dil_parse__return(false);
}

/* Try to parse a reference. */
bool dil_parse_reference(DilBuilder* builder, DilString* string)
{
    return dil_parse_identifier(builder, string);
}

/* Try to parse a string. */
bool dil_parse_string(DilBuilder* builder, DilString* string, DilSource* source)
{
    dil_parse__create(DIL_SYMBOL_STRING);

    DilString const SET_0 = dil_string_terminated("0123456789abcdefABCDEF");
    DilString const SET_1 = dil_string_terminated("tn\\\"");
    DilString const SET_2 = dil_string_terminated("\\\"");

    if (!dil_parse__terminal(builder, string, '"')) {
        dil_parse__return(false);
    }

    while (dil_string_finite(string)) {
        if (dil_parse__terminal(builder, string, '\\')) {
            if (dil_string_prefix_set(string, &SET_0)) {
                for (size_t i = 0; i < 2 - 1; i++) {
                    if (!dil_string_prefix_set(string, &SET_0)) {
                        dil_parse__error_set(string, source, &SET_0);
                        dil_parse__return(true);
                    }
                }
                continue;
            }
            if (dil_string_prefix_set(string, &SET_1)) {
                continue;
            }
            dil_parse__error(
                string,
                source,
                "Unexpected character in `String`!");
            dil_parse__return(true);
        }
        if (dil_string_prefix_not_set(string, &SET_2)) {
            continue;
        }
        break;
    }

    if (!dil_parse__terminal(builder, string, '"')) {
        dil_parse__error(string, source, "Expected `\"` in `String`!");
        dil_parse__return(true);
    }

    dil_parse__return(true);
}

/* Try to parse a all set. */
bool dil_parse_all_set(DilBuilder* builder, DilString* string)
{
    dil_parse__create(DIL_SYMBOL_ALL_SET);

    if (!dil_parse__terminal(builder, string, '.')) {
        dil_parse__return(false);
    }

    dil_parse__return(true);
}

/* Try to parse a set. */
bool dil_parse_set(DilBuilder* builder, DilString* string, DilSource* source)
{
    dil_parse__create(DIL_SYMBOL_SET);

    if (!dil_parse__terminal(builder, string, '\'')) {
        dil_parse__return(false);
    }

    if (!dil_parse_escaped(builder, string, source)) {
        dil_parse__error(string, source, "Expected `Escaped` in `Set`!");
        dil_parse__return(true);
    }

    while (dil_parse_escaped(builder, string, source)) {}

    if (!dil_parse__terminal(builder, string, '\'')) {
        dil_parse__error(string, source, "Expected `'` in `Set`!");
        dil_parse__return(true);
    }

    dil_parse__return(true);
}

/* Try to parse a not set. */
bool dil_parse_not_set(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    dil_parse__create(DIL_SYMBOL_NOT_SET);

    if (!dil_parse__terminal(builder, string, '!')) {
        dil_parse__return(false);
    }

    if (!dil_parse_set(builder, string, source)) {
        dil_parse__error(string, source, "Expected `Set` in `NotSet`!");
        dil_parse__return(true);
    }

    dil_parse__return(true);
}

/* Try to parse a literal. */
bool dil_parse_literal(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    return dil_parse_set(builder, string, source) ||
           dil_parse_not_set(builder, string, source) ||
           dil_parse_all_set(builder, string) ||
           dil_parse_string(builder, string, source) ||
           dil_parse_reference(builder, string);
}

/* Try to parse a number. */
bool dil_parse_number(DilBuilder* builder, DilString* string)
{
    dil_parse__create(DIL_SYMBOL_NUMBER);

    DilString const SET_0 = dil_string_terminated("123456789");
    DilString const SET_1 = dil_string_terminated("0123456789");

    if (!dil_string_prefix_set(string, &SET_0)) {
        dil_parse__return(false);
    }

    dil_parse__skip_0(string);

    while (dil_string_prefix_set(string, &SET_1)) {}

    dil_parse__return(true);
}

/* Try to parse a group. */
bool dil_parse_group(DilBuilder* builder, DilString* string, DilSource* source)
{
    dil_parse__create(DIL_SYMBOL_GROUP);

    if (dil_parse_literal(builder, string, source)) {
        dil_parse__return(true);
    }

    if (dil_parse__terminal(builder, string, '(')) {
        dil_parse__skip_0(string);

        if (!dil_parse_literal(builder, string, source)) {
            dil_parse__error(string, source, "Expected `Literal` in `Group`!");
            dil_parse__return(true);
        }

        dil_parse__skip_0(string);

        while (dil_parse_literal(builder, string, source)) {
            dil_parse__skip_0(string);
        }

        if (!dil_parse__terminal(builder, string, ')')) {
            dil_parse__error(string, source, "Expected `)` in `Group`!");
            dil_parse__return(true);
        }

        dil_parse__return(true);
    }

    dil_parse__return(false);
}

/* Try to parse a fixed times. */
bool dil_parse_fixed_times(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    dil_parse__create(DIL_SYMBOL_FIXED_TIMES);

    if (!dil_parse_number(builder, string)) {
        dil_parse__return(false);
    }

    dil_parse__skip_0(string);

    if (!dil_parse_group(builder, string, source)) {
        dil_parse__error(string, source, "Expected `Group` in `FixedTimes`!");
        dil_parse__return(true);
    }

    dil_parse__return(true);
}

/* Try to parse a one or more. */
bool dil_parse_one_or_more(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    dil_parse__create(DIL_SYMBOL_ONE_OR_MORE);

    if (!dil_parse__terminal(builder, string, '+')) {
        dil_parse__return(false);
    }

    dil_parse__skip_0(string);

    if (!dil_parse_group(builder, string, source)) {
        dil_parse__error(string, source, "Expected `Group` in `OneOrMore`!");
        dil_parse__return(true);
    }

    dil_parse__return(true);
}

/* Try to parse a zero or more. */
bool dil_parse_zero_or_more(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    dil_parse__create(DIL_SYMBOL_ZERO_OR_MORE);

    if (!dil_parse__terminal(builder, string, '*')) {
        dil_parse__return(false);
    }

    dil_parse__skip_0(string);

    if (!dil_parse_group(builder, string, source)) {
        dil_parse__error(string, source, "Expected `Group` in `ZeroOrMore`!");
        dil_parse__return(true);
    }

    dil_parse__return(true);
}

/* Try to parse a optional. */
bool dil_parse_optional(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    dil_parse__create(DIL_SYMBOL_OPTIONAL);

    if (!dil_parse__terminal(builder, string, '?')) {
        dil_parse__return(false);
    }

    dil_parse__skip_0(string);

    if (!dil_parse_group(builder, string, source)) {
        dil_parse__error(string, source, "Expected `Group` in `Optional`!");
        dil_parse__return(true);
    }

    dil_parse__return(true);
}

/* Try to parse a repeat. */
bool dil_parse_repeat(DilBuilder* builder, DilString* string, DilSource* source)
{
    return dil_parse_group(builder, string, source) ||
           dil_parse_optional(builder, string, source) ||
           dil_parse_zero_or_more(builder, string, source) ||
           dil_parse_one_or_more(builder, string, source) ||
           dil_parse_fixed_times(builder, string, source);
}

/* Try to parse a pattern. */
bool dil_parse_alternatives(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    dil_parse__create(DIL_SYMBOL_ALTERNATIVES);

    if (!dil_parse_repeat(builder, string, source)) {
        dil_parse__return(false);
    }

    dil_parse__skip_0(string);

    while (dil_parse__terminal(builder, string, '|')) {
        dil_parse__skip_0(string);

        if (!dil_parse_repeat(builder, string, source)) {
            dil_parse__error(
                string,
                source,
                "Expected `Repeat` in `Alternatives`!");
            dil_parse__return(true);
        }

        dil_parse__skip_0(string);
    }

    dil_parse__return(true);
}

/* Try to parse a pattern. */
bool dil_parse_pattern(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    return dil_parse_alternatives(builder, string, source);
}

/* Try to parse a rule. */
bool dil_parse_rule(DilBuilder* builder, DilString* string, DilSource* source)
{
    dil_parse__create(DIL_SYMBOL_RULE);

    if (!dil_parse_identifier(builder, string)) {
        dil_parse__return(false);
    }

    dil_parse__skip_0(string);

    if (!dil_parse__terminal(builder, string, '=')) {
        dil_parse__error(string, source, "Expected `=` in `Rule`!");
        dil_parse__return(true);
    }

    dil_parse__skip_0(string);

    if (!dil_parse_pattern(builder, string, source)) {
        dil_parse__error(string, source, "Expected `Pattern` in `Rule`!");
        dil_parse__return(true);
    }

    dil_parse__skip_0(string);

    if (!dil_parse__terminal(builder, string, ';')) {
        dil_parse__error(string, source, "Expected `;` in `Rule`!");
        dil_parse__return(true);
    }

    dil_parse__return(true);
}

/* Try to parse a skip directive. */
bool dil_parse_directive_skip(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    dil_parse__create(DIL_SYMBOL_SKIP);

    DilString const TERMINALS_0 = dil_string_terminated("skip");

    if (!dil_parse__terminal_string(builder, string, &TERMINALS_0)) {
        dil_parse__return(false);
    }

    dil_parse__skip_0(string);

    if (!dil_parse_pattern(builder, string, source)) {
        dil_parse__error(string, source, "Expected `Pattern` in `Skip`!");
        dil_parse__return(true);
    }

    dil_parse__skip_0(string);

    if (!dil_parse__terminal(builder, string, ';')) {
        dil_parse__error(string, source, "Expected `;` in `Skip`!");
        dil_parse__return(true);
    }

    dil_parse__return(true);
}

/* Try to parse a start directive. */
bool dil_parse_directive_start(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    dil_parse__create(DIL_SYMBOL_START);

    DilString const TERMINALS_0 = dil_string_terminated("start");

    if (!dil_parse__terminal_string(builder, string, &TERMINALS_0)) {
        dil_parse__return(false);
    }

    dil_parse__skip_0(string);

    if (!dil_parse_pattern(builder, string, source)) {
        dil_parse__error(string, source, "Expected `Pattern` in `Start`!");
        dil_parse__return(true);
    }

    dil_parse__skip_0(string);

    if (!dil_parse__terminal(builder, string, ';')) {
        dil_parse__error(string, source, "Expected `;` in `Start`!");
        dil_parse__return(true);
    }

    dil_parse__return(true);
}

/* Try to parse an output directive. */
bool dil_parse_directive_output(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    dil_parse__create(DIL_SYMBOL_OUTPUT);

    DilString const TERMINALS_0 = dil_string_terminated("output");

    if (!dil_parse__terminal_string(builder, string, &TERMINALS_0)) {
        dil_parse__return(false);
    }

    dil_parse__skip_0(string);

    if (!dil_parse_string(builder, string, source)) {
        dil_parse__error(string, source, "Expected `String` in `Output`!");
        dil_parse__return(true);
    }

    dil_parse__skip_0(string);

    if (!dil_parse__terminal(builder, string, ';')) {
        dil_parse__error(string, source, "Expected `;` in `Output`!");
        dil_parse__return(true);
    }

    dil_parse__return(true);
}

/* Try to parse a statement. */
bool dil_parse_statement(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    return dil_parse_directive_output(builder, string, source) ||
           dil_parse_directive_start(builder, string, source) ||
           dil_parse_directive_skip(builder, string, source) ||
           dil_parse_rule(builder, string, source);
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

    dil_parse__skip_0(&string);
    while (dil_parse_statement(builder, &string, source)) {
        dil_parse__skip_0(&string);
    }

    dil_builder_pop(builder);
    dil_tree_at(builder->built, index)->object.value.last = string.first;

    if (dil_string_finite(&string)) {
        dil_parse__error(&string, source, "Unexpected characters in the file!");
    }

    if (source->error != 0) {
        printf(
            "%s: error: File had %llu errors.\n",
            source->path,
            source->error);
    }
}
