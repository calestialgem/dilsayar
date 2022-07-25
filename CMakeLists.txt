# SPDX-FileCopyrightText: 2022 Cem Geçgel <gecgelcem@outlook.com>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)

project(sablon)

set(headers
    src/sln/lib.c
)

add_library(${PROJECT_NAME} INTERFACE ${headers})
target_include_directories(${PROJECT_NAME} INTERFACE src)

function(setup_target target)
    target_link_libraries(${target} PRIVATE ${PROJECT_NAME})

    # Enable all warnings.
    if(MSVC)
        target_compile_options(${target} PRIVATE /Wall)
    else(MSVC)
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wno-pragma-once-outside-header
        )
    endif(MSVC)
endfunction(setup_target target)

add_executable(debug src/main.c)
setup_target(debug)

# Create compile commands for the header files as well.
add_library(header OBJECT ${headers})
setup_target(header)
