#pragma once

#include "volt/code/chunk.h"
#include "volt/code/value.h"
#include "volt/code/object.h"
#include "volt/hash_table.h"

#define FRAMES_MAX 64
#define VM_STACK_MAX (256 * FRAMES_MAX)
// #define VM_STACK_MAX 256


typedef struct {
    ObjFunction* func;
    byte_t* pc;
    Value* stack_slots;
} CallFrame;

typedef struct {
    // Chunk* cnk;
    // byte_t* prog_counter;

    CallFrame frames[FRAMES_MAX];
    unsigned int frame_count;

    // the great stack itself
    Value stack[VM_STACK_MAX];

    // the top most element is *(stack_top - 1), NOT *stack_top
    // In other words it points to the location where the next element will go 
    // it helps to detect if stack is empty by just comapring if it points
    // to the zero'th element
    Value* stack_top;

    Obj* objects;
    HashTable interned_strings;
    HashTable globals;
} VM;

extern VM vm;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

void vm_init();
void vm_free();

// void vm_pushstack(Value val);
// Value vm_popstack();
// Value vm_peekstack();

InterpretResult vm_execsource(const char* source);
