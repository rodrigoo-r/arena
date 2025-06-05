include(FetchContent)

cmake_minimum_required(VERSION 3.12)
project(arena C)

set(CMAKE_C_STANDARD 11)

add_library(arena STATIC arena.c arena.h)

if (NOT FLUENT_LIBC_RELEASE)
    FetchContent_Declare(
            types
            GIT_REPOSITORY https://github.com/rodrigoo-r/types
            GIT_TAG        master
    )

    FetchContent_Declare(
            vector
            GIT_REPOSITORY https://github.com/rodrigoo-r/vector
            GIT_TAG        master
    )

    FetchContent_MakeAvailable(types)
    FetchContent_MakeAvailable(vector)
    target_include_directories(arena PRIVATE ${CMAKE_BINARY_DIR}/_deps/types-src)
    target_include_directories(arena PRIVATE ${CMAKE_BINARY_DIR}/_deps/vector-src)
    target_link_libraries(arena PRIVATE types)
    target_link_libraries(arena PRIVATE vector)
endif()