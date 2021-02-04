#pragma once
#include "volt/code/value.h"
#include "volt/code/chunk.h"

#include <stddef.h>
#include <stdint.h>

/* General Obj stuff */
typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVEFN
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
};

#define OBJ_TYPE(val) (VAL_AS_OBJ(val)->type)
void print_obj(Value val);
Obj* allocate_obj(size_t size, ObjType type);

static inline bool is_obj_type(Value val, ObjType type) { return IS_VAL_OBJ(val) && VAL_AS_OBJ(val)->type == type; }


/* +======+ STRINGS +======+ */
typedef uint32_t strhash_t;

struct ObjString {
    Obj obj;
    int length;
    char* chars;
    strhash_t hash;
};

ObjString* allocate_string(char* chars, int length, strhash_t hash);
ObjString* copy_string(const char* chars, int length);
ObjString* take_string(char* chars, int length);

#define IS_OBJ_STRING(val) is_obj_type(val, OBJ_STRING)
#define OBJ_AS_STRING(val)   ((ObjString*)VAL_AS_OBJ(val))
#define AS_CSTRING(val)  (((ObjString*)VAL_AS_OBJ(val))->chars)


/* +======+ USER FUNCTIONS +======+ */
typedef struct {
    Obj obj;
    unsigned int arity;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

#define IS_OBJ_FUNC(val) is_obj_type(val, OBJ_FUNCTION)
#define OBJ_AS_FUNC(val) ((ObjFunction*)VAL_AS_OBJ(val))

ObjFunction* new_function();


/* +======+ NATIVE FUNCTIONS +======+ */

typedef Value (*NativeFn)(int argc, Value* args);

typedef struct {
    Obj obj;
    NativeFn fn;
} ObjNativeFn;

#define IS_OBJ_NATIVEFN(val) is_obj_type(val, OBJ_NATIVEFN)
#define OBJ_AS_NATIVEFN(val) ((ObjNativeFn*)VAL_AS_OBJ(val))

// wraps a NativeFn in ObjNativeFn*
ObjNativeFn* new_native(NativeFn fn);
