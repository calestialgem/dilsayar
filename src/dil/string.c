// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdbool.h>
#include <stddef.h>

/* Immutable pointers to contiguous elements. */
typedef struct {
    /* Border before the first character. */
    char const* first;
    /* Border after the last character. */
    char const* last;
} DilString;

/* Convert a null terminated string. */
DilString dil_string_terminated(char const* array)
{
    DilString string = {.first = array, .last = array};
    while (*string.last) {
        string.last++;
    }
    return string;
}

/* Amount of elements. */
size_t dil_string_size(DilString const* view)
{
    return view->last - view->first;
}

/* Whether there are any elements. */
bool dil_string_finite(DilString const* view)
{
    return dil_string_size(view) > 0;
}

/* Pointer to the element at the index. */
char const* dil_string_at(DilString const* view, size_t index)
{
    return view->first + index;
}

/* Element at the index. */
char dil_string_get(DilString const* view, size_t index)
{
    return *dil_string_at(view, index);
}

/* Pointer to the first element. */
char const* dil_string_start(DilString const* view)
{
    return view->first;
}

/* Pointer to the last element. */
char const* dil_string_finish(DilString const* view)
{
    return view->last - 1;
}

/* Find the first occurance of the element. Returns the position after the last
 * element if it does not exist. */
char const* dil_string_first(DilString const* view, char element)
{
    for (char const* i = view->first; i < view->last; i++) {
        if (*i == element) {
            return i;
        }
    }
    return view->last;
}

/* Find the first element that fits the predicate. Returns the position after
 * the last character if none fits it. */
char const* dil_string_first_fit(DilString const* view, bool (*predicate)(char))
{
    for (char const* i = view->first; i < view->last; i++) {
        if (predicate(*i)) {
            return i;
        }
    }
    return view->last;
}

/* Find the last occurance of the element. Returns the position before the first
 * element if it does not exist. */
char const* dil_string_last(DilString const* view, char element)
{
    for (char const* i = view->last - 1; i >= view->first; i--) {
        if (*i == element) {
            return i;
        }
    }
    return view->first - 1;
}

/* Find the last element that fits the predicate. Returns the position before
 * the first character if none fits it. */
char const* dil_string_last_fit(DilString const* view, bool (*predicate)(char))
{
    for (char const* i = view->last - 1; i >= view->first; i--) {
        if (predicate(*i)) {
            return i;
        }
    }
    return view->first - 1;
}

/* Whether view contains the element. */
bool dil_string_contains(DilString const* view, char element)
{
    return dil_string_first(view, element) != view->last;
}

/* Whether the view contains an element that fits the predicate. */
bool dil_string_contains_fit(DilString const* view, bool (*predicate)(char))
{
    return dil_string_first_fit(view, predicate) != view->last;
}

/* Whether the first element equals to the given. */
bool dil_string_starts(DilString const* view, char element)
{
    return *view->first == element;
}

/* Whether the first element fits the predicate. */
bool dil_string_starts_fit(DilString const* view, bool (*predicate)(char))
{
    return predicate(*view->first);
}

/* Whether the last element equals to the given. */
bool dil_string_finishes(DilString const* view, char element)
{
    return *(view->last - 1) == element;
}

/* Whether the last element fits the predicate. */
bool dil_string_finishes_fit(DilString const* view, bool (*predicate)(char))
{
    return predicate(*(view->last - 1));
}

/* Remove the elements from the ends if they exist. */
void dil_string_unwrap(DilString* string, char opening, char closing)
{
    bool starts   = dil_string_starts(string, opening);
    bool finishes = dil_string_finishes(string, closing);
    if (starts && finishes) {
        string->first++;
        string->last--;
    }
}

/* Whether the views are the same. */
bool dil_string_equal(DilString const* lhs, DilString const* rhs)
{
    size_t size = dil_string_size(lhs);
    if (size != dil_string_size(rhs)) {
        return false;
    }
    for (size_t i = 0; i < size; i++) {
        if (lhs->first[i] != rhs->first[i]) {
            return false;
        }
    }
    return true;
}

/* View split at a border. */
typedef struct {
    /* Elements before the border. */
    DilString before;
    /* Elements after the border. */
    DilString after;
} DilSplit;

/* Split at the position. */
DilSplit dil_string_split_position(DilString const* view, char const* position)
{
    return (DilSplit){
        .before = {.first = view->first,   .last = position},
        .after  = {   .first = position, .last = view->last}
    };
}

/* Split at the index. */
DilSplit dil_string_split_index(DilString const* view, size_t index)
{
    return dil_string_split_position(view, view->first + index);
}

/* Split at the first occurence of the element. */
DilSplit dil_string_split_first(DilString const* view, char element)
{
    return dil_string_split_position(view, dil_string_first(view, element));
}

/* Split at the first element that fits the predicate. */
DilSplit
dil_string_split_first_fit(DilString const* view, bool (*predicate)(char))
{
    return dil_string_split_position(
        view,
        dil_string_first_fit(view, predicate));
}

/* Split at the last occurence of the element. */
DilSplit dil_string_split_last(DilString const* view, char element)
{
    return dil_string_split_position(view, dil_string_last(view, element) + 1);
}

/* Split at the last element that fits the predicate. */
DilSplit
dil_string_split_last_fit(DilString const* view, bool (*predicate)(char))
{
    return dil_string_split_position(
        view,
        dil_string_last_fit(view, predicate) + 1);
}

/* Return a view upto the position from the begining of the view. Removes those
 * elements from the view. */
DilString dil_string_lead_position(DilString* view, char const* position)
{
    DilString prefix = {.first = view->first, .last = position};
    view->first      = position;
    return prefix;
}

/* Return a view to the amount of elements from the begining of the view.
 * Removes those elements from the view. */
DilString dil_string_lead_amount(DilString* view, size_t amount)
{
    return dil_string_lead_position(view, view->first + amount);
}

/* Return a view of elements upto the first occurence of the element from the
 * begining of the view. Removes those elements from the view. */
DilString dil_string_lead_first(DilString* view, char element)
{
    return dil_string_lead_position(view, dil_string_first(view, element));
}

/* Return a view of elements upto the first element that fits the predicate from
 * the begining of the view. Removes those elements from the view. */
DilString dil_string_lead_first_fit(DilString* view, bool (*predicate)(char))
{
    return dil_string_lead_position(
        view,
        dil_string_first_fit(view, predicate));
}

/* Whether the view starts with the element. Consumes the element when true. */
bool dil_string_prefix_element(DilString* string, char element)
{
    if (dil_string_finite(string) && dil_string_starts(string, element)) {
        string->first++;
        return true;
    }
    return false;
}

/* Whether the view starts with an element of the set. Consumes the element when
 * true. */
bool dil_string_prefix_set(DilString* string, DilString const* set)
{
    if (dil_string_finite(string) && dil_string_contains(set, *string->first)) {
        string->first++;
        return true;
    }
    return false;
}

/* Whether the view does not start with an element of the set. Consumes the
 * element when true. */
bool dil_string_prefix_not_set(DilString* string, DilString const* set)
{
    if (dil_string_finite(string) &&
        !dil_string_contains(set, *string->first)) {
        string->first++;
        return true;
    }
    return false;
}

/* Whether the view starts with the prefix. Consumes the prefix when true. */
bool dil_string_prefix_check(DilString* string, DilString const* prefix)
{
    size_t size = dil_string_size(prefix);
    if (dil_string_size(string) < size) {
        return false;
    }
    DilSplit split = dil_string_split_index(string, size);
    if (!dil_string_equal(&split.before, prefix)) {
        return false;
    }
    *string = split.after;
    return true;
}
