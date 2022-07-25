// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/builder.c"
#include "dil/object.c"
#include "dil/string.c"
#include "dil/tree.c"

#include <stdbool.h>
#include <stddef.h>

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

/* Skip according to the skip directive. */
void dil_parse_skip(DilString* string)
{
    bool skip = true;
    while (skip) {
        skip =
            dil_parse_skip_whitespace(string) || dil_parse_skip_comment(string);
    }
}

/* Try to parse a string. */
bool dil_parse_string(DilBuilder* builder, DilString* string)
{
    return false;
}

/* Try to parse a rule. */
bool dil_parse_rule(DilBuilder* builder, DilString* string)
{
    return false;
}

/* Try to parse a skip directive. */
bool dil_parse_directive_skip(DilBuilder* builder, DilString* string)
{
    return false;
}

/* Try to parse a start directive. */
bool dil_parse_directive_start(DilBuilder* builder, DilString* string)
{
    return false;
}

/* Try to parse an output directive. */
bool dil_parse_directive_output(DilBuilder* builder, DilString* string)
{
    DilString const directive = dil_string_terminated("#output");
    if (!dil_string_prefix_check(string, &directive)) {
        return false;
    }
    dil_parse_skip(string);

    if (!dil_parse_string(builder, string)) {}

    dil_parse_skip(string);

    return true;
}

/* Try to parse a statement. */
bool dil_parse_statement(DilBuilder* builder, DilString* string)
{
    return dil_parse_directive_output(builder, string) ||
           dil_parse_directive_start(builder, string) ||
           dil_parse_directive_skip(builder, string) ||
           dil_parse_rule(builder, string);
}

/* Parses the start symbol. */
void dil_parse(DilBuilder* builder, DilString string)
{
    size_t start = dil_tree_size(builder->built);
    dil_builder_add(
        builder,
        (DilObject){.symbol = DIL_SYMBOL_START, .value = string});
    dil_builder_push(builder);

    bool parsed = true;
    while (parsed) {
        dil_parse_skip(&string);
        parsed = dil_parse_statement(builder, &string);
    }

    dil_builder_pop(builder);
    dil_tree_at(builder->built, start)->object.value.last = string.first;
}
