set(ver ${CMAKE_CXX_COMPILER_VERSION})
set(id ${CMAKE_CXX_COMPILER_ID})

# must use a supported generator
if (NOT (CMAKE_GENERATOR MATCHES "^Visual Studio" OR
    CMAKE_GENERATOR MATCHES "Ninja"))
    message(FATAL_ERROR "You must use the Visual Studio or Ninja generator
        for C++20 module support.")
endif()

# AppleClang is entirely unsupported
if (id MATCHES "AppleClang")
    message(FATAL_ERROR "AppleClang is unsupported. Use Homebrew's Clang
        (brew install llvm) instead.")
endif()

# GCC must be 16+
if (id MATCHES "GNU" AND ver VERSION_LESS 16)
    message(FATAL_ERROR "Unsupported GCC version ${ver}. Only 16 and up
        are supported.")
endif()

# Clang must be 17+ (maybe 19+ idk)
if (id MATCHES "Clang")
    if(ver VERSION_LESS 17)
        message(FATAL_ERROR "Unsupported Clang version ${ver}. Only 17 and
            up are supported.")
    endif()

    # clang-cl
    # https://cmake.org/cmake/help/v4.4/release/4.4.html
    if (MSVC)
        # doesn't support Visual Studio
        if (CMAKE_GENERATOR MATCHES "^Visual Studio")
            message(FATAL_ERROR "clang-cl does not support the Visual Studio
                generator when C++20 modules are enabled. Use Ninja instead.")
        endif()

        # requires CMake 4.4 to scan for modules
        if (CMAKE_VERSION VERSION_LESS 4.4)
            message(FATAL_ERROR "clang-cl requires CMake 4.4 or newer for
                C++20 module support")
        endif()
    endif()
endif()

# MSVC must be 2022+ (19.39 specifically)
if (id MATCHES "MSVC" AND ver VERSION_LESS 19.39)
    message(FATAL_ERROR "Unsupported MSVC version ${ver}. Only 2022 and
        up (19.39+) are supported.")
endif()