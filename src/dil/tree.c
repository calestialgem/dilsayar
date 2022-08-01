// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/buffer.c"
#include "dil/indices.c"
#include "dil/object.c"
#include "dil/string.c"

#include <Windows.h>
#include <errhandlingapi.h>
#include <fileapi.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Contiguous, dynamicly allocated elements. */
typedef struct {
    /* Border before the first element. */
    DilNode* first;
    /* Border after the last element. */
    DilNode* last;
    /* Border after the last allocated element. */
    DilNode* allocated;
} DilTree;

/* Amount of elements. */
size_t dil_tree_size(DilTree const* list)
{
    return list->last - list->first;
}

/* Amount of allocated elements. */
size_t dil_tree_capacity(DilTree const* list)
{
    return list->allocated - list->first;
}

/* Amount of allocated but unused elements. */
size_t dil_tree_space(DilTree const* list)
{
    return list->allocated - list->last;
}

/* Whether there are any elements. */
bool dil_tree_finite(DilTree const* list)
{
    return dil_tree_size(list) > 0;
}

/* Pointer to the element at the index. */
DilNode* dil_tree_at(DilTree const* list, size_t index)
{
    return list->first + index;
}

/* Element at the index. */
DilNode dil_tree_get(DilTree const* list, size_t index)
{
    return *dil_tree_at(list, index);
}

/* Pointer to the first element. */
DilNode* dil_tree_start(DilTree const* list)
{
    return list->first;
}

/* Pointer to the last element. */
DilNode* dil_tree_finish(DilTree const* list)
{
    return list->last - 1;
}

/* Make sure the amount of elements will fit. Grows by at least half if
 * necessary. */
void dil_tree_reserve(DilTree* list, size_t amount)
{
    size_t space = dil_tree_space(list);
    if (amount <= space) {
        return;
    }

    size_t growth       = amount - space;
    size_t capacity     = dil_tree_capacity(list);
    size_t halfCapacity = capacity / 2;
    if (growth < halfCapacity) {
        growth = halfCapacity;
    }

    size_t   newCapacity = capacity + growth;
    DilNode* memory      = realloc(list->first, newCapacity * sizeof(DilNode));

    list->last      = memory + dil_tree_size(list);
    list->first     = memory;
    list->allocated = memory + newCapacity;
}

/* Add the element to the end. */
void dil_tree_add(DilTree* list, DilNode element)
{
    dil_tree_reserve(list, 1);
    *list->last++ = element;
}

/* Open space at the index for the amount of element. Returns pointer to the
 * first opened element. */
DilNode* dil_tree_open(DilTree* list, size_t index, size_t amount)
{
    dil_tree_reserve(list, amount);
    DilNode* position = list->first + index;
    memmove(position + amount, position, amount * sizeof(DilNode));
    list->last += amount;
    return position;
}

/* Put the element to the given index. */
void dil_tree_put(DilTree* list, size_t index, DilNode element)
{
    *dil_tree_open(list, index, 1) = element;
}

/* Place the element the amount of times to the end. */
void dil_tree_place(DilTree* list, size_t amount, DilNode element)
{
    dil_tree_reserve(list, amount);
    for (size_t i = 0; i < amount; i++) {
        *list->last++ = element;
    }
}

/* Remove from the end. */
void dil_tree_remove(DilTree* list)
{
    list->last--;
}

/* Remove from the end and return the removed element. */
DilNode dil_tree_pop(DilTree* list)
{
    dil_tree_remove(list);
    return *list->last;
}

/* Remove all the elements. Keeps the memory. */
void dil_tree_clear(DilTree* list)
{
    list->last = list->first;
}

/* Deallocate memory. */
void dil_tree_free(DilTree* list)
{
    free(list->first);
    list->first     = NULL;
    list->last      = NULL;
    list->allocated = NULL;
}

/* Whether the subtrees are equal. */
bool dil_tree_equal(DilNode const* lhs, DilNode const* rhs)
{
    if (lhs->childeren != rhs->childeren) {
        return false;
    }
    if (lhs->childeren == 0) {
        return lhs->object.symbol == rhs->object.symbol &&
               dil_string_equal(&lhs->object.value, &rhs->object.value);
    }
    for (size_t i = 0; i < lhs->childeren; i++) {
        if (!dil_tree_equal(lhs + 1 + i, rhs + 1 + i)) {
            return false;
        }
    }
    return true;
}

/* Whether the subtrees at the given indicies are equal. */
bool dil_tree_equal_sub(DilTree const* tree, size_t lhs, size_t rhs)
{
    DilNode const* lhsp = dil_tree_at(tree, lhs);
    DilNode const* rhsp = dil_tree_at(tree, rhs);
    return dil_tree_equal(lhsp, rhsp);
}

/* Print pipes and object if its given. */
void dil_tree_print_branch(FILE* stream, int pipes, DilObject const* object)
{
    for (int i = 0; i < pipes; i++) {
        (void)fprintf(stream, "|   ");
    }
    if (object != NULL) {
        if (pipes > -1) {
            (void)fprintf(stream, "+- ");
        }
        dil_object_print(stream, object);
    }
    (void)fprintf(stream, "\n");
}

/* Print the tree. */
void dil_tree_print(FILE* stream, DilTree const* tree)
{
    DilIndices childeren = {0};

    for (size_t current = 0; current < dil_tree_size(tree); current++) {
        DilNode const* node  = dil_tree_at(tree, current);
        int            depth = (int)dil_indices_size(&childeren);
        dil_tree_print_branch(stream, depth - 1, &node->object);

        if (depth > 0) {
            (*dil_indices_finish(&childeren))--;
        }

        if (node->childeren > 0) {
            dil_tree_print_branch(stream, depth + 1, NULL);
            dil_indices_add(&childeren, node->childeren);
        } else {
            bool closed = false;
            while (dil_indices_finite(&childeren) &&
                   *dil_indices_finish(&childeren) == 0) {
                dil_indices_remove(&childeren);
                closed = true;
            }
            if (closed) {
                depth = (int)dil_indices_size(&childeren);
                dil_tree_print_branch(stream, depth, NULL);
            }
        }
    }

    dil_indices_free(&childeren);
}

/* Print the tree to the default file. */
void dil_tree_print_file(DilTree const* tree, DilString const* path)
{
    DilString const EXTENSION       = dil_string_terminated("_parse.txt");
    DilString const BUILD_DIRECTORY = dil_string_terminated("build\\");
    DilSplit        pathSplit       = dil_string_split_last(path, '\\');
    DilBuffer       buffer          = {0};

    // Remove extension `.dil`.
    pathSplit.after.last -= 4;

    dil_buffer_reserve(
        &buffer,
        1 + dil_string_size(&BUILD_DIRECTORY) +
            dil_string_size(&pathSplit.before));
    buffer.last += sprintf(
        buffer.last,
        "%.*s%.*s",
        (int)dil_string_size(&BUILD_DIRECTORY),
        BUILD_DIRECTORY.first,
        (int)dil_string_size(&pathSplit.before),
        pathSplit.before.first);

    if (!CreateDirectory(buffer.first, NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS) {
        printf(
            "%.*s: error: Could not create the build directory `%s`!\n",
            (int)dil_string_size(path),
            path->first,
            buffer.first);
        return;
    }

    dil_buffer_reserve(
        &buffer,
        1 + dil_string_size(&EXTENSION) + dil_string_size(&pathSplit.after));
    buffer.last += sprintf(
        buffer.last,
        "%.*s%.*s",
        (int)dil_string_size(&pathSplit.after),
        pathSplit.after.first,
        (int)dil_string_size(&EXTENSION),
        EXTENSION.first);

    FILE* outputParseStream = fopen(buffer.first, "w");
    if (outputParseStream == NULL) {
        printf(
            "%.*s: error: Could not open the output file `%s`!\n",
            (int)dil_string_size(path),
            path->first,
            buffer.first);
        return;
    }
    dil_buffer_free(&buffer);
    dil_tree_print(outputParseStream, tree);
    (void)fclose(outputParseStream);
}
