// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dil/ix.cc"

namespace dil {
/* Hashing method. */
struct Hasher {
    /* Seed of the hashing process. */
    ix static constexpr SEED = 131;

    /* Current value of the hash. */
    ix hash;
};

/* Start the hash process. */
void init(Hasher& hasher) {
    hasher.hash = Hasher::SEED;
}

/* Hash the integer. */
void hash(ix hash, Hasher& hasher) {
    hasher.hash *= Hasher::SEED;
    hasher.hash += hash;
}
} // namespace dil
