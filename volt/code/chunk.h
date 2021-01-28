#pragma once
#include <stdint.h>

#include "code/value.h"

typedef uint8_t byte_t;

typedef struct {
    // pointer to start of chunk
    byte_t* code;
    // the total capacity of the chunk (used or unused)
    int capacity;
    // the number of used slots
    int count;
    // static constants that appear in code
    ValueArray constants;
} Chunk;

void chunk_init(Chunk* cnk);
void chunk_write(Chunk* cnk, byte_t byte);
void chunk_free(Chunk* cnk);
int chunk_addconst(Chunk* cnk, Value val);