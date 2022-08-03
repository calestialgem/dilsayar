// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/ix.cc"

namespace dil {
/* Immutable pointers to contiguous elements. */
template<typename Element>
struct View {
    /* Border before the first element. */
    Element const* first;
    /* Border after the last element. */
    Element const* last;
};

/* Amount of elements. */
template<typename Element>
ix size(View<Element> const& view) {
    return view.last - view.first;
}

/* Whether there are any elements. */
template<typename Element>
bool finite(View<Element> const& view) {
    return view.last > view.last;
}

/* Reference to the element at the index. */
template<typename Element>
Element const& at(View<Element> const& view, ix index) {
    return *(view.first + index);
}

/* Reference to the element at the index from behind. */
template<typename Element>
Element const& back(View<Element> const& view, ix index) {
    return *(view.last - 1 - index);
}

/* Find the first occurance of the element. Returns the position after the last
 * element if it does not exist. */
template<typename Element>
Element const* first(View<Element> const& view, Element const& element) {
    for (Element const* i = view.first; i < view.last; i++) {
        if (equal(*i, element)) {
            return i;
        }
    }
    return view.last;
}

/* Find the first element that fits the predicate. Returns the position after
 * the last element if none fits. */
template<typename Element, typename Predicate>
Element const* first_fit(View<Element> const& view, Predicate predicate) {
    for (Element const* i = view.first; i < view.last; i++) {
        if (predicate(*i)) {
            return i;
        }
    }
    return view.last;
}

/* Find the last occurance of the element. Returns the position before the first
 * element if it does not exist. */
template<typename Element>
Element const* last(View<Element> const& view, Element const& element) {
    for (Element const* i = view.last - 1; i >= view.first; i--) {
        if (equal(*i, element)) {
            return i;
        }
    }
    return view.first - 1;
}

/* Find the last element that fits the predicate. Returns the position before
 * the first element if none fits. */
template<typename Element, typename Predicate>
Element const* last_fit(View<Element> const& view, Predicate predicate) {
    for (Element const* i = view.last - 1; i >= view.first; i--) {
        if (predicate(*i)) {
            return i;
        }
    }
    return view.first - 1;
}

/* Whether the view contains the element. */
template<typename Element>
bool contains(View<Element> const& view, Element const& element) {
    return first(view, element) != view.last;
}

/* Whether the view contains an element that fits the predicate. */
template<typename Element, typename Predicate>
bool contains_fit(View<Element> const& view, Predicate predicate) {
    return first(view, predicate) != view.last;
}

/* Whether the first element equals to the given element. */
template<typename Element>
bool starts(View<Element> const& view, Element const& element) {
    return equal(at(view, 0), element);
}

/* Whether the first element fits the predicate. */
template<typename Element, typename Predicate>
bool starts_fit(View<Element> const& view, Predicate predicate) {
    return predicate(at(view, 0));
}

/* Whether the last element equals to the given element. */
template<typename Element>
bool finishes(View<Element> const& view, Element const& element) {
    return equal(back(view, 0), element);
}

/* Whether the last element fits the predicate. */
template<typename Element, typename Predicate>
bool finishes_fit(View<Element> const& view, Predicate predicate) {
    return predicate(back(view, 0));
}

/* Whether the views point to same elements. */
template<typename Element>
bool equal(View<Element> const& lhs, View<Element> const& rhs) {
    ix size = size(lhs);
    if (size != size(rhs)) {
        return false;
    }
    for (ix i = 0; i < size; i++) {
        if (!equal(at(lhs, i), at(rhs, i))) {
            return false;
        }
    }
    return true;
}
} // namespace dil
