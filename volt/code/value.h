#pragma once
#include "bool.h"

typedef enum {
    VAL_NUMBER,
    VAL_NIL,
    VAL_BOOL,
    // VAL_OBJ
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
    } as;
} Value;


// checks if the given value object is of a specific type
#define IS_VAL_NUM(val) ((val).type == VAL_NUMBER) 
#define IS_VAL_NIL(val) ((val).type == VAL_NIL)
#define IS_VAL_BOOL(val) ((val).type == VAL_BOOL)

// wraps a native C value in a Value object
#define MK_VAL_NUM(c_num) ((Value){VAL_NUMBER, {.number = c_num}})
#define MK_VAL_NIL ((Value){VAL_NIL, {.number = 0}})
#define MK_VAL_BOOL(c_bool) ((Value){VAL_BOOL, {.boolean = c_bool}})

// Converts a Value object to a native C value
#define VAL_AS_NUM(val) ((val).as.number)
#define VAL_AS_BOOL(val) ((val).as.boolean)

typedef struct {
    Value* values;
    int capacity; // total capacity
    int count; // num of used slots
} ValueArray;

void valarray_init(ValueArray* valarr);
void valarray_write(ValueArray* valarr, Value val);
void valarray_free(ValueArray* valarr);

void print_val(Value val);