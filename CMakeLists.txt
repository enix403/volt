cmake_minimum_required(VERSION 3.0)

set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

project(VoltLang VERSION 1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(src)

add_executable(
    app

    "src/volt/main.c"
    "src/volt/mem.c"
    "src/volt/vm.c"
    "src/volt/hash_table.c"
    "src/volt/code/chunk.c"
    "src/volt/code/value.c"
    "src/volt/code/object.c"
    "src/volt/debugging/disassembly.c"
    "src/volt/scanning/scanner.c"
    "src/volt/compiling/compiler.c"
)

set_property(TARGET app PROPERTY C_STANDARD 11)

if (false)
add_executable(
    app2

    "src/volt2/main.c"

    "src/volt2/repl/repl.c"
    "src/volt2/memory.c"
    "src/volt2/code/chunk.c"
    "src/volt2/code/valarray.c"
    "src/volt2/runtime/value.c"
    "src/volt2/runtime/vlobjects/vlobject.c"
    "src/volt2/scanner/scanner.c"
)
set_property(TARGET app2 PROPERTY C_STANDARD 11)
endif()
# c versions go as 90 -> 99 -> 11 -> 17
# 17 is not supported by cmake

