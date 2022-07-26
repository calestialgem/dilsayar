// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/builder.c"
#include "dil/file.c"
#include "dil/object.c"
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
void dil_parse_error(DilString* string, DilFile* file, char const* message)
{
    DilString portion = {.first = string->first, .last = string->first};
    while (string->first <= string->last && !dil_parse_skip_once(string)) {
        string->first++;
        portion.last++;
    }
    file->error++;
    dil_file_print(file, portion, "error", message);
}

/* Try to parse a string. */
bool dil_parse_string(DilBuilder* builder, DilString* string, DilFile* file)
{
    return false;
}

/* Try to parse a rule. */
bool dil_parse_rule(DilBuilder* builder, DilString* string, DilFile* file)
{
    return false;
}

/* Try to parse a skip directive. */
bool dil_parse_directive_skip(
    DilBuilder* builder,
    DilString*  string,
    DilFile*    file)
{
    return false;
}

/* Try to parse a start directive. */
bool dil_parse_directive_start(
    DilBuilder* builder,
    DilString*  string,
    DilFile*    file)
{
    return false;
}

/* Try to parse an output directive. */
bool dil_parse_directive_output(
    DilBuilder* builder,
    DilString*  string,
    DilFile*    file)
{
    DilString const directive = dil_string_terminated("#output");
    if (!dil_string_prefix_check(string, &directive)) {
        return false;
    }
    dil_parse_skip(string);

    if (!dil_parse_string(builder, string, file)) {
        dil_parse_error(
            string,
            file,
            "Expected file name in `#output` directive!");
        return true;
    }

    dil_parse_skip(string);

    DilString const statement = dil_string_terminated(";");
    if (!dil_string_prefix_check(string, &statement)) {
        dil_parse_error(
            string,
            file,
            "Expected `;` to end the `#output` directive!");
        return true;
    }
    return true;
}

/* Try to parse a statement. */
bool dil_parse_statement(DilBuilder* builder, DilString* string, DilFile* file)
{
    return dil_parse_directive_output(builder, string, file) ||
           dil_parse_directive_start(builder, string, file) ||
           dil_parse_directive_skip(builder, string, file) ||
           dil_parse_rule(builder, string, file);
}

/* Parses the start symbol. */
void dil_parse(DilBuilder* builder, DilFile* file)
{
    DilString string = file->contents;

    size_t start = dil_tree_size(builder->built);
    dil_tree_add(
        builder->built,
        (DilNode){
            .object = {.symbol = DIL_SYMBOL_START, .value = string}
    });
    dil_builder_push(builder);

    bool parsed = true;
    while (parsed) {
        dil_parse_skip(&string);
        parsed = dil_parse_statement(builder, &string, file);
    }

    dil_builder_pop(builder);
    dil_tree_at(builder->built, start)->object.value.last = string.first;

    if (file->error != 0) {
        printf("%s: error: File had %llu errors.\n", file->path, file->error);
    }
}
