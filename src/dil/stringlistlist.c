// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/stringlist.c"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Contiguous, dynamicly allocated elements. */
typedef struct {
    /* Border before the first element. */
    DilStringList* first;
    /* Border after the last element. */
    DilStringList* last;
    /* Border after the last allocated element. */
    DilStringList* allocated;
} DilStringListList;

/* Amount of elements. */
size_t dil_string_list_list_size(DilStringListList const* list)
{
    return list->last - list->first;
}

/* Amount of allocated elements. */
size_t dil_string_list_list_capacity(DilStringListList const* list)
{
    return list->allocated - list->first;
}

/* Amount of allocated but unused elements. */
size_t dil_string_list_list_space(DilStringListList const* list)
{
    return list->allocated - list->last;
}

/* Whether there are any elements. */
bool dil_string_list_list_finite(DilStringListList const* list)
{
    return dil_string_list_list_size(list) > 0;
}

/* Pointer to the element at the index. */
DilStringList*
dil_string_list_list_at(DilStringListList const* list, size_t index)
{
    return list->first + index;
}

/* Element at the index. */
DilStringList
dil_string_list_list_get(DilStringListList const* list, size_t index)
{
    return *dil_string_list_list_at(list, index);
}

/* Pointer to the first element. */
DilStringList* dil_string_list_list_start(DilStringListList const* list)
{
    return list->first;
}

/* Pointer to the last element. */
DilStringList* dil_string_list_list_finish(DilStringListList const* list)
{
    return list->last - 1;
}

/* Make sure the amount of elements will fit. Grows by at least half if
 * necessary. */
void dil_string_list_list_reserve(DilStringListList* list, size_t amount)
{
    size_t space = dil_string_list_list_space(list);
    if (amount <= space) {
        return;
    }

    size_t growth       = amount - space;
    size_t capacity     = dil_string_list_list_capacity(list);
    size_t halfCapacity = capacity / 2;
    if (growth < halfCapacity) {
        growth = halfCapacity;
    }

    size_t         newCapacity = capacity + growth;
    DilStringList* memory =
        realloc(list->first, newCapacity * sizeof(DilStringList));

    list->last      = memory + dil_string_list_list_size(list);
    list->first     = memory;
    list->allocated = memory + newCapacity;
}

/* Add the element to the end. */
void dil_string_list_list_add(DilStringListList* list, DilStringList element)
{
    dil_string_list_list_reserve(list, 1);
    *list->last++ = element;
}

/* Open space at the index for the amount of element. Returns pointer to the
 * first opened element. */
DilStringList*
dil_string_list_list_open(DilStringListList* list, size_t index, size_t amount)
{
    dil_string_list_list_reserve(list, amount);
    DilStringList* position = list->first + index;
    memmove(position + amount, position, amount * sizeof(DilStringList));
    list->last += amount;
    return position;
}

/* Put the element to the given index. */
void dil_string_list_list_put(
    DilStringListList* list,
    size_t             index,
    DilStringList      element)
{
    *dil_string_list_list_open(list, index, 1) = element;
}

/* Place the element the amount of times to the end. */
void dil_string_list_list_place(
    DilStringListList* list,
    size_t             amount,
    DilStringList      element)
{
    dil_string_list_list_reserve(list, amount);
    for (size_t i = 0; i < amount; i++) {
        *list->last++ = element;
    }
}

/* Remove from the end. */
void dil_string_list_list_remove(DilStringListList* list)
{
    list->last--;
}

/* Remove from the end and return the removed element. */
DilStringList dil_string_list_list_pop(DilStringListList* list)
{
    dil_string_list_list_remove(list);
    return *list->last;
}

/* Remove all the elements. Keeps the memory. */
void dil_string_list_list_clear(DilStringListList* list)
{
    list->last = list->first;
}

/* Deallocate memory. */
void dil_string_list_list_free(DilStringListList* list)
{
    free(list->first);
    list->first     = NULL;
    list->last      = NULL;
    list->allocated = NULL;
}
