// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/ix.cc"
#include "dil/view.cc"

#include <cstdlib>
#include <cstring>

namespace dil {
/* Contiguous, dynamicly allocated elements. */
template<typename Element>
struct List {
    /* Border before the first element. */
    Element* first;
    /* Border after the last element. */
    Element* last;
    /* Border after the last allocated element. */
    Element* allocated;
};

/* Amount of elements. */
template<typename Element>
ix size(List<Element> const& list) {
    return list.last - list.first;
}

/* Amount of allocated elements. */
template<typename Element>
ix capacity(List<Element> const& list) {
    return list.allocated - list.first;
}

/* Amount of allocated but unused elements. */
template<typename Element>
ix space(List<Element> const& list) {
    return list.allocated - list.last;
}

/* Whether there are any elements. */
template<typename Element>
bool finite(List<Element> const& list) {
    return list.last > list.first;
}

/* Reference to the element at the index. */
template<typename Element>
Element& at(List<Element> const& list, ix index) {
    return list.first[index];
}

/* Make sure the amount of elements will fit. Grows by at least half if
 * necessary. */
template<typename Element>
void reserve(List<Element>& list, ix amount) {
    ix growth = amount - space(list);
    if (growth <= 0) {
        return;
    }
    ix oldCapacity = capacity(list);
    ix minGrowth   = oldCapacity / 2;
    if (growth < minGrowth) {
        growth = minGrowth;
    }
    ix       newCapacity = oldCapacity + growth;
    Element* memory = std::realloc(list.first, newCapacity * sizeof(Element));
    list.allocated  = memory + newCapacity;
    list.last       = memory + size(list);
    list.first      = memory;
}

/* Add the element to the end. */
template<typename Element>
void add(List<Element>& list, Element const& element) {
    reserve(list, 1);
    *list.last++ = element;
}

/* Add the elements to the end. */
template<typename Element>
void add_view(List<Element>& list, View<Element> const& view) {
    ix amount = size(view);
    reserve(list, amount);
    std::memmove(list.last, view.first, amount * sizeof(Element));
    list.last += amount;
}

/* Open space at the index for the amount of elements. Returns pointer to the
 * first opened position. */
template<typename Element>
Element* open(List<Element>& list, ix index, ix amount) {
    reserve(list, amount);
    Element* first = list.first + index;
    std::memmove(first + amount, first, amount * sizeof(Element));
    list.last += amount;
    return first;
}

/* Put the element to the index. */
template<typename Element>
void put(List<Element>& list, ix index, Element const& element) {
    *open(list, index, 1) = element;
}

/* Put the elements to the index. */
template<typename Element>
void put_view(List<Element>& list, ix index, View<Element> const& view) {
    ix amount = size(view);
    std::memmove(
        open(list, index, amount),
        view.first,
        amount * sizeof(Element));
}

/* Remove the element at the end. */
template<typename Element>
void remove(List<Element>& list) {
    list.last--;
}

/* Remove the element at the index. */
template<typename Element>
void remove_at(List<Element>& list, ix index) {
    std::memmove(
        list.first + index,
        list.first + index + 1,
        (size(list) - index - 1) * sizeof(Element));
    list.last--;
}

/* Remove the element from the end and return it. */
template<typename Element>
Element pop(List<Element>& list) {
    remove(list);
    return *list.last;
}

/* Remove all the elements. */
template<typename Element>
void clear(List<Element>& list) {
    list.last = list.first;
}

/* Deallocate memory. */
template<typename Element>
void free(List<Element>& list) {
    std::free(list.first);
    list.first     = nullptr;
    list.last      = nullptr;
    list.allocated = nullptr;
}
} // namespace dil
