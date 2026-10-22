# Only supported on GCC (16 is already the minimum, see CompilerChecks)
if (NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(FATAL_ERROR "Reflection is only supported on GCC."
        "Turn off JACINTH_USE_REFLECTION.")
endif()

# 16.1 and below are unstable, but may work on e.g. Gentoo
if (CMAKE_CXX_COMPILER_VERSION LESS 16.2)
    message(WARNING "GCC 16.1 and below are unstable when using reflection
        and modules together. Consider upgrading GCC or disabling
        reflection if you encounter compile errors.")
endif()

macro(EnableReflection target)
    target_compile_definitions(${target} PUBLIC
        $<BUILD_INTERFACE:JACINTH_USE_REFLECTION>)
    target_compile_options(${target} PUBLIC
        $<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:CXX>:-freflection>>)
    target_compile_features(${target} PUBLIC
        $<BUILD_INTERFACE:cxx_std_26>)
endmacro()