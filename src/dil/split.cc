// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/ix.cc"
#include "dil/view.cc"

namespace dil {
/* Two parts of a view split at a border. */
template<typename Element>
struct Split {
    /* Elements before the border. */
    View<Element> before;
    /* Elements after the border. */
    View<Element> after;
};

/* Split at the position. */
template<typename Element>
Split<Element> split(View<Element> const& view, Element const* position) {
    return {
        .before = {.first = view.first,  .last = position},
        .after  = {  .first = position, .last = view.last}
    };
}

/* Split at the index. */
template<typename Element>
Split<Element> split_at(View<Element> const& view, ix index) {
    return split(view, at(view, index));
}

/* Split at the first occurence of the element. */
template<typename Element>
Split<Element> split_first(View<Element> const& view, Element const& element) {
    return split(view, first(view, element));
}

/* Split at the first element that fits the predicate. */
template<typename Element, typename Predicate>
Split<Element> split_first_fit(View<Element> const& view, Predicate predicate) {
    return split_fit(view, first_fit(view, predicate));
}

/* Split at the last occurence of the element. */
template<typename Element>
Split<Element> split_last(View<Element> const& view, Element const& element) {
    return split(view, last(view, element));
}

/* Split at the last element that fits the predicate. */
template<typename Element, typename Predicate>
Split<Element> split_last_fit(View<Element> const& view, Predicate predicate) {
    return split(view, last_fit(view, predicate));
}
} // namespace dil
