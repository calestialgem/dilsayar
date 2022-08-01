// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/string.c"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Hash of the value. */
size_t dil_string_set_hash(DilString const* value)
{
    return dil_string_hash(value);
}

/* Whether the values are equal. */
bool dil_string_set_equal(DilString const* lhs, DilString const* rhs)
{
    return dil_string_equal(lhs, rhs);
}

/* Contiguous range of values. */
typedef struct {
    /* Border before the first value. */
    DilString* first;
    /* Border after the last value. */
    DilString* last;
} DilStringSetRange;

/* Amount of values in the range. */
size_t dil_string_set_range_size(DilStringSetRange const* range)
{
    return range->last - range->first;
}

/* Find the value in the range. Returns null if not found. */
DilString* dil_string_set_range_find(
    DilStringSetRange const* range,
    DilString const*         value)
{
    for (DilString* i = range->first; i < range->last; i++) {
        if (dil_string_set_equal(i, value)) {
            return i;
        }
    }
    return NULL;
}

/* Whether the value is in the range. */
bool dil_string_set_range_contains(
    DilStringSetRange const* range,
    DilString const*         value)
{
    return dil_string_set_range_find(range, value) != NULL;
}

/* Collection of values that use the hash of the value, that is stored
 * contiguously in the memory. */
typedef struct {
    /* Values. */
    struct {
        /* Border before the first value. */
        DilString* first;
        /* Border after the last value. */
        DilString* last;
        /* Border after the last allocated value. */
        DilString* allocated;
    } values;
    /* Value indicies corresponding to hashes. */
    struct {
        /* Border before the first index. */
        size_t* first;
        /* Border before the last index. */
        size_t* last;
    } indicies;
} DilStringSet;

/* Amount of values in the set. */
size_t dil_string_set_size(DilStringSet const* set)
{
    return set->values.last - set->values.first;
}

/* Amount of indicies in the set. */
size_t dil_string_set_indicies(DilStringSet const* set)
{
    return set->indicies.last - set->indicies.first;
}

/* Range corresponding to the hash. */
DilStringSetRange dil_string_set_range(DilStringSet const* set, size_t hash)
{
    size_t indicies = dil_string_set_indicies(set);

    // If the indicies are empty, the values are empty too.
    if (indicies == 0) {
        return (DilStringSetRange){0};
    }

    // Find the indicies that correspond to current and the next hashes.
    size_t* current = set->indicies.first + hash % indicies;
    size_t* next    = current + 1;

    DilStringSetRange result = {
        .first = set->values.first + *current,
        .last  = set->values.last};

    // The current hash might be the last one; thus, check the next one.
    // If the next hash is valid, limit the search view from the end.
    if (next < set->indicies.last) {
        result.last = set->values.first + *next;
    }

    return result;
}

/* Pointer to the value that corresponds to the value. Returns nullptr if the
 * value does not exist. */
DilString* dil_string_set_at(DilStringSet const* set, DilString const* value)
{
    DilStringSetRange range =
        dil_string_set_range(set, dil_string_set_hash(value));
    return dil_string_set_range_find(&range, value);
}

/* Whether the set contains the value. */
bool dil_string_set_contains(DilStringSet const* set, DilString const* value)
{
    DilStringSetRange range =
        dil_string_set_range(set, dil_string_set_hash(value));
    return dil_string_set_range_contains(&range, value);
}

/* Maximum allowed amount of values whose hashes give the same index after
 * modulus operation. Decreases the speed of the set, but also the memory usage.
 */
#define DIL_STRING_SET_MAX_COLLISION 1

/* Whether the set should be rehashed to fit the collision restriction. */
bool dil_string_set_need_rehash(DilStringSet const* set)
{
    size_t indicies = dil_string_set_indicies(set);
    size_t values   = dil_string_set_size(set);
    for (size_t i = 0; i < indicies; i++) {
        DilStringSetRange range = dil_string_set_range(set, i);
        size_t            size  = dil_string_set_range_size(&range);
        if (size > DIL_STRING_SET_MAX_COLLISION) {
            return true;
        }
        values -= size;
        if (values <= DIL_STRING_SET_MAX_COLLISION) {
            break;
        }
    }
    return false;
}

/* Grow the indicies. Doubles starting from 1. */
void dil_string_set_grow_indicies(DilStringSet* set)
{
    size_t size    = dil_string_set_indicies(set);
    size_t newSize = size * 2;
    if (newSize == 0) {
        newSize = 1;
    }

    size_t* memory = realloc(set->indicies.first, newSize * sizeof(size_t*));

    set->indicies.first = memory;
    set->indicies.last  = memory + newSize;
}

/* Compare values by hash. */
int dil_string_set_compare(void* indiciesp, void const* lhs, void const* rhs)
{
    size_t leftHash  = dil_string_set_hash(lhs);
    size_t rightHash = dil_string_set_hash(rhs);
    size_t indicies  = *(size_t*)indiciesp;
    return (int)(leftHash % indicies - rightHash % indicies);
}

/* Recalculate indicies. */
void dil_string_set_recalculate(DilStringSet* set)
{
    size_t index    = 0;
    size_t indicies = dil_string_set_indicies(set);
    size_t values   = dil_string_set_size(set);
    for (size_t i = 0; i < values; i++) {
        size_t current = dil_string_set_hash(&set->values.first[i]) % indicies;
        while (current >= index) {
            set->indicies.first[index++] = i;
        }
    }
    while (index < indicies) {
        set->indicies.first[index++] = values;
    }
}

/* Rehash until the maximum collision restriction is obeyed. */
void dil_string_set_rehash(DilStringSet* set)
{
    do {
        dil_string_set_grow_indicies(set);
        size_t indicies = dil_string_set_indicies(set);

        // Sort the values by hash.
        qsort_s(
            set->values.first,
            dil_string_set_size(set),
            sizeof(DilString),
            &dil_string_set_compare,
            &indicies);

        dil_string_set_recalculate(set);
    } while (dil_string_set_need_rehash(set));
}

/* Amount of allocated values. */
size_t dil_string_set_capacity(DilStringSet const* set)
{
    return set->values.allocated - set->values.first;
}

/* Amount of allocated but unused values. */
size_t dil_string_set_space(DilStringSet const* set)
{
    return set->values.allocated - set->values.last;
}

/* Make sure the amount of values will fit. Grows by at least half of the
 * current capacity if necessary. */
void dil_string_set_reserve(DilStringSet* set, size_t amount)
{
    size_t space = dil_string_set_space(set);
    if (amount <= space) {
        return;
    }

    size_t growth       = amount - space;
    size_t capacity     = dil_string_set_capacity(set);
    size_t halfCapacity = capacity / 2;
    if (growth < halfCapacity) {
        growth = halfCapacity;
    }

    size_t     newCapacity = capacity + growth;
    DilString* memory =
        realloc(set->values.first, newCapacity * sizeof(DilString));

    set->values.last      = memory + dil_string_set_size(set);
    set->values.first     = memory;
    set->values.allocated = memory + newCapacity;
}

/* Put the value to the index. */
void dil_string_set_put(DilStringSet* set, size_t index, DilString value)
{
    dil_string_set_reserve(set, 1);
    DilString* position = set->values.first + index;
    size_t     move     = set->values.last++ - position;
    if (move != 0) {
        memmove(position + 1, position, move * sizeof(DilString));
    }
    *position = value;
}

/* Add a setping to the value with the value. */
void dil_string_set_add(DilStringSet* set, DilString value)
{
    // Get the corresponding view for the hash.
    size_t            hash  = dil_string_set_hash(&value);
    DilStringSetRange range = dil_string_set_range(set, hash);

    size_t indicies = dil_string_set_indicies(set);

    // If it would not create too many collisions.
    if (dil_string_set_range_size(&range) < DIL_STRING_SET_MAX_COLLISION &&
        indicies != 0) {
        dil_string_set_put(set, range.last - set->values.first, value);

        // Increase indicies.
        for (size_t* i = set->indicies.first + hash % indicies + 1;
             i < set->indicies.last;
             i++) {
            (*i)++;
        }
    } else {
        // Just add to the end; rehash will handle the rest.
        dil_string_set_put(set, dil_string_set_size(set), value);
        dil_string_set_rehash(set);
    }
}

/* Deallocate the memory. */
void dil_string_set_free(DilStringSet* set)
{
    free(set->values.first);
    set->values.first     = NULL;
    set->values.last      = NULL;
    set->values.allocated = NULL;
    free(set->indicies.first);
    set->indicies.first = NULL;
    set->indicies.last  = NULL;
}

/* Print the set. */
void dil_string_set_print(DilStringSet const* set)
{
    printf("{");
    for (DilString const* i = set->values.first; i < set->values.last; i++) {
        printf("%.*s", (int)dil_string_size(i), i->first);
        if (i + 1 < set->values.last) {
            printf(",");
        }
    }
    printf("}");
}
