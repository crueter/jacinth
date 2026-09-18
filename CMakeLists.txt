cmake_minimum_required(VERSION 3.31)

project(jacinth LANGUAGES CXX)

# options
option(JACINTH_BUILD_TESTS "Build Jacinth's test application" ON)
option(JACINTH_USE_SYSTEM_PFR "Use system Boost.PFR instead of bundled"
    ${PROJECT_IS_TOP_LEVEL})
option(JACINTH_USE_SYSTEM_YYJSON "Use system yyjson instead of bundled"
    ${PROJECT_IS_TOP_LEVEL})

if (JACINTH_USE_SYSTEM_PFR AND JACINTH_USE_SYSTEM_YYJSON)
    option(JACINTH_INSTALL "Create a Jacinth install target"
        ${PROJECT_IS_TOP_LEVEL})
endif()

# cxx
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# pfr
if(JACINTH_USE_SYSTEM_PFR)
    find_package(Boost COMPONENTS pfr REQUIRED)
else()
    add_library(boost_pfr INTERFACE)
    target_include_directories(boost_pfr INTERFACE pfr/include)
    add_library(Boost::pfr ALIAS boost_pfr)
endif()

# yyjson
if(JACINTH_USE_SYSTEM_YYJSON)
    find_package(yyjson REQUIRED)
else()
    add_library(yyjson OBJECT
        yyjson/src/yyjson.h yyjson/src/yyjson.c)
    target_include_directories(yyjson PUBLIC yyjson/src)
endif()

# jacinth module target
add_library(jacinth)
target_sources(jacinth PUBLIC
    FILE_SET jacinth TYPE CXX_MODULES FILES jacinth.ixx)
target_link_libraries(jacinth PUBLIC yyjson Boost::pfr)
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