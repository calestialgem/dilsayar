// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/string.c"

#include <stddef.h>

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
    size_t    childeren;
} DilObject;
