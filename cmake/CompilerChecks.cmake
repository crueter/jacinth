set(ver ${CMAKE_CXX_COMPILER_VERSION})
set(id ${CMAKE_CXX_COMPILER_ID})

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
if (id MATCHES "Clang" AND ver VERSION_LESS 17)
    message(FATAL_ERROR "Unsupported Clang version ${ver}. Only 17 and
        up are supported.")
endif()

# MSVC must be 2022+ (19.39 specifically)
if (id MATCHES "MSVC" AND ver VERSION_LESS 19.39)
    message(FATAL_ERROR "Unsupported MSVC version ${ver}. Only 2022 and
        up (19.39+) are supported.")
endif()