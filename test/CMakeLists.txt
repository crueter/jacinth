# SPDX-FileCopyrightText: Copyright 2026 crueter
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.31)

enable_testing()

project(jacinth_test LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)

option(JACINTH_USE_REFLECTION
    "Use C++26 reflection features instead of Boost.PFR" OFF)

if (PROJECT_IS_TOP_LEVEL)
    find_package(jacinth REQUIRED)
endif()

# test lib
add_library(tests STATIC)

target_sources(tests PUBLIC
    FILE_SET tests TYPE CXX_MODULES FILES tests.ixx)

target_link_libraries(tests PUBLIC jacinth::jacinth)
if (MSVC)
    target_compile_options(tests PUBLIC
        /W4 /WX /we4834)
else()
    target_compile_options(tests PUBLIC
        -Werror=unused-result -Wall -Wextra -Werror
        -Wno-c23-extensions)
endif()

# awesome cmake moment
if (MSVC AND CMAKE_CXX_COMPILER MATCHES "Clang")
    target_compile_options(tests PUBLIC -Wno-unused-command-line-argument)
endif()


set(TESTS meta numvec errors array adl move release map cars)

foreach(test ${TESTS})
    add_executable(${test} cases/${test}.cpp)
    target_link_libraries(${test} PRIVATE tests)

    add_test(NAME ${test} COMMAND ${test})
endforeach()
