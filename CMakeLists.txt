cmake_minimum_required(VERSION 3.0)

set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

project(VoltLang VERSION 1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(volt)

add_executable(
    app

    "volt/main.c"
    "volt/mem.c"
    "volt/code/chunk.c"
    "volt/code/value.c"
    "volt/vm.c"
    "volt/debugging/disassembly.c"
    "volt/scanning/scanner.c"
    "volt/compiling/compiler.c"
)
set_property(TARGET app PROPERTY C_STANDARD 11)
# c versions go as 90 -> 99 -> 11 -> 17
# 17 is not supported by cmake

