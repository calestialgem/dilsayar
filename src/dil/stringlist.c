// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/string.c"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Contiguous, dynamicly allocated elements. */
typedef struct {
    /* Border before the first element. */
    DilString* first;
    /* Border after the last element. */
    DilString* last;
    /* Border after the last allocated element. */
    DilString* allocated;
} DilStringList;

/* Amount of elements. */
size_t dil_string_list_size(DilStringList const* list)
{
    return list->last - list->first;
}

/* Amount of allocated elements. */
size_t dil_string_list_capacity(DilStringList const* list)
{
    return list->allocated - list->first;
}

/* Amount of allocated but unused elements. */
size_t dil_string_list_space(DilStringList const* list)
{
    return list->allocated - list->last;
}

/* Whether there are any elements. */
bool dil_string_list_finite(DilStringList const* list)
{
    return dil_string_list_size(list) > 0;
}

/* Pointer to the element at the index. */
DilString* dil_string_list_at(DilStringList const* list, size_t index)
{
    return list->first + index;
}

/* Element at the index. */
DilString dil_string_list_get(DilStringList const* list, size_t index)
{
    return *dil_string_list_at(list, index);
}

/* Pointer to the first element. */
DilString* dil_string_list_start(DilStringList const* list)
{
    return list->first;
}

/* Pointer to the last element. */
DilString* dil_string_list_finish(DilStringList const* list)
{
    return list->last - 1;
}

/* Make sure the amount of elements will fit. Grows by at least half if
 * necessary. */
void dil_string_list_reserve(DilStringList* list, size_t amount)
{
    size_t space = dil_string_list_space(list);
    if (amount <= space) {
        return;
    }

    size_t growth       = amount - space;
    size_t capacity     = dil_string_list_capacity(list);
    size_t halfCapacity = capacity / 2;
    if (growth < halfCapacity) {
        growth = halfCapacity;
    }

    size_t     newCapacity = capacity + growth;
    DilString* memory = realloc(list->first, newCapacity * sizeof(DilString));

    list->last      = memory + dil_string_list_size(list);
    list->first     = memory;
    list->allocated = memory + newCapacity;
}

/* Add the element to the end. */
void dil_string_list_add(DilStringList* list, DilString element)
{
    dil_string_list_reserve(list, 1);
    *list->last++ = element;
}

/* Open space at the index for the amount of element. Returns pointer to the
 * first opened element. */
DilString*
dil_string_list_open(DilStringList* list, size_t index, size_t amount)
{
    dil_string_list_reserve(list, amount);
    DilString* position = list->first + index;
    memmove(position + amount, position, amount * sizeof(DilString));
    list->last += amount;
    return position;
}

/* Put the element to the given index. */
void dil_string_list_put(DilStringList* list, size_t index, DilString element)
{
    *dil_string_list_open(list, index, 1) = element;
}

/* Place the element the amount of times to the end. */
void dil_string_list_place(
    DilStringList* list,
    size_t         amount,
    DilString      element)
{
    dil_string_list_reserve(list, amount);
    for (size_t i = 0; i < amount; i++) {
        *list->last++ = element;
    }
}

/* Remove from the end. */
void dil_string_list_remove(DilStringList* list)
{
    list->last--;
}

/* Remove from the end and return the removed element. */
DilString dil_string_list_pop(DilStringList* list)
{
    dil_string_list_remove(list);
    return *list->last;
}

/* Remove all the elements. Keeps the memory. */
void dil_string_list_clear(DilStringList* list)
{
    list->last = list->first;
}

/* Deallocate memory. */
void dil_string_list_free(DilStringList* list)
{
    free(list->first);
    list->first     = NULL;
    list->last      = NULL;
    list->allocated = NULL;
}

/* Whether the list contains the element. */
bool dil_string_list_contains(
    DilStringList const* list,
    DilString const*     element)
{
    for (DilString const* i = list->first; i < list->last; i++) {
        if (dil_string_equal(i, element)) {
            return true;
        }
    }

    return false;
}
