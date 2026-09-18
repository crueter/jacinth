cmake_minimum_required(VERSION 3.31)

project(jacinth LANGUAGES C CXX)

include(CMakeDependentOption)

# options
option(JACINTH_BUILD_TESTS "Build Jacinth's test application"
    ${PROJECT_IS_TOP_LEVEL})
option(JACINTH_USE_REFLECTION
    "Use C++26 reflection features instead of Boost.PFR" OFF)
cmake_dependent_option(JACINTH_USE_SYSTEM_PFR
    "Use system Boost.PFR instead of bundled" ${PROJECT_IS_TOP_LEVEL}
    "NOT JACINTH_USE_REFLECTION" ON) # forcing to "on" enables install target
option(JACINTH_USE_SYSTEM_YYJSON "Use system yyjson instead of bundled"
    ${PROJECT_IS_TOP_LEVEL})

if (JACINTH_USE_SYSTEM_PFR AND JACINTH_USE_SYSTEM_YYJSON)
    option(JACINTH_INSTALL "Create a Jacinth install target"
        ${PROJECT_IS_TOP_LEVEL})
else()
    set(JACINTH_INSTALL OFF)
endif()

# cxx
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# reflection
# TODO: MSVC
if (JACINTH_USE_REFLECTION)
    set(CMAKE_CXX_STANDARD 26)
    add_compile_definitions(JACINTH_USE_REFLECTION)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-freflection>)
else()
    # pfr
    if(JACINTH_USE_SYSTEM_PFR)
        find_package(Boost COMPONENTS pfr REQUIRED)
    endif()

    if (NOT TARGET Boost::pfr)
        add_library(boost_pfr INTERFACE)
        target_include_directories(boost_pfr INTERFACE pfr/include)
        add_library(Boost::pfr ALIAS boost_pfr)
    endif()
endif()

# yyjson
if(JACINTH_USE_SYSTEM_YYJSON)
    find_package(yyjson REQUIRED)
endif()

if(NOT TARGET yyjson)
    add_library(yyjson OBJECT
        yyjson/src/yyjson.h yyjson/src/yyjson.c)
    target_include_directories(yyjson PUBLIC yyjson/src)
endif()

# jacinth module target
add_library(jacinth)

target_sources(jacinth PUBLIC
    FILE_SET jacinth TYPE CXX_MODULES FILES jacinth.ixx)

target_link_libraries(jacinth PUBLIC yyjson)
if (NOT JACINTH_USE_REFLECTION)
    target_link_libraries(jacinth PUBLIC Boost::pfr)
endif()

add_library(jacinth::jacinth ALIAS jacinth)

if (JACINTH_INSTALL)
    include(GNUInstallDirs)
    install(TARGETS jacinth
        EXPORT jacinth-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        FILE_SET jacinth DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jacinth)

    install(EXPORT jacinth-targets
        FILE jacinthConfig.cmake
        NAMESPACE jacinth::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/jacinth")
endif()

if (JACINTH_BUILD_TESTS)
    add_subdirectory(test)
endif()