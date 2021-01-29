#pragma once

#include "code/chunk.h"
#include "code/value.h"
#include "code/object.h"
#include "hash_table.h"

#define VM_STACK_MAX 256

typedef struct {
    Chunk* cnk;
    byte_t* prog_counter;

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
InterpretResult vm_execchunk(Chunk* cnk);
