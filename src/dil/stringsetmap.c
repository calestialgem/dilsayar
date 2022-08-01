// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/string.c"
#include "dil/stringset.c"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Hash of the key. */
size_t dil_string_set_map_hash(DilString const* key)
{
    return dil_string_hash(key);
}

/* Whether the keys are equal. */
bool dil_string_set_map_equal(DilString const* lhs, DilString const* rhs)
{
    return dil_string_equal(lhs, rhs);
}

/* Mapping from a key to a value. */
typedef struct {
    /* Key. */
    DilString key;
    /* Value. */
    DilStringSet value;
} DilStringSetMapPair;

/* Contiguous range of pairs. */
typedef struct {
    /* Border before the first pair. */
    DilStringSetMapPair* first;
    /* Border after the last pair. */
    DilStringSetMapPair* last;
} DilStringSetMapPairRange;

/* Amount of pairs in the range. */
size_t dil_string_set_map_range_size(DilStringSetMapPairRange const* range)
{
    return range->last - range->first;
}

/* Find the pair with the key in the range. Returns null if not found. */
DilStringSetMapPair* dil_string_set_map_range_find(
    DilStringSetMapPairRange const* range,
    DilString const*                key)
{
    for (DilStringSetMapPair* i = range->first; i < range->last; i++) {
        if (dil_string_set_map_equal(&i->key, key)) {
            return i;
        }
    }
    return NULL;
}

/* Whether the key is in the range. */
bool dil_string_set_map_range_contains(
    DilStringSetMapPairRange const* range,
    DilString const*                key)
{
    return dil_string_set_map_range_find(range, key) != NULL;
}

/* Collection of mappings that use the hash of the key, that is stored
 * contiguously in the memory. */
typedef struct {
    /* Mappings. */
    struct {
        /* Border before the first pair. */
        DilStringSetMapPair* first;
        /* Border after the last pair. */
        DilStringSetMapPair* last;
        /* Border after the last allocated pair. */
        DilStringSetMapPair* allocated;
    } pairs;
    /* Pair indicies corresponding to hashes. */
    struct {
        /* Border before the first index. */
        size_t* first;
        /* Border before the last index. */
        size_t* last;
    } indicies;
} DilStringSetMap;

/* Amount of mappings in the map. */
size_t dil_string_set_map_size(DilStringSetMap const* map)
{
    return map->pairs.last - map->pairs.first;
}

/* Amount of indicies in the map. */
size_t dil_string_set_map_indicies(DilStringSetMap const* map)
{
    return map->indicies.last - map->indicies.first;
}

/* Range corresponding to the hash. */
DilStringSetMapPairRange
dil_string_set_map_range(DilStringSetMap const* map, size_t hash)
{
    size_t indicies = dil_string_set_map_indicies(map);

    // If the indicies are empty, the pairs are empty too.
    if (indicies == 0) {
        return (DilStringSetMapPairRange){0};
    }

    // Find the indicies that correspond to current and the next hashes.
    size_t* current = map->indicies.first + hash % indicies;
    size_t* next    = current + 1;

    DilStringSetMapPairRange result = {
        .first = map->pairs.first + *current,
        .last  = map->pairs.last};

    // The current hash might be the last one; thus, check the next one.
    // If the next hash is valid, limit the search view from the end.
    if (next < map->indicies.last) {
        result.last = map->pairs.first + *next;
    }

    return result;
}

/* Pointer to the value that corresponds to the key. Returns nullptr if the
 * value does not exist. */
DilStringSet*
dil_string_set_map_at(DilStringSetMap const* map, DilString const* key)
{
    DilStringSetMapPairRange range =
        dil_string_set_map_range(map, dil_string_set_map_hash(key));
    DilStringSetMapPair* pair = dil_string_set_map_range_find(&range, key);
    return &pair->value;
}

/* Whether the map contains the key. */
bool dil_string_set_map_contains(
    DilStringSetMap const* map,
    DilString const*       key)
{
    DilStringSetMapPairRange range =
        dil_string_set_map_range(map, dil_string_set_map_hash(key));
    return dil_string_set_map_range_contains(&range, key);
}

/* Maximum allowed amount of keys whose hashes give the same index after modulus
 * operation. Decreases the speed of the map, but also the memory usage. */
#define DIL_STRING_SET_MAP_MAX_COLLISION 1

/* Whether the map should be rehashed to fit the collision restriction. */
bool dil_string_set_map_need_rehash(DilStringSetMap const* map)
{
    size_t indicies = dil_string_set_map_indicies(map);
    size_t pairs    = dil_string_set_map_size(map);
    for (size_t i = 0; i < indicies; i++) {
        DilStringSetMapPairRange range = dil_string_set_map_range(map, i);
        size_t                   size  = dil_string_set_map_range_size(&range);
        if (size > DIL_STRING_SET_MAP_MAX_COLLISION) {
            return true;
        }
        pairs -= size;
        if (pairs <= DIL_STRING_SET_MAP_MAX_COLLISION) {
            break;
        }
    }
    return false;
}

/* Grow the indicies. Doubles starting from 1. */
void dil_string_set_map_grow_indicies(DilStringSetMap* map)
{
    size_t size    = dil_string_set_map_indicies(map);
    size_t newSize = size * 2;
    if (newSize == 0) {
        newSize = 1;
    }

    size_t* memory = realloc(map->indicies.first, newSize * sizeof(size_t*));

    map->indicies.first = memory;
    map->indicies.last  = memory + newSize;
}

/* Compare pairs by hash. */
int dil_string_set_map_compare(
    void*       indiciesp,
    void const* lhs,
    void const* rhs)
{
    size_t leftHash =
        dil_string_set_map_hash(&((DilStringSetMapPair*)lhs)->key);
    size_t rightHash =
        dil_string_set_map_hash(&((DilStringSetMapPair*)rhs)->key);
    size_t indicies = *(size_t*)indiciesp;
    return (int)(leftHash % indicies - rightHash % indicies);
}

/* Recalculate indicies. */
void dil_string_set_map_recalculate(DilStringSetMap* map)
{
    size_t index    = 0;
    size_t indicies = dil_string_set_map_indicies(map);
    size_t pairs    = dil_string_set_map_size(map);
    for (size_t i = 0; i < pairs; i++) {
        size_t current =
            dil_string_set_map_hash(&map->pairs.first[i].key) % indicies;
        while (current >= index) {
            map->indicies.first[index++] = i;
        }
    }
    while (index < indicies) {
        map->indicies.first[index++] = pairs;
    }
}

/* Rehash until the maximum collision restriction is obeyed. */
void dil_string_set_map_rehash(DilStringSetMap* map)
{
    do {
        dil_string_set_map_grow_indicies(map);
        size_t indicies = dil_string_set_map_indicies(map);

        // Sort the pairs by hash.
        qsort_s(
            map->pairs.first,
            dil_string_set_map_size(map),
            sizeof(DilStringSetMapPair),
            &dil_string_set_map_compare,
            &indicies);

        dil_string_set_map_recalculate(map);
    } while (dil_string_set_map_need_rehash(map));
}

/* Amount of allocated pairs. */
size_t dil_string_set_map_capacity(DilStringSetMap const* map)
{
    return map->pairs.allocated - map->pairs.first;
}

/* Amount of allocated but unused pairs. */
size_t dil_string_set_map_space(DilStringSetMap const* map)
{
    return map->pairs.allocated - map->pairs.last;
}

/* Make sure the amount of pairs will fit. Grows by at least half of the current
 * capacity if necessary. */
void dil_string_set_map_reserve(DilStringSetMap* map, size_t amount)
{
    size_t space = dil_string_set_map_space(map);
    if (amount <= space) {
        return;
    }

    size_t growth       = amount - space;
    size_t capacity     = dil_string_set_map_capacity(map);
    size_t halfCapacity = capacity / 2;
    if (growth < halfCapacity) {
        growth = halfCapacity;
    }

    size_t               newCapacity = capacity + growth;
    DilStringSetMapPair* memory =
        realloc(map->pairs.first, newCapacity * sizeof(DilStringSetMapPair));

    map->pairs.last      = memory + dil_string_set_map_size(map);
    map->pairs.first     = memory;
    map->pairs.allocated = memory + newCapacity;
}

/* Put the pair to the index. */
void dil_string_set_map_put(
    DilStringSetMap*    map,
    size_t              index,
    DilStringSetMapPair pair)
{
    dil_string_set_map_reserve(map, 1);
    DilStringSetMapPair* position = map->pairs.first + index;
    size_t               move     = map->pairs.last++ - position;
    if (move != 0) {
        memmove(position + 1, position, move * sizeof(DilStringSetMapPair));
    }
    *position = pair;
}

/* Add a mapping to the value with the key. */
void dil_string_set_map_add(
    DilStringSetMap* map,
    DilString        key,
    DilStringSet     value)
{
    // Get the corresponding view for the hash.
    size_t                   hash  = dil_string_set_map_hash(&key);
    DilStringSetMapPairRange range = dil_string_set_map_range(map, hash);

    DilStringSetMapPair pair     = {.key = key, .value = value};
    size_t              indicies = dil_string_set_map_indicies(map);

    // If it would not create too many collisions.
    if (dil_string_set_map_range_size(&range) <
            DIL_STRING_SET_MAP_MAX_COLLISION &&
        indicies != 0) {
        dil_string_set_map_put(map, range.last - map->pairs.first, pair);

        // Increase indicies.
        for (size_t* i = map->indicies.first + hash % indicies + 1;
             i < map->indicies.last;
             i++) {
            (*i)++;
        }
    } else {
        // Just add to the end; rehash will handle the rest.
        dil_string_set_map_put(map, dil_string_set_map_size(map), pair);
        dil_string_set_map_rehash(map);
    }
}

/* Deallocate the memory. */
void dil_string_set_map_free(DilStringSetMap* map)
{
    free(map->pairs.first);
    map->pairs.first     = NULL;
    map->pairs.last      = NULL;
    map->pairs.allocated = NULL;
    free(map->indicies.first);
    map->indicies.first = NULL;
    map->indicies.last  = NULL;
}

/* Print the map. */
void dil_string_set_map_print(DilStringSetMap const* map)
{
    printf("[\n");
    for (DilStringSetMapPair const* i = map->pairs.first; i < map->pairs.last;
         i++) {
        printf("    (%.*s, ", (int)dil_string_size(&i->key), i->key.first);
        dil_string_set_print(&i->value);
        printf(")\n");
    }
    printf("]\n");
}
