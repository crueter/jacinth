cmake_minimum_required(VERSION 3.31)

project(jacinth VERSION 0.1.0 LANGUAGES C CXX)

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
if (NOT DEFINED CMAKE_CXX_STANDARD OR "${CMAKE_CXX_STANDARD}" LESS "23")
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# TODO: better control flow
if (JACINTH_USE_REFLECTION)
    set(CMAKE_CXX_STANDARD 26)
else()
    # pfr
    if(JACINTH_USE_SYSTEM_PFR)
        find_package(Boost REQUIRED COMPONENTS headers)
    endif()

    if (TARGET Boost::headers AND NOT TARGET Boost::pfr)
        add_library(Boost::pfr ALIAS Boost::headers)
    elseif (NOT TARGET Boost::pfr)
        add_library(boost_pfr INTERFACE)
        target_include_directories(boost_pfr INTERFACE externals/pfr/include)
        add_library(Boost::pfr ALIAS boost_pfr)
    endif()
endif()

# yyjson
if(JACINTH_USE_SYSTEM_YYJSON)
    find_package(yyjson REQUIRED)
else()
    add_library(yyjson OBJECT
        externals/yyjson/src/yyjson.h externals/yyjson/src/yyjson.c)
    target_include_directories(yyjson PUBLIC externals/yyjson/src)
endif()

# jacinth module target
if (BUILD_SHARED_LIBS)
    set(lib_type SHARED)
else()
    set(lib_type STATIC)
endif()

add_library(jacinth)

if (JACINTH_USE_REFLECTION)
    target_compile_definitions(jacinth PUBLIC
        $<BUILD_INTERFACE:JACINTH_USE_REFLECTION>)
    target_compile_options(jacinth PUBLIC
        $<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:CXX>:-freflection>>)
    target_compile_features(jacinth PUBLIC
        $<BUILD_INTERFACE:cxx_std_26>)
endif()

target_sources(jacinth PUBLIC
    FILE_SET jacinth TYPE CXX_MODULES FILES
        modules/jacinth.ixx
        modules/spec.ixx
        modules/write.ixx
        modules/read.ixx
        modules/iter.ixx
        modules/err.ixx)

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
        FILE jacinthTargets.cmake
        NAMESPACE jacinth::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/jacinth")

    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        "${CMAKE_SOURCE_DIR}/cmake/jacinthConfig.cmake.in"
        "${CMAKE_BINARY_DIR}/jacinthConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jacinth)

    write_basic_package_version_file(
        "${CMAKE_BINARY_DIR}/jacinthConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

    install(FILES
        "${CMAKE_BINARY_DIR}/jacinthConfigVersion.cmake"
        "${CMAKE_BINARY_DIR}/jacinthConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jacinth)
endif()

if (JACINTH_BUILD_TESTS)
    add_subdirectory(test)
endif()