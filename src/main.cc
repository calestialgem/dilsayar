// SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstddef>
#include <cstdio>
#include <cstdlib>

/* Start the program. */
int main(int argumentCount, char const* const* arguments)
{
    // Print the arguments.
    std::printf("Running with arguments:\n");
    for (int i = 0; i < argumentCount; i++) {
        std::printf("[%d] {%s}\n", i, arguments[i]);
    }
    std::printf("\n");

    if (argumentCount < 2) {
        std::printf("Provide a `.dil` file!\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
