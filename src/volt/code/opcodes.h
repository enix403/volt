#pragma once

typedef enum {
    // actions
    OP_RETURN,
    OP_LOADCONST,
    OP_POP,
    OP_POPN,

    // control flow and statments
    OP_PRINT,
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,

    OP_GET_LOCAL,
    OP_SET_LOCAL,

    // instructions that put constants on the stack
    OP_NIL,
    OP_TRUE,
    OP_FALSE,

    // arithematic operators
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,

    // logical operators
    OP_LOGIC_NOT,
    OP_LOGIC_AND,
    OP_LOGIC_OR,

    OP_LOGIC_EQUAL, // ==
    OP_LOGIC_GREATER, // >
    OP_LOGIC_LESS, // <

    // bitwise operators
    OP_BIT_NOT,
    OP_BIT_AND,
    OP_BIT_OR,

    // jumps
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_TRUE,
    OP_JUMP,
    OP_LOOP,

    // functions
    OP_CALL

} OpCode;