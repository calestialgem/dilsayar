// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>

namespace dil {
/* Index type. Signed integer that can hold differences between pointers, and
 * sizes of contiguous chunks of memory. */
using ix = std::ptrdiff_t;
} // namespace dil
