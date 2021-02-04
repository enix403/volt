#pragma once

#include "volt/code/chunk.h"

void disassemble_chunk(Chunk* cnk, const char* chunk_name);
int disassemble_instruction(Chunk* cnk, int offset);