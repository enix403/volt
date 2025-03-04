#include "volt/debugging/disassembly.h"

#include <stdio.h>
#include "volt/code/opcodes.h"
#include "volt/code/value.h"

// pretty prints an instruction that takes no operand
static int simple_instruction(const char* name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

// instruction that takes one operand
static int byte_instruction(const char* name, int offset, Chunk* cnk)
{
    printf("%-16s %4d\n", name, cnk->code[offset + 1]);
    return offset + 2;
}


static int jump_instruction(const char* name, int sign,
                           int offset, Chunk* chunk) {
  uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
  jump |= chunk->code[offset + 2];
  printf("%-16s %4d -> %d (%c%d)\n", name, offset,
         offset + 3 + sign * jump,
         sign == -1 ? '-' : '+',
         jump);
  return offset + 3;
}



// pretty prints an instruction that takes index of a constant as an operand
static int const_instruction(const char* name, int offset, Chunk* cnk)
{
    byte_t constant_loc = cnk->code[offset + 1]; // the next byte
    printf("%-16s %4d '", name, constant_loc); // print the index
    print_val(cnk->constants.values[constant_loc]); // print the actual value
    printf("'\n");
    return offset + 2;
}

void disassemble_chunk(Chunk* cnk, const char* chunk_name)
{
    printf("==== %s ====\n", chunk_name);
    for (int offset = 0; offset < cnk->count;) {
        offset = disassemble_instruction(cnk, offset);
    }
    printf("/====/ END CHUNK: %s /====/\n\n", chunk_name);
}

int disassemble_instruction(Chunk* cnk, int offset)
{
    printf("%04d ", offset);
    byte_t instruction = cnk->code[offset];
    switch (instruction) {
    // clang-format off
        case OP_LOADCONST:      return const_instruction("OP_LOADCONST", offset, cnk);
        case OP_DEFINE_GLOBAL:  return const_instruction("OP_DEFINE_GLOBAL", offset, cnk);

        case OP_GET_GLOBAL:     return const_instruction("OP_GET_GLOBAL", offset, cnk);
        case OP_SET_GLOBAL:     return const_instruction("OP_SET_GLOBAL", offset, cnk);
        case OP_GET_LOCAL:      return byte_instruction("OP_GET_LOCAL", offset, cnk);
        case OP_SET_LOCAL:      return byte_instruction("OP_SET_LOCAL", offset, cnk);

        case OP_POPN:       return byte_instruction("OP_POPN", offset, cnk);
        case OP_RETURN:     return simple_instruction("OP_RETURN", offset);
        case OP_POP:        return simple_instruction("OP_POP", offset);
        case OP_PRINT:      return simple_instruction("OP_PRINT", offset);

        case OP_NEGATE:     return simple_instruction("OP_NEGATE", offset);
        case OP_ADD:        return simple_instruction("OP_ADD", offset);
        case OP_SUBTRACT:   return simple_instruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:   return simple_instruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:     return simple_instruction("OP_DIVIDE", offset);

        case OP_NIL:        return simple_instruction("OP_NIL", offset);
        case OP_TRUE:       return simple_instruction("OP_TRUE", offset);
        case OP_FALSE:      return simple_instruction("OP_FALSE", offset);

        case OP_LOGIC_NOT:  return simple_instruction("OP_LOGIC_NOT", offset);
        case OP_LOGIC_AND:  return simple_instruction("OP_LOGIC_AND", offset);
        case OP_LOGIC_OR:   return simple_instruction("OP_LOGIC_OR", offset);

        case OP_LOGIC_EQUAL:    return simple_instruction("OP_LOGIC_EQUAL", offset);
        case OP_LOGIC_GREATER:  return simple_instruction("OP_LOGIC_GREATER", offset);
        case OP_LOGIC_LESS:     return simple_instruction("OP_LOGIC_LESS", offset);
        
        case OP_BIT_NOT:    return simple_instruction("OP_BIT_NOT", offset);
        case OP_BIT_AND:    return simple_instruction("OP_BIT_AND", offset);
        case OP_BIT_OR:     return simple_instruction("OP_BIT_OR", offset);

        case OP_JUMP_IF_FALSE:  return jump_instruction("OP_JUMP_IF_FALSE", 1, offset, cnk);
        case OP_JUMP_IF_TRUE:  return jump_instruction("OP_JUMP_IF_TRUE", 1, offset, cnk);
        case OP_JUMP:           return jump_instruction("OP_JUMP", 1, offset, cnk);
        case OP_LOOP:           return jump_instruction("OP_LOOP", -1, offset, cnk);

        case OP_CALL:   return byte_instruction("OP_CALL", offset, cnk);

        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;

        // clang-format on
    }
}