# SPDX-FileCopyrightText: Copyright 2026 crueter
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.31)

project(jacinth_test LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)

option(JACINTH_USE_REFLECTION
    "Use C++26 reflection features instead of Boost.PFR" OFF)

if (PROJECT_IS_TOP_LEVEL)
    find_package(jacinth REQUIRED)
endif()

# main test
add_executable(test main.cpp)

target_link_libraries(test PRIVATE jacinth::jacinth)
target_compile_options(test PRIVATE -Werror=unused-result -Wall -Wextra -Werror)

# meta spec test
add_executable(meta meta.cpp)

target_link_libraries(meta PRIVATE jacinth::jacinth)
target_compile_options(meta PRIVATE -Werror=unused-result -Wall -Wextra -Werror)

# jsons
file(COPY_FILE release.json ${CMAKE_CURRENT_BINARY_DIR}/release.json)
file(COPY_FILE cars.json ${CMAKE_CURRENT_BINARY_DIR}/cars.json)
file(COPY_FILE room.json ${CMAKE_CURRENT_BINARY_DIR}/room.json)
