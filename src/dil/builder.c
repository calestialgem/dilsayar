// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/indices.c"
#include "dil/object.c"
#include "dil/string.c"
#include "dil/tree.c"

/* Tree builder. */
typedef struct {
    /* Built tree. */
    DilTree* built;
    /* Stack of indicies to parents. */
    DilIndices parents;
} DilBuilder;

/* Push the last added object as the parent. */
void dil_builder_push(DilBuilder* builder)
{
    dil_indices_add(&builder->parents, dil_tree_size(builder->built) - 1);
}

/* Pop the pushed parent. */
void dil_builder_pop(DilBuilder* builder)
{
    dil_indices_remove(&builder->parents);
}

/* Add childeren to the last pushed object. */
void dil_builder_add(DilBuilder* builder, DilObject object)
{
    dil_tree_add(builder->built, (DilNode){.object = object});
    dil_tree_at(builder->built, *dil_indices_finish(&builder->parents))
        ->childeren++;
}

/* Remove the parents. Keeps the memory. */
void dil_builder_clear(DilBuilder* builder)
{
    dil_indices_clear(&builder->parents);
}

/* Deallocate memory. */
void dil_builder_free(DilBuilder* builder)
{
    dil_indices_free(&builder->parents);
}
