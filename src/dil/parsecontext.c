// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/builder.c"
#include "dil/source.c"
#include "dil/string.c"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Context of the parsing process. */
typedef struct {
    /* Builder to parse into. */
    DilBuilder* builder;
    /* Remaining source file contents. */
    DilString* string;
    /* Parsed source file. */
    DilSource* source;
    /* Function that should be called next. */
    bool (*next)(void* context, void* list);
} DilParseContext;

/* Contiguous, dynamicly allocated elements. */
typedef struct {
    /* Border before the first element. */
    DilParseContext* first;
    /* Border after the last element. */
    DilParseContext* last;
    /* Border after the last allocated element. */
    DilParseContext* allocated;
} DilParseContextList;

/* Amount of elements. */
size_t dil_parse_context_list_size(DilParseContextList const* list)
{
    return list->last - list->first;
}

/* Amount of allocated elements. */
size_t dil_parse_context_list_capacity(DilParseContextList const* list)
{
    return list->allocated - list->first;
}

/* Amount of allocated but unused elements. */
size_t dil_parse_context_list_space(DilParseContextList const* list)
{
    return list->allocated - list->last;
}

/* Whether there are any elements. */
bool dil_parse_context_list_finite(DilParseContextList const* list)
{
    return dil_parse_context_list_size(list) > 0;
}

/* Pointer to the element at the index. */
DilParseContext*
dil_parse_context_list_at(DilParseContextList const* list, size_t index)
{
    return list->first + index;
}

/* Element at the index. */
DilParseContext
dil_parse_context_list_get(DilParseContextList const* list, size_t index)
{
    return *dil_parse_context_list_at(list, index);
}

/* Pointer to the first element. */
DilParseContext* dil_parse_context_list_start(DilParseContextList const* list)
{
    return list->first;
}

/* Pointer to the last element. */
DilParseContext* dil_parse_context_list_finish(DilParseContextList const* list)
{
    return list->last - 1;
}

/* Make sure the amount of elements will fit. Grows by at least half if
 * necessary. */
void dil_parse_context_list_reserve(DilParseContextList* list, size_t amount)
{
    size_t space = dil_parse_context_list_space(list);
    if (amount <= space) {
        return;
    }

    size_t growth       = amount - space;
    size_t capacity     = dil_parse_context_list_capacity(list);
    size_t halfCapacity = capacity / 2;
    if (growth < halfCapacity) {
        growth = halfCapacity;
    }

    size_t           newCapacity = capacity + growth;
    DilParseContext* memory =
        realloc(list->first, newCapacity * sizeof(DilParseContext));

    list->last      = memory + dil_parse_context_list_size(list);
    list->first     = memory;
    list->allocated = memory + newCapacity;
}

/* Add the element to the end. */
void dil_parse_context_list_add(
    DilParseContextList* list,
    DilParseContext      element)
{
    dil_parse_context_list_reserve(list, 1);
    *list->last++ = element;
}

/* Open space at the index for the amount of element. Returns pointer to the
 * first opened element. */
DilParseContext* dil_parse_context_list_open(
    DilParseContextList* list,
    size_t               index,
    size_t               amount)
{
    dil_parse_context_list_reserve(list, amount);
    DilParseContext* position = list->first + index;
    memmove(position + amount, position, amount * sizeof(DilParseContext));
    list->last += amount;
    return position;
}

/* Put the element to the given index. */
void dil_parse_context_list_put(
    DilParseContextList* list,
    size_t               index,
    DilParseContext      element)
{
    *dil_parse_context_list_open(list, index, 1) = element;
}

/* Place the element the amount of times to the end. */
void dil_parse_context_list_place(
    DilParseContextList* list,
    size_t               amount,
    DilParseContext      element)
{
    dil_parse_context_list_reserve(list, amount);
    for (size_t i = 0; i < amount; i++) {
        *list->last++ = element;
    }
}

/* Remove from the end. */
void dil_parse_context_list_remove(DilParseContextList* list)
{
    list->last--;
}

/* Remove from the end and return the removed element. */
DilParseContext dil_parse_context_list_pop(DilParseContextList* list)
{
    dil_parse_context_list_remove(list);
    return *list->last;
}

/* Remove all the elements. Keeps the memory. */
void dil_parse_context_list_clear(DilParseContextList* list)
{
    list->last = list->first;
}

/* Deallocate memory. */
void dil_parse_context_list_free(DilParseContextList* list)
{
    free(list->first);
    list->first     = NULL;
    list->last      = NULL;
    list->allocated = NULL;
}
