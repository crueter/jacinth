add_executable(test main.cpp)
target_link_libraries(test PRIVATE jacinth::jacinth)

file(COPY_FILE release.json ${CMAKE_CURRENT_BINARY_DIR}/release.json)
file(COPY_FILE cars.json ${CMAKE_CURRENT_BINARY_DIR}/cars.json)