// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/string.c"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Hash of the key. */
size_t dil_index_map_hash(DilString const* key)
{
    return dil_string_hash(key);
}

/* Whether the keys are equal. */
bool dil_index_map_equal(DilString const* lhs, DilString const* rhs)
{
    return dil_string_equal(lhs, rhs);
}

/* Mapping from a key to a value. */
typedef struct {
    /* Key. */
    DilString key;
    /* Value. */
    size_t value;
} DilIndexMapPair;

/* Contiguous range of pairs. */
typedef struct {
    /* Border before the first pair. */
    DilIndexMapPair* first;
    /* Border after the last pair. */
    DilIndexMapPair* last;
} DilIndexMapPairRange;

/* Amount of pairs in the range. */
size_t dil_index_map_range_size(DilIndexMapPairRange const* range)
{
    return range->last - range->first;
}

/* Find the pair with the key in the range. Returns null if not found. */
DilIndexMapPair* dil_index_map_range_find(
    DilIndexMapPairRange const* range,
    DilString const*            key)
{
    for (DilIndexMapPair* i = range->first; i < range->last; i++) {
        if (dil_index_map_equal(&i->key, key)) {
            return i;
        }
    }
    return NULL;
}

/* Whether the key is in the range. */
bool dil_index_map_range_contains(
    DilIndexMapPairRange const* range,
    DilString const*            key)
{
    return dil_index_map_range_find(range, key) != NULL;
}

/* Collection of mappings that use the hash of the key, that is stored
 * contiguously in the memory. */
typedef struct {
    /* Mappings. */
    struct {
        /* Border before the first pair. */
        DilIndexMapPair* first;
        /* Border after the last pair. */
        DilIndexMapPair* last;
        /* Border after the last allocated pair. */
        DilIndexMapPair* allocated;
    } pairs;
    /* Pair indicies corresponding to hashes. */
    struct {
        /* Border before the first index. */
        size_t* first;
        /* Border before the last index. */
        size_t* last;
    } indicies;
} DilIndexMap;

/* Amount of mappings in the map. */
size_t dil_index_map_size(DilIndexMap const* map)
{
    return map->pairs.last - map->pairs.first;
}

/* Amount of indicies in the map. */
size_t dil_index_map_indicies(DilIndexMap const* map)
{
    return map->indicies.last - map->indicies.first;
}

/* Range corresponding to the hash. */
DilIndexMapPairRange dil_index_map_range(DilIndexMap const* map, size_t hash)
{
    size_t indicies = dil_index_map_indicies(map);

    // If the indicies are empty, the pairs are empty too.
    if (indicies == 0) {
        return (DilIndexMapPairRange){0};
    }

    // Find the indicies that correspond to current and the next hashes.
    size_t* current = map->indicies.first + hash % indicies;
    size_t* next    = current + 1;

    DilIndexMapPairRange result = {
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
size_t* dil_index_map_at(DilIndexMap const* map, DilString const* key)
{
    DilIndexMapPairRange range =
        dil_index_map_range(map, dil_index_map_hash(key));
    DilIndexMapPair* pair = dil_index_map_range_find(&range, key);
    return &pair->value;
}

/* Whether the map contains the key. */
bool dil_index_map_contains(DilIndexMap const* map, DilString const* key)
{
    DilIndexMapPairRange range =
        dil_index_map_range(map, dil_index_map_hash(key));
    return dil_index_map_range_contains(&range, key);
}

/* Maximum allowed amount of keys whose hashes give the same index after modulus
 * operation. Decreases the speed of the map, but also the memory usage. */
#define DEFINE__MAX_COLLISION 1

/* Whether the map should be rehashed to fit the collision restriction. */
bool dil_index_map_need_rehash(DilIndexMap const* map)
{
    size_t indicies = dil_index_map_indicies(map);
    size_t pairs    = dil_index_map_size(map);
    for (size_t i = 0; i < indicies; i++) {
        DilIndexMapPairRange range = dil_index_map_range(map, i);
        size_t               size  = dil_index_map_range_size(&range);
        if (size > DEFINE__MAX_COLLISION) {
            return true;
        }
        pairs -= size;
        if (pairs <= DEFINE__MAX_COLLISION) {
            break;
        }
    }
    return false;
}

/* Grow the indicies. Doubles starting from 1. */
void dil_index_map_grow_indicies(DilIndexMap* map)
{
    size_t size    = dil_index_map_indicies(map);
    size_t newSize = size * 2;
    if (newSize == 0) {
        newSize = 1;
    }

    size_t* memory = realloc(map->indicies.first, newSize * sizeof(size_t*));

    map->indicies.first = memory;
    map->indicies.last  = memory + newSize;
}

/* Compare pairs by hash. */
int dil_index_map_compare(void* indiciesp, void const* lhs, void const* rhs)
{
    size_t leftHash  = dil_index_map_hash(&((DilIndexMapPair*)lhs)->key);
    size_t rightHash = dil_index_map_hash(&((DilIndexMapPair*)rhs)->key);
    size_t indicies  = *(size_t*)indiciesp;
    return (int)(leftHash % indicies - rightHash % indicies);
}

/* Recalculate indicies. */
void dil_index_map_recalculate(DilIndexMap* map)
{
    size_t index    = 0;
    size_t indicies = dil_index_map_indicies(map);
    size_t pairs    = dil_index_map_size(map);
    for (size_t i = 0; i < pairs; i++) {
        size_t current =
            dil_index_map_hash(&map->pairs.first[i].key) % indicies;
        while (current >= index) {
            map->indicies.first[index++] = i;
        }
    }
    while (index < indicies) {
        map->indicies.first[index++] = pairs;
    }
}

/* Rehash until the maximum collision restriction is obeyed. */
void dil_index_map_rehash(DilIndexMap* map)
{
    do {
        dil_index_map_grow_indicies(map);
        size_t indicies = dil_index_map_indicies(map);

        // Sort the pairs by hash.
        qsort_s(
            map->pairs.first,
            dil_index_map_size(map),
            sizeof(DilIndexMapPair),
            &dil_index_map_compare,
            &indicies);

        dil_index_map_recalculate(map);
    } while (dil_index_map_need_rehash(map));
}

/* Amount of allocated pairs. */
size_t dil_index_map_capacity(DilIndexMap const* map)
{
    return map->pairs.allocated - map->pairs.first;
}

/* Amount of allocated but unused pairs. */
size_t dil_index_map_space(DilIndexMap const* map)
{
    return map->pairs.allocated - map->pairs.last;
}

/* Make sure the amount of pairs will fit. Grows by at least half of the current
 * capacity if necessary. */
void dil_index_map_reserve(DilIndexMap* map, size_t amount)
{
    size_t space = dil_index_map_space(map);
    if (amount <= space) {
        return;
    }

    size_t growth       = amount - space;
    size_t capacity     = dil_index_map_capacity(map);
    size_t halfCapacity = capacity / 2;
    if (growth < halfCapacity) {
        growth = halfCapacity;
    }

    size_t           newCapacity = capacity + growth;
    DilIndexMapPair* memory =
        realloc(map->pairs.first, newCapacity * sizeof(DilIndexMapPair));

    map->pairs.last      = memory + dil_index_map_size(map);
    map->pairs.first     = memory;
    map->pairs.allocated = memory + newCapacity;
}

/* Put the pair to the index. */
void dil_index_map_put(DilIndexMap* map, size_t index, DilIndexMapPair pair)
{
    dil_index_map_reserve(map, 1);
    DilIndexMapPair* position = map->pairs.first + index;
    size_t           move     = map->pairs.last++ - position;
    if (move != 0) {
        memmove(position + 1, position, move * sizeof(DilIndexMapPair));
    }
    *position = pair;
}

/* Add a mapping to the value with the key. */
void dil_index_map_add(DilIndexMap* map, DilString key, size_t value)
{
    // Get the corresponding view for the hash.
    size_t               hash  = dil_index_map_hash(&key);
    DilIndexMapPairRange range = dil_index_map_range(map, hash);

    DilIndexMapPair pair     = {.key = key, .value = value};
    size_t          indicies = dil_index_map_indicies(map);

    // If it would not create too many collisions.
    if (dil_index_map_range_size(&range) < DEFINE__MAX_COLLISION &&
        indicies != 0) {
        dil_index_map_put(map, range.last - map->pairs.first, pair);

        // Increase indicies.
        for (size_t* i = map->indicies.first + hash % indicies + 1;
             i < map->indicies.last;
             i++) {
            (*i)++;
        }
    } else {
        // Just add to the end; rehash will handle the rest.
        dil_index_map_put(map, dil_index_map_size(map), pair);
        dil_index_map_rehash(map);
    }
}

/* Deallocate the memory. */
void dil_index_map_free(DilIndexMap* map)
{
    free(map->pairs.first);
    map->pairs.first     = NULL;
    map->pairs.last      = NULL;
    map->pairs.allocated = NULL;
    free(map->indicies.first);
    map->indicies.first = NULL;
    map->indicies.last  = NULL;
}
