get_target_property(launcher jacinth CXX_COMPILER_LAUNCHER)
if ("${launcher}" MATCHES "ccache")
    set_target_properties(jacinth PROPERTIES
        CXX_COMPILER_LAUNCHER ""
        C_COMPILER_LAUNCHER "")
endif()