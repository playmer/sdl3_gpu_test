project(test C CXX)
cmake_minimum_required(VERSION 3.27.2)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_subdirectory(SDL)

add_executable(gpu_test)

target_sources(gpu_test 
PUBLIC 
    "main.c"
)

target_link_libraries(gpu_test 
PUBLIC 
    $<IF:$<TARGET_EXISTS:SDL3::SDL3>,SDL3::SDL3,SDL3::SDL3-static>
)
