cmake_minimum_required(VERSION 3.16)
project(limbo C)

set(CMAKE_C_STANDARD 23)

add_executable(limbo main.c lexer.c lexer.h unicode.c unicode.h num.c num.h error.c error.h)
target_link_libraries(limbo m)
