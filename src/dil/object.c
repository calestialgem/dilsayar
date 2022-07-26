// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/string.c"

#include <stddef.h>
#include <stdio.h>

typedef enum {
    DIL_SYMBOL_START,
    DIL_SYMBOL_STATEMENT,
    DIL_SYMBOL_DIRECTIVE_OUTPUT,
    DIL_SYMBOL_DIRECTIVE_START,
    DIL_SYMBOL_DIRECTIVE_SKIP,
    DIL_SYMBOL_RULE,
    DIL_SYMBOL_ALTERNATIVES,
    DIL_SYMBOL_REPEAT,
    DIL_SYMBOL_OPTIONAL,
    DIL_SYMBOL_ZERO_OR_MORE,
    DIL_SYMBOL_ONE_OR_MORE,
    DIL_SYMBOL_FIXED_TIMES,
    DIL_SYMBOL_GROUP,
    DIL_SYMBOL_LITERAL,
    DIL_SYMBOL_CHARACTER,
    DIL_SYMBOL_RANGE,
    DIL_SYMBOL_STRING,
    DIL_SYMBOL_WILDCARD,
    DIL_SYMBOL_REFERENCE,
    DIL_SYMBOL_ESCAPED,
    DIL_SYMBOL_IDENTIFIER,
    DIL_SYMBOL_WHITESPACE,
    DIL_SYMBOL_COMMENT,
    DIL_SYMBOL_TERMINAL
} DilSymbol;

typedef struct {
    DilSymbol symbol;
    DilString value;
} DilObject;

typedef struct {
    DilObject object;
    size_t    childeren;
} DilNode;

/* Format string of the symbol. */
char const* dil_object_format(DilSymbol symbol)
{
    switch (symbol) {
        case DIL_SYMBOL_START:
            return "start";
        case DIL_SYMBOL_STATEMENT:
            return "statement";
        case DIL_SYMBOL_DIRECTIVE_OUTPUT:
            return "#output";
        case DIL_SYMBOL_DIRECTIVE_START:
            return "#start";
        case DIL_SYMBOL_DIRECTIVE_SKIP:
            return "#skip";
        case DIL_SYMBOL_RULE:
            return "rule";
        case DIL_SYMBOL_ALTERNATIVES:
            return "|";
        case DIL_SYMBOL_REPEAT:
            return "repeat";
        case DIL_SYMBOL_OPTIONAL:
            return "?";
        case DIL_SYMBOL_ZERO_OR_MORE:
            return "*";
        case DIL_SYMBOL_ONE_OR_MORE:
            return "+";
        case DIL_SYMBOL_FIXED_TIMES:
            return "%.*s";
        case DIL_SYMBOL_GROUP:
            return "{}";
        case DIL_SYMBOL_LITERAL:
            return "literal";
        case DIL_SYMBOL_CHARACTER:
        case DIL_SYMBOL_RANGE:
        case DIL_SYMBOL_STRING:
        case DIL_SYMBOL_WILDCARD:
        case DIL_SYMBOL_REFERENCE:
        case DIL_SYMBOL_ESCAPED:
        case DIL_SYMBOL_IDENTIFIER:
            return "%.*s";
        case DIL_SYMBOL_WHITESPACE:
            return "whitespace";
        case DIL_SYMBOL_COMMENT:
            return "comment";
        case DIL_SYMBOL_TERMINAL:
            return "%.*s";
        default:
            return "!(%.*s)";
    }
}

/* Print the object. */
void dil_object_print(DilObject const* object)
{
    printf(
        dil_object_format(object->symbol),
        (int)(dil_string_size(&object->value)),
        object->value.first);
}
