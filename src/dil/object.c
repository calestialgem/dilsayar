// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/string.c"

#include <stddef.h>
#include <stdio.h>

typedef enum {
    DIL_SYMBOL__START,
    DIL_SYMBOL_STATEMENT,
    DIL_SYMBOL_OUTPUT,
    DIL_SYMBOL_START,
    DIL_SYMBOL_SKIP,
    DIL_SYMBOL_RULE,
    DIL_SYMBOL_PATTERN,
    DIL_SYMBOL_ALTERNATIVE,
    DIL_SYMBOL_OPTIONAL,
    DIL_SYMBOL_ZERO_OR_MORE,
    DIL_SYMBOL_ONE_OR_MORE,
    DIL_SYMBOL_FIXED_TIMES,
    DIL_SYMBOL_GROUP,
    DIL_SYMBOL_NUMBER,
    DIL_SYMBOL_LITERAL,
    DIL_SYMBOL_SET,
    DIL_SYMBOL_NOT_SET,
    DIL_SYMBOL_STRING,
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
        case DIL_SYMBOL__START:
            return "__Start__";
        case DIL_SYMBOL_STATEMENT:
            return "Statement {%.*s}";
        case DIL_SYMBOL_OUTPUT:
            return "Output {%.*s}";
        case DIL_SYMBOL_START:
            return "Start {%.*s}";
        case DIL_SYMBOL_SKIP:
            return "Skip {%.*s}";
        case DIL_SYMBOL_RULE:
            return "Rule {%.*s}";
        case DIL_SYMBOL_PATTERN:
            return "Pattern {%.*s}";
        case DIL_SYMBOL_ALTERNATIVE:
            return "Alternative {%.*s}";
        case DIL_SYMBOL_OPTIONAL:
            return "Optional {%.*s}";
        case DIL_SYMBOL_ZERO_OR_MORE:
            return "Zero Or More {%.*s}";
        case DIL_SYMBOL_ONE_OR_MORE:
            return "One Or More {%.*s}";
        case DIL_SYMBOL_FIXED_TIMES:
            return "Fixed Times {%.*s}";
        case DIL_SYMBOL_GROUP:
            return "Group {%.*s}";
        case DIL_SYMBOL_NUMBER:
            return "Number {%.*s}";
        case DIL_SYMBOL_LITERAL:
            return "Literal {%.*s}";
        case DIL_SYMBOL_SET:
            return "Set {%.*s}";
        case DIL_SYMBOL_NOT_SET:
            return "Not Set {%.*s}";
        case DIL_SYMBOL_STRING:
            return "String {%.*s}";
        case DIL_SYMBOL_REFERENCE:
            return "Reference {%.*s}";
        case DIL_SYMBOL_ESCAPED:
            return "Escaped {%.*s}";
        case DIL_SYMBOL_IDENTIFIER:
            return "Identifier {%.*s}";
        case DIL_SYMBOL_WHITESPACE:
            return "Whitespace {%.*s}";
        case DIL_SYMBOL_COMMENT:
            return "Comment {%.*s}";
        case DIL_SYMBOL_TERMINAL:
            return "Terminal {%.*s}";
        default:
            return "Unknown !{%.*s}";
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
