#include "volt/mem.h"
#include <stdlib.h>
#include <stdio.h>

#include "volt/code/object.h"

// All memory handling must be done here to pass through logging
void* reallocate(void* buffer, int old_size, int new_size) {
    if (new_size == 0) {
        free(buffer);
        return NULL;
    }

    void* result = realloc(buffer, new_size);
    if (result == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(1);
    }
    return result;
}

static void free_object(Obj* object) {
    switch (object->type) {
        case OBJ_STRING: {
            ObjString* string_obj = (ObjString*) object;
            FREE_ARRAY(char, string_obj->chars, string_obj->length + 1); // include the null terminator
            FREE(ObjString, string_obj);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* func = (ObjFunction*) object;
            chunk_free(&func->chunk);
            FREE(ObjFunction, func);
            break;
        }
        case OBJ_NATIVEFN: {
            FREE(ObjNativeFn, (ObjNativeFn*)object);
            break;
        }
    }
}

// takes a linked list of objects and frees them
void free_objects(Obj* list_start) {
    Obj* object = list_start;
    while (object != NULL) {
        Obj* next = object->next;
        free_object(object);
        object = next;
    }
}
