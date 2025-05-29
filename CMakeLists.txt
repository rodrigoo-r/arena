cmake_minimum_required(VERSION 3.30)
project(arena C)

set(CMAKE_C_STANDARD 11)

add_library(arena STATIC arena.c)
