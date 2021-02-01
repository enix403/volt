#include "vm.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "code/opcodes.h"
#include "mem.h"
#include "compiling/compiler.h"
#include "debugging/disassembly.h"

VM vm;
typedef uint16_t short_t;


/* Stack handling and vm initializtion */
static inline void reset_stack() {
    vm.stack_top = vm.stack;
}
void vm_init() { 
    reset_stack();
    vm.objects = NULL;
    hashtable_init(&vm.interned_strings);
    hashtable_init(&vm.globals);
}
void vm_free() {
    free_objects(vm.objects);
    vm_init();
    hashtable_free(&vm.interned_strings);
    hashtable_free(&vm.globals);
}

static inline void pushstack(Value val) {
    // *vm.stack_top = val;
    // vm.stack_top++;
    *vm.stack_top++ = val;
}
static inline Value popstack() {
    // vm.stack_top--;
    // return *vm.stack_top;
    // shortcut for above
    return *(--vm.stack_top);
}
static inline Value peekstack(int distance) {
    return vm.stack_top[-1 - distance];
}

static inline void popstack_discard(int num) {
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
static bool is_falsey(Value val) 
{
    // only nil and false are "falsey" 
    return IS_VAL_NIL(val) || (IS_VAL_BOOL(val) && !VAL_AS_BOOL(val));
}

static void concatenate(ObjString* a, ObjString* b)
{
    int length = a->length + b->length;
    char* new_string = ALLOCATE(char, length + 1);
    memcpy(new_string, a->chars, a->length);
    memcpy(new_string + a->length, b->chars, b->length);
    new_string[length] = '\0';

    ObjString* string_obj = take_string(new_string, length);
    pushstack(MK_VAL_OBJ(string_obj));
}
/* Actual implementation of each opcode */ 
#define READ_BYTE() (*vm.prog_counter++)
#define READ_SHORT() \
    (vm.prog_counter += 2, (uint16_t)(vm.prog_counter[-2] << 8 | vm.prog_counter[-1]))
#define READ_CONST() (vm.cnk->constants.values[READ_BYTE()])
#define READ_STRING() OBJ_AS_STRING(READ_CONST())

#define BINARY_OPERATION(valtype_macro, op)                             \
    if (!IS_VAL_NUM(peekstack(0)) || !IS_VAL_NUM(peekstack(1))) { \
        runtime_error("Operands must be numbers.");                     \
        return INTERPRET_RUNTIME_ERROR;                                 \
    }                                                                   \
    double b = VAL_AS_NUM(popstack());                               \
    double a = VAL_AS_NUM(popstack());                               \
    pushstack(valtype_macro(a op b))

    /// END BINARY_OPERATION()


static InterpretResult run_machine() 
{
    byte_t instruction;
    for(;;) {
        instruction = READ_BYTE();

        switch (instruction) {
            case OP_RETURN:     printf("[Stack count = %d]\n", (int)(vm.stack_top - vm.stack)); return INTERPRET_OK;
            case OP_LOADCONST:  pushstack(READ_CONST()); break;

            case OP_POP:    popstack(); break;
            case OP_POPN:   popstack_discard(READ_BYTE()); break;

            case OP_NEGATE: {
                if (!IS_VAL_NUM(peekstack(0))) {
                    runtime_error("Operand must be a number");
                    return INTERPRET_RUNTIME_ERROR;
                }
                pushstack(
                    MK_VAL_NUM(
                        -VAL_AS_NUM( popstack() )
                    )
                ); 
                break;
            }

            // arithematic instructions 
            case OP_ADD: {
                Value vala = peekstack(1);
                Value valb = peekstack(0);
                if (
                    IS_OBJ_STRING(vala) && 
                    IS_OBJ_STRING(valb)
                ) {
                    popstack_discard(2);
                    concatenate(OBJ_AS_STRING(vala), OBJ_AS_STRING(valb));
                }
                else if (
                    IS_VAL_NUM(vala) && 
                    IS_VAL_NUM(valb) 
                ) {
                    double a = VAL_AS_NUM(vala);
                    double b = VAL_AS_NUM(valb);
                    popstack_discard(2);
                    pushstack(MK_VAL_NUM(a + b));
                }
                else {
                    runtime_error("Operands must be two numbers or strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT:   { BINARY_OPERATION(MK_VAL_NUM, -); break; }
            case OP_MULTIPLY:   { BINARY_OPERATION(MK_VAL_NUM, *); break; }
            case OP_DIVIDE:     { BINARY_OPERATION(MK_VAL_NUM, /); break; }

            // stack's constant instrucions
            case OP_NIL:    pushstack(MK_VAL_NIL); break;
            case OP_TRUE:   pushstack(MK_VAL_BOOL(true)); break;
            case OP_FALSE:  pushstack(MK_VAL_BOOL(false)); break;

            case OP_LOGIC_NOT:  pushstack(MK_VAL_BOOL(is_falsey(popstack()))); break;

            case OP_LOGIC_EQUAL: {
                Value a = popstack();
                Value b = popstack();
                pushstack(MK_VAL_BOOL(values_equal(a, b)));
                break;
            }
            case OP_LOGIC_GREATER:  { BINARY_OPERATION(MK_VAL_BOOL, >); break; }
            case OP_LOGIC_LESS:     { BINARY_OPERATION(MK_VAL_BOOL, <); break; }

            case OP_PRINT: 
                print_val(popstack());
                printf("\n");
                break;

            // variables
            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                hashtable_set(&vm.globals, name, peekstack(0));
                popstack_discard(1);
                break;
            }

            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value res;
                if (hashtable_get(&vm.globals, name, &res)) {
                    pushstack(res);
                }
                else {
                    runtime_error("Undefined variable \"%s\".", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                if (hashtable_set(&vm.globals, name, peekstack(0))) {
                    // if it is not being overwritten
                    hashtable_delete(&vm.globals, name);
                    runtime_error("Undefined variable \"%s\".", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                // note that we don't pop it off the stack because
                // assignment is an expression
                break;
            }

            case OP_GET_LOCAL: {
                byte_t slot_index = READ_BYTE();
                pushstack(vm.stack[slot_index]);
                break;
            }

            case OP_SET_LOCAL: {
                byte_t slot_index = READ_BYTE();
                vm.stack[slot_index] = peekstack(0); // remember, don't pop because assignment is an expression
                break;
            }

            // jumps
            case OP_JUMP_IF_FALSE: {
                short_t offset = READ_SHORT();
                if (is_falsey(peekstack(0)))
                    vm.prog_counter += offset;
                break;
            }

            case OP_JUMP_IF_TRUE: {
                short_t offset = READ_SHORT();
                if (!is_falsey(peekstack(0)))
                    vm.prog_counter += offset;
                break;
            }

            case OP_JUMP: {
                short_t offset = READ_SHORT();
                vm.prog_counter += offset;
                break;
            }

            case OP_LOOP: {
                short_t offset = READ_SHORT();
                vm.prog_counter -= offset;
                break;
            }



            default:
                break;
        }
    }
}
#undef BINARY_OPERATION
#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONST
#undef READ_STRING


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