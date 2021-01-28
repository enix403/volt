#include "vm.h"

#include <stdio.h>
#include <stdarg.h>

#include "code/opcodes.h"
#include "compiling/compiler.h"

VM vm;

/* Stack handling and vm initializtion */
static inline void reset_stack() {
    vm.stack_top = vm.stack;
}
void vm_init() { reset_stack(); }
void vm_free() {}

void vm_pushstack(Value val) {
    *vm.stack_top = val;
    vm.stack_top++;
}
Value vm_popstack() {
    // vm.stack_top--;
    // return *vm.stack_top;
    // shortcut for above
    return *(--vm.stack_top);
}
Value vm_peekstack(int distance) {
    return vm.stack_top[-1 - distance];
}

static inline void vm_pop_discard(int num) {
    vm.stack_top -= num;
}

/* Error handling */
static void runtime_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.prog_counter - vm.cnk->code - 1;
    // int line = vm.cnk->lines[instruction];
    // TODO: Get the real line of instruction
    int line = 888;
    fprintf(stderr, "[line %d] in script\n", line);
    reset_stack();
}

/* Misc helpers */
static bool is_falsey(Value val) {
    // only nil and false are "falsey" 
    return IS_VAL_NIL(val) || (IS_VAL_BOOL(val) && !VAL_AS_BOOL(val));
}

/* Actual implementation of each opcode */ 
#define READ_BYTE() (*vm.prog_counter++)
#define READ_CONST() (vm.cnk->constants.values[READ_BYTE()])
#define BINARY_OPERATION(valtype_macro, op)                             \
    if (!IS_VAL_NUM(vm_peekstack(0)) || !IS_VAL_NUM(vm_peekstack(1))) { \
        runtime_error("Operands must be numbers.");                     \
        return INTERPRET_RUNTIME_ERROR;                                 \
    }                                                                   \
    double b = VAL_AS_NUM(vm_popstack());                               \
    double a = VAL_AS_NUM(vm_popstack());                               \
    vm_pushstack(valtype_macro(a op b))

    /// END BINARY_OPERATION()


static InterpretResult run_machine() {
    byte_t instruction;
    while (1) {
        instruction = READ_BYTE();

        switch (instruction) {
            case OP_RETURN:     { 
                print_val(vm_popstack()); 
                printf("\n");
                return INTERPRET_OK; 
            }

            case OP_LOADCONST:  vm_pushstack(READ_CONST()); break;

            case OP_POP:        vm_popstack(); break;
            case OP_POPN:       vm_pop_discard(READ_BYTE()); break;

            case OP_NEGATE: {
                if (!IS_VAL_NUM(vm_peekstack(0))) {
                    runtime_error("Operand must be a number");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vm_pushstack(
                    MK_VAL_NUM(
                        -VAL_AS_NUM( vm_popstack() )
                    )
                ); 
                break;
            }

            case OP_ADD:        { BINARY_OPERATION(MK_VAL_NUM, +); break; }
            case OP_SUBTRACT:   { BINARY_OPERATION(MK_VAL_NUM, -); break; }
            case OP_MULTIPLY:   { BINARY_OPERATION(MK_VAL_NUM, *); break; }
            case OP_DIVIDE:     { BINARY_OPERATION(MK_VAL_NUM, /); break; }

            case OP_NIL:        vm_pushstack(MK_VAL_NIL); break;
            case OP_TRUE:       vm_pushstack(MK_VAL_BOOL(true)); break;
            case OP_FALSE:      vm_pushstack(MK_VAL_BOOL(false)); break;

            case OP_LOGIC_NOT:  vm_pushstack(MK_VAL_BOOL(is_falsey(vm_popstack()))); break;

            case OP_LOGIC_EQUAL: {
                Value a = vm_popstack();
                Value b = vm_popstack();
                vm_pushstack(MK_VAL_BOOL(values_equal(a, b)));
                break;
            }
            case OP_LOGIC_GREATER:  { BINARY_OPERATION(MK_VAL_BOOL, >); break; }
            case OP_LOGIC_LESS:     { BINARY_OPERATION(MK_VAL_BOOL, <); break; }

            default:
                break;
        }
    }
}
#undef READ_BYTE
#undef READ_CONST


/* Runner functions */
InterpretResult vm_execchunk(Chunk* cnk) {
    vm.cnk = cnk;
    vm.prog_counter = cnk->code;
    return run_machine();
}

InterpretResult vm_execsource(const char* source) {
    Chunk cnk;
    chunk_init(&cnk);

    // compile
    if (!compile(source, &cnk)) {
        chunk_free(&cnk);
        return INTERPRET_COMPILE_ERROR;
    }

    // setup execution
    vm.cnk = &cnk;
    vm.prog_counter = cnk.code;

    // run
    InterpretResult res = run_machine();
    // free resources
    chunk_free(&cnk);
    return res;
}