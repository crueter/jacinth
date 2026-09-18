cmake_minimum_required(VERSION 3.31)

project(jacinth LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(BOOST_USE_MODULES ON)
add_subdirectory(pfr)

set(YYJSON_INSTALL OFF)
add_subdirectory(yyjson)

add_executable(jacinth main.cpp)
target_link_libraries(jacinth PRIVATE Boost::pfr yyjson)

include(GNUInstallDirs)
install(TARGETS jacinth
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
