#include "volt/vm.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "volt/code/opcodes.h"
#include "volt/mem.h"
#include "volt/compiling/compiler.h"
#include "volt/debugging/switches.h"
#include "volt/debugging/disassembly.h"

VM vm;
typedef uint16_t short_t;


/* Stack handling and vm initializtion */
static inline void reset_stack() {
    vm.stack_top = vm.stack;
    vm.frame_count = 0;
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

// this function assumes that the stack is completely empty
static void define_native(const char* name, NativeFn fn) {
    pushstack(MK_VAL_OBJ(copy_string(name, (int)strlen(name))));
    pushstack(MK_VAL_OBJ(new_native(fn)));
    hashtable_set(&vm.globals, OBJ_AS_STRING(vm.stack[0]), vm.stack[1]);
    popstack_discard(2);
    // ObjNativeFn* fn_obj = new_native(fn);
    // hashtable_set(&vm.globals, copy_string( name, (int)strlen(name)), MK_VAL_OBJ(fn_obj));
}

static Value clock_native(int argc, Value* args) {
    return MK_VAL_NUM((double)clock() / CLOCKS_PER_SEC);
}

static Value input_num_native(int argc, Value* args) {
    long val;
    scanf("%ld", &val);
    return MK_VAL_NUM((double)val);
}

void vm_init() { 
    reset_stack();
    vm.objects = NULL;
    hashtable_init(&vm.interned_strings);
    hashtable_init(&vm.globals);

    define_native("clock", clock_native);
    define_native("input_num", input_num_native);
}
void vm_free() {
    free_objects(vm.objects);
    vm_init();
    hashtable_free(&vm.interned_strings);
    hashtable_free(&vm.globals);
}


/* Error handling */
static void runtime_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    // size_t instruction = vm.prog_counter - vm.cnk->code - 1;
    CallFrame* frame = &vm.frames[vm.frame_count - 1];
    size_t instruction = frame->pc - frame->func->chunk.code - 1;
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

static bool call_fn(ObjFunction* func, int arg_count) {
    if (func->arity != arg_count) {
        runtime_error("Expected %d arguments, got %d", func->arity, arg_count);
        return false;
    }

    if (vm.frame_count == FRAMES_MAX) {
        runtime_error("Call stack overflow");
        return false;
    }

    CallFrame* frame = &vm.frames[vm.frame_count++];
    frame->func = func;
    frame->pc = func->chunk.code;
    frame->stack_slots = vm.stack_top - arg_count - 1;
    return true;
}

static bool call_value(Value val, int arg_count) {
    switch(OBJ_TYPE(val)) {
        case OBJ_FUNCTION:
            return call_fn(OBJ_AS_FUNC(val), arg_count);

        case OBJ_NATIVEFN: {
            ObjNativeFn* native_obj = OBJ_AS_NATIVEFN(val);
            Value res = native_obj->fn(arg_count, vm.stack_top - arg_count);
            vm.stack_top -= arg_count + 1;
            pushstack(res);
            return true;
        }

        default:
            break;
    }

    runtime_error("Can only call functions and classes");
    return false;
}

/* Actual implementation of each opcode */ 
static InterpretResult run_machine() 
{

    CallFrame* frame = &vm.frames[vm.frame_count - 1];

#define READ_BYTE() (*frame->pc++)
#define READ_SHORT() \
    (frame->pc += 2, (short_t)(frame->pc[-2] << 8 | frame->pc[-1]))
// #define READ_CONST() (vm.cnk->constants.values[READ_BYTE()])
#define READ_CONST() (frame->func->chunk.constants.values[READ_BYTE()])
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

    byte_t instruction;

    for(;;) {

#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm.stack; slot < vm.stack_top; slot++) {
            printf("[ ");
            print_val(*slot);
            printf(" ]");
        }
        printf("\n");
        disassemble_instruction(&frame->func->chunk, (int)(frame->pc - frame->func->chunk.code));

#endif

        instruction = READ_BYTE();

        switch (instruction) {
            case OP_RETURN: {
                // popstack_discard(1);
                Value return_val = popstack();
                vm.frame_count--;

                if (vm.frame_count == 0) {
                    popstack_discard(1);
                    return INTERPRET_OK; 
                }

                vm.stack_top = frame->stack_slots;
                pushstack(return_val);

                frame = &vm.frames[vm.frame_count - 1];
                break;
            }
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
                pushstack(frame->stack_slots[slot_index]);
                break;
            }

            case OP_SET_LOCAL: {
                byte_t slot_index = READ_BYTE();
                frame->stack_slots[slot_index] = peekstack(0); // remember, don't pop because assignment is an expression
                break;
            }

            // jumps
            case OP_JUMP_IF_FALSE: {
                short_t offset = READ_SHORT();
                if (is_falsey(peekstack(0)))
                    frame->pc += offset;
                break;
            }

            case OP_JUMP_IF_TRUE: {
                short_t offset = READ_SHORT();
                if (!is_falsey(peekstack(0)))
                    frame->pc += offset;
                break;
            }

            case OP_JUMP: {
                short_t offset = READ_SHORT();
                frame->pc += offset;
                break;
            }

            case OP_LOOP: {
                short_t offset = READ_SHORT();
                frame->pc -= offset;
                break;
            }

            case OP_CALL: {
                byte_t arg_count = READ_BYTE();

                // fn arg_1 arg_2 [stack_top]
                if (!call_value(peekstack(arg_count), arg_count)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm.frames[vm.frame_count - 1];
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


InterpretResult vm_execsource(const char* source) {
    ObjFunction* func = compile(source);
    if (func == NULL) return INTERPRET_COMPILE_ERROR;

    // CallFrame* frame = &vm.frames[vm.frame_count++];
    // frame->func = func;
    // frame->pc = func->chunk.code;
    // frame->stack_slots = vm.stack_top;

    pushstack(MK_VAL_OBJ(func));
    call_fn(func, 0);

    return run_machine();
}