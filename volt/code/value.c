#include "code/value.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "code/object.h"
#include "mem.h"

void valarray_init(ValueArray* valarr) {
    valarr->count = 0;
    valarr->capacity = 0;
    valarr->values = NULL;
}
void valarray_write(ValueArray* valarr, Value val) {
    if (valarr->capacity <= valarr->count) {
        int old_cap = valarr->capacity;
        valarr->capacity = GROW_CAPACITY(old_cap);
        valarr->values = GROW_ARRAY(Value, valarr->values, old_cap, valarr->capacity);
    }

    valarr->values[valarr->count] = val;
    valarr->count++;
}
void valarray_free(ValueArray* valarr) {
    FREE_ARRAY(Value, valarr->values, valarr->capacity);
    valarray_init(valarr);
}

void print_val(Value val) {
    switch (val.type) {
        case VAL_BOOL:
            printf("%s", VAL_AS_BOOL(val) ? "true" : "false");
            break;

        case VAL_NUMBER:
            printf("%g", VAL_AS_NUM(val));
            break;

        case VAL_NIL:
            printf("nil");
            break;

        case VAL_OBJ:
            print_obj(val);
            break;

        default:
            break;
    }
}


bool values_equal(Value v1, Value v2) {
    if (v1.type != v2.type)
        return false;
    
    switch (v1.type) {
        case VAL_BOOL:      return VAL_AS_BOOL(v1) == VAL_AS_BOOL(v2);
        case VAL_NUMBER:    return VAL_AS_NUM(v1) == VAL_AS_NUM(v2);
        case VAL_NIL:       return true;
        case VAL_OBJ: { // we only have strings yet.... 
            // after string internalization is implemented,
            // we can just compare if v1 and v2 point to the
            // same ObjString i.e OBJ_AS_STRING(v1) == OBJ_AS_STRING(v2)

            // ObjString* a = OBJ_AS_STRING(v1);
            // ObjString* b = OBJ_AS_STRING(v2);
            
            // if (a->length != b->length) return false;
            // return memcmp(a->chars, b->chars, a->length) == 0;

            return OBJ_AS_STRING(v1) == OBJ_AS_STRING(v2);
        }

        default: return false; // Unreachable
    }
    return false;
}
