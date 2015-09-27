cmake_minimum_required(VERSION 3.3)
project(cc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -std=c++11")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")

set(SOURCE_FILES
    src/api.h
    src/cgen.c
    src/code.c
    src/core.c
    src/core.h
    src/lexer.c
    src/lstd.c
    src/main.c
    src/parser.c
    src/printer.c
    src/pvmc.h
    src/tree.c
    src/type.c)

add_executable(cc ${SOURCE_FILES})
target_link_libraries(cc m dl)