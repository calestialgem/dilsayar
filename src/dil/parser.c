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

/* Try to parse a specific terminal. */
bool dil_parse_terminal(DilBuilder* builder, DilString* string, char terminal)
{
    DilObject object = {
        .symbol = DIL_SYMBOL_TERMINAL,
        .value  = {.first = string->first}};
    size_t index = dil_tree_size(builder->built);

    if (!dil_string_prefix_element(string, terminal)) {
        return false;
    }

    dil_builder_add(builder, object);
    dil_tree_at(builder->built, index)->object.value.last = string->first;
    return true;
}

/* Try to parse any terminal. */
bool dil_parse_terminal_any(DilBuilder* builder, DilString* string)
{
    DilObject object = {
        .symbol = DIL_SYMBOL_TERMINAL,
        .value  = {.first = string->first}};
    size_t index = dil_tree_size(builder->built);

    if (!dil_string_finite(string)) {
        return false;
    }

    // string: "jflkdrsj"

    string->first++;
    dil_builder_add(builder, object);
    dil_tree_at(builder->built, index)->object.value.last = string->first;
    return true;
}

/* Try to parse an identifier. */
bool dil_parse_identifier(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    return false;
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

    if (dil_string_starts(string, '\\')) {
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

/* Try to parse a reference. */
bool dil_parse_reference(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    return dil_parse_identifier(builder, string, source);
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

/* Try to parse a all set. */
bool dil_parse_all_set(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    return false;
}

/* Try to parse a not set. */
bool dil_parse_not_set(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    return false;
}

/* Try to parse a set. */
bool dil_parse_set(DilBuilder* builder, DilString* string, DilSource* source)
{
    DilObject object = {
        .symbol = DIL_SYMBOL_SET,
        .value  = {.first = string->first}};

    if (!dil_string_prefix_element(string, '\'')) {
        return false;
    }

    size_t index = dil_tree_size(builder->built);
    dil_builder_add(builder, object);
    dil_builder_push(builder);

    if (!dil_parse_escaped(builder, string, source)) {
        dil_parse_error(string, source, "Expected `Escaped` in `Set`!");
        goto end;
    }

    while (dil_parse_escaped(builder, string, source)) {}

    if (!dil_string_prefix_element(string, '\'')) {
        dil_parse_error(string, source, "Expected `'''` in `Set`!");
        goto end;
    }

end:
    dil_builder_pop(builder);
    dil_tree_at(builder->built, index)->object.value.last = string->first;
    return true;
}

/* Try to parse a literal. */
bool dil_parse_literal(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    return dil_parse_set(builder, string, source) ||
           dil_parse_not_set(builder, string, source) ||
           dil_parse_all_set(builder, string, source) ||
           dil_parse_string(builder, string, source) ||
           dil_parse_reference(builder, string, source);
}

/* Try to parse a number. */
bool dil_parse_number(DilBuilder* builder, DilString* string)
{
    DilObject object = {
        .symbol = DIL_SYMBOL_NUMBER,
        .value  = {.first = string->first}};

    {
        DilString const set = dil_string_terminated("123456789");
        if (!dil_string_prefix_set(string, &set)) {
            return false;
        }
    }

    dil_parse_skip(string);

    size_t index = dil_tree_size(builder->built);
    dil_builder_add(builder, object);
    dil_builder_push(builder);

    {
        DilString const set = dil_string_terminated("0123456789");
        while (dil_string_prefix_set(string, &set)) {}
    }

    dil_builder_pop(builder);
    dil_tree_at(builder->built, index)->object.value.last = string->first;
    return true;
}

/* Try to parse a group. */
bool dil_parse_group(DilBuilder* builder, DilString* string, DilSource* source)
{
    DilObject object = {
        .symbol = DIL_SYMBOL_GROUP,
        .value  = {.first = string->first}};
    size_t index = dil_tree_size(builder->built);

    if (dil_parse_literal(builder, string, source)) {
        dil_builder_add(builder, object);
        dil_builder_push(builder);
        goto end;
    } else if (dil_string_prefix_element(string, '(')) {
        dil_builder_add(builder, object);
        dil_builder_push(builder);

        dil_parse_skip(string);

        if (!dil_parse_literal(builder, string, source)) {
            dil_parse_error(string, source, "Expected `Literal` in `Group`!");
            goto end;
        }

        dil_parse_skip(string);

        while (dil_parse_literal(builder, string, source)) {
            dil_parse_skip(string);
        }

        if (!dil_string_prefix_element(string, ')')) {
            dil_parse_error(string, source, "Expected `')'` in `Group`!");
            goto end;
        }

        goto end;
    } else {
        return false;
    }

end:
    dil_builder_pop(builder);
    dil_tree_at(builder->built, index)->object.value.last = string->first;
    return true;
}

/* Try to parse a fixed times. */
bool dil_parse_fixed_times(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    DilObject object = {
        .symbol = DIL_SYMBOL_FIXED_TIMES,
        .value  = {.first = string->first}};

    if (!dil_parse_number(builder, string)) {
        return false;
    }

    dil_parse_skip(string);

    size_t index = dil_tree_size(builder->built);
    dil_builder_add(builder, object);
    dil_builder_push(builder);

    if (!dil_parse_group(builder, string, source)) {
        dil_parse_error(string, source, "Expected `Group` in `FixedTimes`!");
        goto end;
    }

end:
    dil_builder_pop(builder);
    dil_tree_at(builder->built, index)->object.value.last = string->first;
    return true;
}

/* Try to parse a one or more. */
bool dil_parse_one_or_more(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    DilObject object = {
        .symbol = DIL_SYMBOL_ONE_OR_MORE,
        .value  = {.first = string->first}};

    if (!dil_string_prefix_element(string, '+')) {
        return false;
    }

    dil_parse_skip(string);

    size_t index = dil_tree_size(builder->built);
    dil_builder_add(builder, object);
    dil_builder_push(builder);

    if (!dil_parse_group(builder, string, source)) {
        dil_parse_error(string, source, "Expected `Group` in `OneOrMore`!");
        goto end;
    }

end:
    dil_builder_pop(builder);
    dil_tree_at(builder->built, index)->object.value.last = string->first;
    return true;
}

/* Try to parse a zero or more. */
bool dil_parse_zero_or_more(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    DilObject object = {
        .symbol = DIL_SYMBOL_ZERO_OR_MORE,
        .value  = {.first = string->first}};

    if (!dil_string_prefix_element(string, '*')) {
        return false;
    }

    dil_parse_skip(string);

    size_t index = dil_tree_size(builder->built);
    dil_builder_add(builder, object);
    dil_builder_push(builder);

    if (!dil_parse_group(builder, string, source)) {
        dil_parse_error(string, source, "Expected `Group` in `ZeroOrMore`!");
        goto end;
    }

end:
    dil_builder_pop(builder);
    dil_tree_at(builder->built, index)->object.value.last = string->first;
    return true;
}

/* Try to parse a optional. */
bool dil_parse_optional(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    DilObject object = {
        .symbol = DIL_SYMBOL_OPTIONAL,
        .value  = {.first = string->first}};

    if (!dil_string_prefix_element(string, '?')) {
        return false;
    }

    dil_parse_skip(string);

    size_t index = dil_tree_size(builder->built);
    dil_builder_add(builder, object);
    dil_builder_push(builder);

    if (!dil_parse_group(builder, string, source)) {
        dil_parse_error(string, source, "Expected `Group` in `Optional`!");
        goto end;
    }

end:
    dil_builder_pop(builder);
    dil_tree_at(builder->built, index)->object.value.last = string->first;
    return true;
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
    DilObject object = {
        .symbol = DIL_SYMBOL_ALTERNATIVES,
        .value  = {.first = string->first}};

    if (!dil_parse_repeat(builder, string, source)) {
        return false;
    }

    dil_parse_skip(string);

    size_t index = dil_tree_size(builder->built);
    dil_builder_add(builder, object);
    dil_builder_push(builder);

    while (dil_string_prefix_element(string, '|')) {
        dil_parse_skip(string);

        if (!dil_parse_repeat(builder, string, source)) {
            dil_parse_error(
                string,
                source,
                "Expected `Repeat` in `Alternatives`!");
            goto end;
        }

        dil_parse_skip(string);
    }

end:
    dil_builder_pop(builder);
    dil_tree_at(builder->built, index)->object.value.last = string->first;
    return true;
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
    DilObject object = {
        .symbol = DIL_SYMBOL_RULE,
        .value  = {.first = string->first}};

    if (!dil_parse_identifier(builder, string, source)) {
        return false;
    }

    dil_parse_skip(string);

    size_t index = dil_tree_size(builder->built);
    dil_builder_add(builder, object);
    dil_builder_push(builder);

    if (!dil_string_prefix_element(string, '=')) {
        dil_parse_error(string, source, "Expected `'='` in `Rule`!");
        goto end;
    }

    dil_parse_skip(string);

    if (!dil_parse_pattern(builder, string, source)) {
        dil_parse_error(string, source, "Expected `Pattern` in `Rule`!");
        goto end;
    }

    dil_parse_skip(string);

    if (!dil_string_prefix_element(string, ';')) {
        dil_parse_error(string, source, "Expected `';'` in `Rule`!");
        goto end;
    }

end:
    dil_builder_pop(builder);
    dil_tree_at(builder->built, index)->object.value.last = string->first;
    return true;
}

/* Try to parse a skip directive. */
bool dil_parse_directive_skip(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    DilObject object = {
        .symbol = DIL_SYMBOL_SKIP,
        .value  = {.first = string->first}};

    {
        DilString const terminals = dil_string_terminated("skip");
        if (!dil_string_prefix_check(string, &terminals)) {
            return false;
        }
    }

    dil_parse_skip(string);

    size_t index = dil_tree_size(builder->built);
    dil_builder_add(builder, object);
    dil_builder_push(builder);

    if (!dil_parse_pattern(builder, string, source)) {
        dil_parse_error(string, source, "Expected `Pattern` in `Skip`!");
        goto end;
    }

    dil_parse_skip(string);

    if (!dil_string_prefix_element(string, ';')) {
        dil_parse_error(string, source, "Expected `';'` in `Skip`!");
        goto end;
    }

end:
    dil_builder_pop(builder);
    dil_tree_at(builder->built, index)->object.value.last = string->first;
    return true;
}

/* Try to parse a start directive. */
bool dil_parse_directive_start(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    DilObject object = {
        .symbol = DIL_SYMBOL_START,
        .value  = {.first = string->first}};

    {
        DilString const terminals = dil_string_terminated("start");
        if (!dil_string_prefix_check(string, &terminals)) {
            return false;
        }
    }

    dil_parse_skip(string);

    size_t index = dil_tree_size(builder->built);
    dil_builder_add(builder, object);
    dil_builder_push(builder);

    if (!dil_parse_pattern(builder, string, source)) {
        dil_parse_error(string, source, "Expected `Pattern` in `Start`!");
        goto end;
    }

    dil_parse_skip(string);

    if (!dil_string_prefix_element(string, ';')) {
        dil_parse_error(string, source, "Expected `';'` in `Start`!");
        goto end;
    }

end:
    dil_builder_pop(builder);
    dil_tree_at(builder->built, index)->object.value.last = string->first;
    return true;
}

/* Try to parse an output directive. */
bool dil_parse_directive_output(
    DilBuilder* builder,
    DilString*  string,
    DilSource*  source)
{
    DilObject object = {
        .symbol = DIL_SYMBOL_OUTPUT,
        .value  = {.first = string->first}};

    {
        DilString const terminals = dil_string_terminated("output");
        if (!dil_string_prefix_check(string, &terminals)) {
            return false;
        }
    }

    dil_parse_skip(string);

    size_t index = dil_tree_size(builder->built);
    dil_builder_add(builder, object);
    dil_builder_push(builder);

    if (!dil_parse_string(builder, string, source)) {
        dil_parse_error(string, source, "Expected `String` in `Output`!");
        goto end;
    }

    dil_parse_skip(string);

    if (!dil_string_prefix_element(string, ';')) {
        dil_parse_error(string, source, "Expected `';'` in `Output`!");
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
            .object = {
                       .symbol = DIL_SYMBOL__START,
                       .value  = {.first = string.first}}
    });
    dil_builder_push(builder);

    dil_parse_skip(&string);
    while (dil_parse_statement(builder, &string, source)) {
        dil_parse_skip(&string);
    }

    dil_builder_pop(builder);
    dil_tree_at(builder->built, index)->object.value.last = string.first;

    if (dil_string_finite(&string)) {
        dil_parse_error(&string, source, "Unexpected characters in the file!");
    }

    if (source->error != 0) {
        printf(
            "%s: error: File had %llu errors.\n",
            source->path,
            source->error);
    }
}
