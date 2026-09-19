cmake_minimum_required(VERSION 3.31)

project(jacinth_test LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)

option(JACINTH_USE_REFLECTION
    "Use C++26 reflection features instead of Boost.PFR" OFF)

if (PROJECT_IS_TOP_LEVEL)
    find_package(jacinth REQUIRED)
endif()

add_executable(test main.cpp)

# if this is built as a subproject jacinth won't pass its reflection flags
# this is intentional
if (JACINTH_USE_REFLECTION AND NOT PROJECT_IS_TOP_LEVEL)
    target_compile_definitions(test PRIVATE JACINTH_USE_REFLECTION)
    target_compile_options(test PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-freflection>)
    target_compile_features(test PRIVATE cxx_std_26)
endif()

target_link_libraries(test PRIVATE jacinth::jacinth)

file(COPY_FILE release.json ${CMAKE_CURRENT_BINARY_DIR}/release.json)
file(COPY_FILE cars.json ${CMAKE_CURRENT_BINARY_DIR}/cars.json)
