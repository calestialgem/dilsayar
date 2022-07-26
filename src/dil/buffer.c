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
    char* first;
    /* Border after the last element. */
    char* last;
    /* Border after the last allocated element. */
    char* allocated;
} DilBuffer;

/* Amount of elements. */
size_t dil_buffer_size(DilBuffer const* list)
{
    return list->last - list->first;
}

/* Amount of allocated elements. */
size_t dil_buffer_capacity(DilBuffer const* list)
{
    return list->allocated - list->first;
}

/* Amount of allocated but unused elements. */
size_t dil_buffer_space(DilBuffer const* list)
{
    return list->allocated - list->last;
}

/* Whether there are any elements. */
bool dil_buffer_finite(DilBuffer const* list)
{
    return dil_buffer_size(list) > 0;
}

/* Pointer to the element at the index. */
char* dil_buffer_at(DilBuffer const* list, size_t index)
{
    return list->first + index;
}

/* Element at the index. */
char dil_buffer_get(DilBuffer const* list, size_t index)
{
    return *dil_buffer_at(list, index);
}

/* Pointer to the first element. */
char* dil_buffer_start(DilBuffer const* list)
{
    return list->first;
}

/* Pointer to the last element. */
char* dil_buffer_finish(DilBuffer const* list)
{
    return list->last - 1;
}

/* Make sure the amount of elements will fit. Grows by at least half if
 * necessary. */
void dil_buffer_reserve(DilBuffer* list, size_t amount)
{
    size_t space = dil_buffer_space(list);
    if (amount <= space) {
        return;
    }

    size_t growth       = amount - space;
    size_t capacity     = dil_buffer_capacity(list);
    size_t halfCapacity = capacity / 2;
    if (growth < halfCapacity) {
        growth = halfCapacity;
    }

    size_t newCapacity = capacity + growth;
    char*  memory      = realloc(list->first, newCapacity * sizeof(char));

    list->last      = memory + dil_buffer_size(list);
    list->first     = memory;
    list->allocated = memory + newCapacity;
}

/* Add the element to the end. */
void dil_buffer_add(DilBuffer* list, char element)
{
    dil_buffer_reserve(list, 1);
    *list->last++ = element;
}

/* Open space at the index for the amount of element. Returns pointer to the
 * first opened element. */
char* dil_buffer_open(DilBuffer* list, size_t index, size_t amount)
{
    dil_buffer_reserve(list, amount);
    char* position = list->first + index;
    memmove(position + amount, position, amount * sizeof(char));
    list->last += amount;
    return position;
}

/* Put the element to the given index. */
void dil_buffer_put(DilBuffer* list, size_t index, char element)
{
    *dil_buffer_open(list, index, 1) = element;
}

/* Place the element the amount of times to the end. */
void dil_buffer_place(DilBuffer* list, size_t amount, char element)
{
    dil_buffer_reserve(list, amount);
    for (size_t i = 0; i < amount; i++) {
        *list->last++ = element;
    }
}

/* Remove from the end. */
void dil_buffer_remove(DilBuffer* list)
{
    list->last--;
}

/* Remove from the end and return the removed element. */
char dil_buffer_pop(DilBuffer* list)
{
    dil_buffer_remove(list);
    return *list->last;
}

/* Remove all the elements. Keeps the memory. */
void dil_buffer_clear(DilBuffer* list)
{
    list->last = list->first;
}

/* Deallocate memory. */
void dil_buffer_free(DilBuffer* list)
{
    free(list->first);
    list->first     = NULL;
    list->last      = NULL;
    list->allocated = NULL;
}
