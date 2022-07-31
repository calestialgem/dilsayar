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
#include <winnls.h>

/* Context of the parsing process. */
typedef struct {
    /* Tree that is built. */
    DilTree built;
    /* Builder to parse into. */
    DilBuilder builder;
    /* Remaining source file contents. */
    DilString remaining;
    /* Parsed source file. */
    DilSource source;
    /* Whether the context could not continue parsing. */
    bool dead;
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
void dil_parse__return(DilParseContext* context)
{
    if (context->dead) {
        context->remaining.first =
            dil_builder_parent(&context->builder)->object.value.first;
        dil_builder_remove(&context->builder);
        dil_builder_parent(&context->builder)->childeren--;
        return;
    }
    dil_builder_parent(&context->builder)->object.value.last =
        context->remaining.first;
    dil_builder_pop(&context->builder);
}

/* Try to skip a comment. */
bool dil_parse__skip_comment(DilParseContext* context)
{
    DilString const STRING_0 = dil_string_terminated("//");
    DilString const SET_0    = dil_string_terminated("\n");

    if (!dil_string_prefix_check(&context->remaining, &STRING_0)) {
        return false;
    }

    while (dil_string_prefix_not_set(&context->remaining, &SET_0)) {}

    return true;
}

/* Try to skip whitespace. */
bool dil_parse__skip_whitespace(DilParseContext* context)
{
    DilString const SET_1 = dil_string_terminated("\t\n ");
    return dil_string_prefix_set(&context->remaining, &SET_1);
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
        .first = context->remaining.first,
        .last  = context->remaining.first};
    while (context->remaining.first <= context->remaining.last &&
           !dil_parse__skip_once(context)) {
        context->remaining.first++;
        portion.last++;
    }
    context->source.error++;
    dil_source_print(&context->source, &portion, "error", message);
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
void dil_parse__character(DilParseContext* context, char element)
{
    dil_parse__create(context, DIL_SYMBOL__CHARACTER);
    context->dead = !dil_string_prefix_element(&context->remaining, element);
    dil_parse__return(context);
}

/* Try to parse a character from a set. */
void dil_parse__set(DilParseContext* context, DilString const* set)
{
    dil_parse__create(context, DIL_SYMBOL__CHARACTER);
    context->dead = !dil_string_prefix_set(&context->remaining, set);
    dil_parse__return(context);
}

/* Try to parse a character from a not set. */
void dil_parse__not_set(DilParseContext* context, DilString const* set)
{
    dil_parse__create(context, DIL_SYMBOL__CHARACTER);
    context->dead = !dil_string_prefix_not_set(&context->remaining, set);
    dil_parse__return(context);
}

/* Try to parse a string. */
void dil_parse__string(DilParseContext* context, DilString const* string)
{
    dil_parse__create(context, DIL_SYMBOL__STRING);
    context->dead = !dil_string_prefix_check(&context->remaining, string);
    dil_parse__return(context);
}

/* Try to parse an identifier. */
void dil_parse_identifier(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_IDENTIFIER);

    DilString const SET_2 = dil_string_terminated("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    DilString const SET_3 = dil_string_terminated(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

    dil_parse__set(context, &SET_2);
    if (context->dead) {
        dil_parse__return(context);
        return;
    }

    while (true) {
        dil_parse__set(context, &SET_3);
        if (context->dead) {
            break;
        }
    }

    context->dead = false;
    dil_parse__return(context);
}

/* Try to parse an escaped character. */
void dil_parse_escaped(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_ESCAPED);

    DilString const SET_4 = dil_string_terminated("0123456789abcdefABCDEF");
    DilString const SET_5 = dil_string_terminated("tn\\'~");
    DilString const SET_6 = dil_string_terminated("\\'~");

    dil_parse__character(context, '\\');
    if (!context->dead) {
        dil_parse__set(context, &SET_4);
        if (!context->dead) {
            for (size_t i = 0; i < 2 - 1; i++) {
                dil_parse__set(context, &SET_4);
                if (context->dead) {
                    dil_parse__error_set(context, &SET_4);
                    dil_parse__return(context);
                    return;
                }
            }
            dil_parse__return(context);
            return;
        }

        dil_parse__set(context, &SET_5);
        if (!context->dead) {
            dil_parse__return(context);
            return;
        }

        dil_parse__error(context, "Unexpected character in `Escaped`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__not_set(context, &SET_6);
    if (!context->dead) {
        dil_parse__return(context);
        return;
    }

    dil_parse__return(context);
}

/* Try to parse a number. */
void dil_parse_number(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_NUMBER);

    DilString const SET_7 = dil_string_terminated("123456789");
    DilString const SET_8 = dil_string_terminated("0123456789");

    dil_parse__set(context, &SET_7);
    if (context->dead) {
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    while (true) {
        dil_parse__set(context, &SET_8);
        if (context->dead) {
            break;
        }
    }

    context->dead = false;
    dil_parse__return(context);
}

/* Try to parse a set. */
void dil_parse_set(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_SET);

    dil_parse__character(context, '\'');
    if (context->dead) {
        dil_parse__return(context);
        return;
    }

    while (true) {
        dil_parse_escaped(context);
        if (context->dead) {
            break;
        }

        dil_parse__character(context, '~');
        if (context->dead) {
            continue;
        }

        dil_parse_escaped(context);
        if (context->dead) {
            dil_parse__error(context, "Expected `Escaped` in `Set`!");
            dil_parse__return(context);
            return;
        }
    }

    dil_parse__character(context, '\'');
    if (context->dead) {
        dil_parse__error(context, "Expected `'` in `Set`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__return(context);
}

/* Try to parse a not set. */
void dil_parse_not_set(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_NOT_SET);

    dil_parse__character(context, '!');
    if (context->dead) {
        dil_parse__return(context);
        return;
    }

    dil_parse_set(context);
    if (context->dead) {
        dil_parse__error(context, "Expected `Set` in `NotSet`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__return(context);
}

/* Try to parse a string. */
void dil_parse_string(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_STRING);

    DilString const SET_9  = dil_string_terminated("0123456789abcdefABCDEF");
    DilString const SET_10 = dil_string_terminated("tn\\\"");
    DilString const SET_11 = dil_string_terminated("\\\"");

    dil_parse__character(context, '"');
    if (context->dead) {
        dil_parse__return(context);
        return;
    }

    while (true) {
        dil_parse__character(context, '\\');
        if (!context->dead) {
            dil_parse__set(context, &SET_9);
            if (!context->dead) {
                for (size_t i = 0; i < 2 - 1; i++) {
                    dil_parse__set(context, &SET_9);
                    if (context->dead) {
                        dil_parse__error_set(context, &SET_9);
                        dil_parse__return(context);
                        return;
                    }
                }
                continue;
            }
            dil_parse__set(context, &SET_10);
            if (!context->dead) {
                continue;
            }
            dil_parse__error(context, "Unexpected character in `String`!");
            dil_parse__return(context);
            return;
        }
        dil_parse__not_set(context, &SET_11);
        if (!context->dead) {
            continue;
        }
        break;
    }

    dil_parse__character(context, '"');
    if (context->dead) {
        dil_parse__error(context, "Expected `\"` in `String`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__return(context);
}

/* Try to parse a reference. */
void dil_parse_reference(DilParseContext* context)
{
    dil_parse_identifier(context);
}

void dil_parse_pattern(DilParseContext* context);

/* Try to parse a group. */
void dil_parse_group(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_GROUP);

    dil_parse__character(context, '(');
    if (context->dead) {
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    dil_parse_pattern(context);
    if (context->dead) {
        dil_parse__error(context, "Expected `Pattern` in `Group`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    while (true) {
        dil_parse_pattern(context);
        if (context->dead) {
            break;
        }
        dil_parse__skip(context);
    }

    dil_parse__character(context, ')');
    if (context->dead) {
        dil_parse__error(context, "Expected `)` in `Group`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__return(context);
}

/* Try to parse a fixed times. */
void dil_parse_fixed_times(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_FIXED_TIMES);

    dil_parse_number(context);
    if (context->dead) {
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    dil_parse_pattern(context);
    if (context->dead) {
        dil_parse__error(context, "Expected `Pattern` in `FixedTimes`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__return(context);
}

/* Try to parse a one or more. */
void dil_parse_one_or_more(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_ONE_OR_MORE);

    dil_parse__character(context, '+');
    if (context->dead) {
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    dil_parse_pattern(context);
    if (context->dead) {
        dil_parse__error(context, "Expected `Pattern` in `OneOrMore`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__return(context);
}

/* Try to parse a zero or more. */
void dil_parse_zero_or_more(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_ZERO_OR_MORE);

    dil_parse__character(context, '*');
    if (context->dead) {
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    dil_parse_pattern(context);
    if (context->dead) {
        dil_parse__error(context, "Expected `Pattern` in `ZeroOrMore`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__return(context);
}

/* Try to parse a optional. */
void dil_parse_optional(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_OPTIONAL);

    dil_parse__character(context, '?');
    if (context->dead) {
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    dil_parse_pattern(context);
    if (context->dead) {
        dil_parse__error(context, "Expected `Pattern` in `Optional`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__return(context);
}

/* Try to parse a justaposition. */
void dil_parse_justaposition(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_JUSTAPOSITION);

    dil_parse_pattern(context);
    if (context->dead) {
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    while (true) {
        dil_parse_pattern(context);
        if (context->dead) {
            break;
        }
        dil_parse__skip(context);
    }

    context->dead = false;
    dil_parse__return(context);
}

/* Try to parse alternatives. */
void dil_parse_alternative(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_ALTERNATIVE);

    dil_parse_pattern(context);
    if (context->dead) {
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    dil_parse__character(context, '|');
    if (context->dead) {
        dil_parse__error(context, "Expected `|` in `Alternative`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    dil_parse_pattern(context);
    if (context->dead) {
        dil_parse__error(context, "Expected `Pattern` in `Alternative`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    while (true) {
        dil_parse__character(context, '|');
        if (context->dead) {
            break;
        }
        dil_parse__skip(context);

        dil_parse_pattern(context);
        if (context->dead) {
            dil_parse__error(context, "Expected `Pattern` in `Alternative`!");
            dil_parse__return(context);
            return;
        }

        dil_parse__skip(context);
    }

    dil_parse__return(context);
}

/* Try to parse a pattern. */
void dil_parse_pattern(DilParseContext* context)
{
    dil_parse_set(context);
    if (!context->dead) {
        return;
    }

    dil_parse_not_set(context);
    if (!context->dead) {
        return;
    }

    dil_parse_string(context);
    if (!context->dead) {
        return;
    }

    dil_parse_reference(context);
    if (!context->dead) {
        return;
    }

    dil_parse_group(context);
    if (!context->dead) {
        return;
    }

    dil_parse_fixed_times(context);
    if (!context->dead) {
        return;
    }

    dil_parse_one_or_more(context);
    if (!context->dead) {
        return;
    }

    dil_parse_zero_or_more(context);
    if (!context->dead) {
        return;
    }

    dil_parse_optional(context);
    if (!context->dead) {
        return;
    }

    dil_parse_justaposition(context);
    if (!context->dead) {
        return;
    }

    dil_parse_alternative(context);
}

/* Try to parse a rule. */
void dil_parse_rule(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_RULE);

    dil_parse_identifier(context);
    if (context->dead) {
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    dil_parse__character(context, '=');
    if (context->dead) {
        dil_parse__error(context, "Expected `=` in `Rule`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    dil_parse_pattern(context);
    if (context->dead) {
        dil_parse__error(context, "Expected `Pattern` in `Rule`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    dil_parse__character(context, ';');
    if (context->dead) {
        dil_parse__error(context, "Expected `;` in `Rule`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__return(context);
}

/* Try to parse a terminal. */
void dil_parse_terminal(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_TERMINAL);

    DilString const TERMINALS_1 = dil_string_terminated("terminal");

    dil_parse__string(context, &TERMINALS_1);
    if (context->dead) {
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    dil_parse__character(context, ';');
    if (context->dead) {
        dil_parse__error(context, "Expected `;` in `Skip`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__return(context);
}

/* Try to parse a skip. */
void dil_parse_skip(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_SKIP);

    DilString const TERMINALS_2 = dil_string_terminated("skip");

    dil_parse__string(context, &TERMINALS_2);
    if (context->dead) {
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    dil_parse_pattern(context);
    if (!context->dead) {
        dil_parse__skip(context);
    }

    dil_parse__character(context, ';');
    if (context->dead) {
        dil_parse__error(context, "Expected `;` in `Skip`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__return(context);
}

/* Try to parse a start. */
void dil_parse_start(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_START);

    DilString const TERMINALS_3 = dil_string_terminated("start");

    dil_parse__string(context, &TERMINALS_3);
    if (context->dead) {
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    dil_parse_pattern(context);
    if (context->dead) {
        dil_parse__error(context, "Expected `Pattern` in `Start`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    dil_parse__character(context, ';');
    if (context->dead) {
        dil_parse__error(context, "Expected `;` in `Start`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__return(context);
}

/* Try to parse an output. */
void dil_parse_output(DilParseContext* context)
{
    dil_parse__create(context, DIL_SYMBOL_OUTPUT);

    DilString const TERMINALS_4 = dil_string_terminated("output");

    dil_parse__string(context, &TERMINALS_4);
    if (context->dead) {
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    dil_parse_string(context);
    if (context->dead) {
        dil_parse__error(context, "Expected `String` in `Output`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__skip(context);

    dil_parse__character(context, ';');
    if (context->dead) {
        dil_parse__error(context, "Expected `;` in `Output`!");
        dil_parse__return(context);
        return;
    }

    dil_parse__return(context);
}

/* Try to parse a statement. */
void dil_parse_statement(DilParseContext* context)
{
    dil_parse_output(context);
    if (!context->dead) {
        return;
    }

    dil_parse_start(context);
    if (!context->dead) {
        return;
    }

    dil_parse_skip(context);
    if (!context->dead) {
        return;
    }

    dil_parse_terminal(context);
    if (!context->dead) {
        return;
    }

    dil_parse_rule(context);
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
    dil_parse__skip(context);
    while (true) {
        dil_parse_statement(context);
        if (context->dead) {
            break;
        }
        dil_parse__skip(context);
    }

    dil_builder_parent(&context->builder)->object.value.last =
        context->remaining.first;
    dil_builder_pop(&context->builder);

    if (dil_string_finite(&context->remaining)) {
        dil_parse__error(context, "Unexpected characters in the file!");
    }
}

/* Parses the source file. */
DilTree dil_parse(DilSource source)
{
    DilParseContext initial = {
        .builder   = {.built = &initial.built},
        .remaining = source.contents,
        .source    = source};

    dil_parse__start(&initial);

    if (initial.source.error != 0) {
        printf(
            "%s: error: File had %llu errors.\n",
            initial.source.path,
            initial.source.error);
    }

    dil_builder_free(&initial.builder);
    return initial.built;
}
