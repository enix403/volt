#include "code/value.h"
#include <stddef.h>

#include <stdio.h>

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
    FREE_ARRAY(valarr->values);
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

        default:
            break;
    }
}
