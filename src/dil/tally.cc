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
struct Tally {
    /* Border before the first element. */
    Element* first;
    /* Border after the last element. */
    Element* last;
};

/* Amount of elements. */
template<typename Element>
ix size(Tally<Element> const& tally) {
    return tally.last - tally.first;
}

/* Whether there are any elements. */
template<typename Element>
bool finite(Tally<Element> const& tally) {
    return tally.last > tally.first;
}

/* Reference to the element at the index. */
template<typename Element>
Element& at(Tally<Element> const& tally, ix index) {
    return tally.first[index];
}

/* Grow by the amount of elements. Returns pointer to the first growed position.
 */
template<typename Element>
Element* grow(Tally<Element>& tally, ix amount) {
    ix       oldSize = size(tally);
    ix       newSize = oldSize + amount;
    Element* memory  = std::realloc(tally.first, newSize * sizeof(Element));
    tally.last       = memory + newSize;
    tally.first      = memory;
}

/* Add the element to the end. */
template<typename Element>
void add(Tally<Element>& tally, Element const& element) {
    *grow(tally, 1) = element;
}

/* Add the elements to the end. */
template<typename Element>
void add_view(Tally<Element>& tally, View<Element> const& view) {
    ix amount = size(view);
    std::memmove(grow(tally, amount), view.first, amount * sizeof(Element));
}

/* Open space at the index for the amount of elements. Returns pointer to the
 * first opened position. */
template<typename Element>
Element* open(Tally<Element>& tally, ix index, ix amount) {
    grow(tally, amount);
    Element* first = tally.first + index;
    std::memmove(first + amount, first, amount * sizeof(Element));
    tally.last += amount;
    return first;
}

/* Put the element to the index. */
template<typename Element>
void put(Tally<Element>& tally, ix index, Element const& element) {
    *open(tally, index, 1) = element;
}

/* Put the elements to the index. */
template<typename Element>
void put_view(Tally<Element>& tally, ix index, View<Element> const& view) {
    ix amount = size(view);
    std::memmove(
        open(tally, index, amount),
        view.first,
        amount * sizeof(Element));
}

/* Remove the element at the end. */
template<typename Element>
void remove(Tally<Element>& tally) {
    tally.last--;
}

/* Remove the element at the index. */
template<typename Element>
void remove_at(Tally<Element>& tally, ix index) {
    std::memmove(
        tally.first + index,
        tally.first + index + 1,
        (size(tally) - index - 1) * sizeof(Element));
    tally.last--;
}

/* Remove the element from the end and return it. */
template<typename Element>
Element pop(Tally<Element>& tally) {
    remove(tally);
    return *tally.last;
}

/* Deallocate memory. */
template<typename Element>
void free(Tally<Element>& tally) {
    std::free(tally.first);
    tally.first = nullptr;
    tally.last  = nullptr;
}
} // namespace dil
