// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/builder.c"
#include "dil/object.c"
#include "dil/source.c"
#include "dil/string.c"
#include "dil/tree.c"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/* Try to parse a comment. */
bool dil_parse_skip_comment(DilString* string)
{
    DilString const prefix = dil_string_terminated("//");
    if (!dil_string_prefix_check(string, &prefix)) {
        return false;
    }
    dil_string_lead_first(string, '\n');
    if (dil_string_finite(string)) {
        string->first++;
    }
    return true;
}

/* Whether the character is not whitespace. */
bool dil_parse_is_not_whitespace(char character)
{
    return character != '\t' && character != '\n' && character != ' ';
}

/* Try to parse whitespace. */
bool dil_parse_skip_whitespace(DilString* string)
{
    DilString prefix =
        dil_string_lead_first_fit(string, &dil_parse_is_not_whitespace);
    return dil_string_finite(&prefix);
}

/* Try to parse a skip. */
bool dil_parse_skip_once(DilString* string)
{
    return dil_parse_skip_whitespace(string) || dil_parse_skip_comment(string);
}

/* Skip as much as possible. */
void dil_parse_skip(DilString* string)
{
    while (dil_parse_skip_once(string)) {}
}

/* Skip erronous characters and print them. */
void dil_parse_error(DilString* string, DilSource* source, char const* message)
{
    DilString portion = {.first = string->first, .last = string->first};
    while (string->first <= string->last && !dil_parse_skip_once(string)) {
        string->first++;
        portion.last++;
    }
    source->error++;
    dil_source_print(source, &portion, "error", message);
}

/* Try to parse an escaped character. */
bool dil_parse_escaped(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    DilObject object = {
        .symbol = DIL_SYMBOL_ESCAPED,
        .value  = {.first = string->first}};
    size_t index = dil_tree_size(builder->built);

    if (dil_string_prefix_element(string, '\\')) {
        dil_builder_add(builder, object);
        dil_builder_push(builder);
    } else {
        dil_builder_add(builder, object);
        dil_builder_push(builder);
        if (!dil_string_finite(string)) {
            dil_parse_error(string, source, "Expected a character!");
            goto end;
        }
    }

end:
    dil_builder_pop(builder);
    dil_tree_at(builder->built, index)->object.value.last = string->first;
    return true;
}

/* Try to parse a string. */
bool dil_parse_string(DilBuilder* builder, DilString* string, DilSource* source)
{
    DilObject object = {
        .symbol = DIL_SYMBOL_STRING,
        .value  = {.first = string->first}};

    if (!dil_string_prefix_element(string, '"')) {
        return false;
    }

    size_t index = dil_tree_size(builder->built);
    dil_builder_add(builder, object);
    dil_builder_push(builder);

    while (dil_parse_escaped(builder, string, source)) {}

    if (!dil_string_prefix_element(string, '"')) {
        dil_parse_error(string, source, "Expected `\"` to end the string!");
        goto end;
    }

end:
    dil_builder_pop(builder);
    dil_tree_at(builder->built, index)->object.value.last = string->first;
    return true;
}

/* Try to parse a rule. */
bool dil_parse_rule(DilBuilder* builder, DilString* string, DilSource* source)
{
    return false;
}

/* Try to parse a skip directive. */
bool dil_parse_directive_skip(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    return false;
}

/* Try to parse a start directive. */
bool dil_parse_directive_start(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    return false;
}

/* Try to parse an output directive. */
bool dil_parse_directive_output(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    DilObject object = {
        .symbol = DIL_SYMBOL_DIRECTIVE_OUTPUT,
        .value  = {.first = string->first}};

    DilString const directive = dil_string_terminated("#output");
    if (!dil_string_prefix_check(string, &directive)) {
        return false;
    }
    dil_parse_skip(string);

    size_t index = dil_tree_size(builder->built);
    dil_builder_add(builder, object);
    dil_builder_push(builder);

    if (!dil_parse_string(builder, string, source)) {
        dil_parse_error(
            string,
            source,
            "Expected file name in `#output` directive!");
        goto end;
    }
    dil_parse_skip(string);

    if (!dil_string_prefix_element(string, ';')) {
        dil_parse_error(
            string,
            source,
            "Expected `;` to end the `#output` directive!");
        goto end;
    }

end:
    dil_builder_pop(builder);
    dil_tree_at(builder->built, index)->object.value.last = string->first;
    return true;
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
            .object = {.symbol = DIL_SYMBOL_START, .value = string}
    });
    dil_builder_push(builder);

    bool parsed = true;
    while (parsed) {
        dil_parse_skip(&string);
        parsed = dil_parse_statement(builder, &string, source);
    }

    dil_builder_pop(builder);
    dil_tree_at(builder->built, index)->object.value.last = string.first;

    if (source->error != 0) {
        printf(
            "%s: error: File had %llu errors.\n",
            source->path,
            source->error);
    }
}
