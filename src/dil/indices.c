// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Contiguous, dynamicly allocated elements. */
typedef struct {
    /* Border before the first element. */
    size_t* first;
    /* Border after the last element. */
    size_t* last;
    /* Border after the last allocated element. */
    size_t* allocated;
} DilIndices;

/* Amount of elements. */
size_t dil_indices_size(DilIndices const* list)
{
    return list->last - list->first;
}

/* Amount of allocated elements. */
size_t dil_indices_capacity(DilIndices const* list)
{
    return list->allocated - list->first;
}

/* Amount of allocated but unused elements. */
size_t dil_indices_space(DilIndices const* list)
{
    return list->allocated - list->last;
}

/* Whether there are any elements. */
bool dil_indices_finite(DilIndices const* list)
{
    return dil_indices_size(list) > 0;
}

/* Pointer to the element at the index. */
size_t* dil_indices_at(DilIndices const* list, size_t index)
{
    return list->first + index;
}

/* Element at the index. */
size_t dil_indices_get(DilIndices const* list, size_t index)
{
    return *dil_indices_at(list, index);
}

/* Pointer to the first element. */
size_t* dil_indices_start(DilIndices const* list)
{
    return list->first;
}

/* Pointer to the last element. */
size_t* dil_indices_finish(DilIndices const* list)
{
    return list->last - 1;
}

/* Make sure the amount of elements will fit. Grows by at least half if
 * necessary. */
void dil_indices_reserve(DilIndices* list, size_t amount)
{
    size_t space = dil_indices_space(list);
    if (amount <= space) {
        return;
    }

    size_t growth       = amount - space;
    size_t capacity     = dil_indices_capacity(list);
    size_t halfCapacity = capacity / 2;
    if (growth < halfCapacity) {
        growth = halfCapacity;
    }

    size_t  newCapacity = capacity + growth;
    size_t* memory      = realloc(list->first, newCapacity * sizeof(size_t));

    list->last      = memory + dil_indices_size(list);
    list->first     = memory;
    list->allocated = memory + newCapacity;
}

/* Add the element to the end. */
void dil_indices_add(DilIndices* list, size_t element)
{
    dil_indices_reserve(list, 1);
    *list->last++ = element;
}

/* Open space at the index for the amount of element. Returns pointer to the
 * first opened element. */
size_t* dil_indices_open(DilIndices* list, size_t index, size_t amount)
{
    dil_indices_reserve(list, amount);
    size_t* position = list->first + index;
    memmove(position + amount, position, amount * sizeof(size_t));
    list->last += amount;
    return position;
}

/* Put the element to the given index. */
void dil_indices_put(DilIndices* list, size_t index, size_t element)
{
    *dil_indices_open(list, index, 1) = element;
}

/* Place the element the amount of times to the end. */
void dil_indices_place(DilIndices* list, size_t amount, size_t element)
{
    dil_indices_reserve(list, amount);
    for (size_t i = 0; i < amount; i++) {
        *list->last++ = element;
    }
}

/* Remove from the end. */
void dil_indices_remove(DilIndices* list)
{
    list->last--;
}

/* Remove from the end and return the removed element. */
size_t dil_indices_pop(DilIndices* list)
{
    dil_indices_remove(list);
    return *list->last;
}

/* Remove all the elements. Keeps the memory. */
void dil_indices_clear(DilIndices* list)
{
    list->last = list->first;
}

/* Deallocate memory. */
void dil_indices_free(DilIndices* list)
{
    free(list->first);
    list->first     = NULL;
    list->last      = NULL;
    list->allocated = NULL;
}
