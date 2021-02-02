#include "code/object.h"

#include <stdio.h>
#include <string.h>
#include "mem.h"
#include "vm.h"
#include "hash_table.h"

/* +======+ STRINGS +======+ */


// Fowler–Noll–Vo hash function
// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
static strhash_t hash_string(const char* key, int length) {
    strhash_t hash = 2166136261u;

    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }

    return hash;
}

static void print_func(ObjFunction* func) {
    if (func->name == NULL) {
        printf("<main>");
        return;
    }
    printf("<fn %s>", func->name->chars);
}


void print_obj(Value val) {
    switch (OBJ_TYPE(val)) {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(val));
            break;

        case OBJ_FUNCTION:
            print_func(OBJ_AS_FUNC(val));
            break;

        case OBJ_NATIVEFN:
            printf("<[native fn]>");
            break;
        
        default: break; // Unreachable
    }
}

// just lazy to cast back to a valid pointer
#define ALLOCATE_OBJ(ctype, objtype) \
    (ctype*)allocate_obj(sizeof(ctype), objtype)

Obj* allocate_obj(size_t size, ObjType type) {
    Obj* obj = (Obj*)reallocate(NULL, 0, size);
    obj->type = type;

    obj->next = vm.objects;
    vm.objects = obj;

    return obj;
}

ObjString* allocate_string(char* chars, int length, strhash_t hash) {
    ObjString* string_obj = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string_obj->chars = chars;
    string_obj->length = length;
    string_obj->hash = hash;

    hashtable_set(&vm.interned_strings, string_obj, MK_VAL_NIL);

    return string_obj;
}

ObjString* copy_string(const char* chars, int length) {
    char* heap_chars = ALLOCATE(char, length + 1);
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';
    strhash_t hash = hash_string(heap_chars, length);

    // Check if the string is already interned. If so, return it
    // instead of creating a new one
    ObjString* interned = hashtable_findstr(&vm.interned_strings, chars, length, hash);
    if (interned != NULL)
        return interned;

    return allocate_string(heap_chars, length, hash);
}


ObjString* take_string(char* chars, int length) {
    strhash_t hash = hash_string(chars, length);

    // Check if the string is already interned. If so, return it
    // instead of creating a new one
    ObjString* interned = hashtable_findstr(&vm.interned_strings, chars, length, hash);
    if (interned != NULL) {
        // free the chars, because we have their ownership now
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return allocate_string(chars, length, hash);
}


/* +======+ FUNCTIONS +======+ */

ObjFunction* new_function() {
    ObjFunction* func = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    func->arity = 0;
    func->name = NULL;
    chunk_init(&func->chunk);
    return func;
}


/* +======+ NATIVE FUNCTIONS +======+ */
ObjNativeFn* new_native(NativeFn fn) {
    ObjNativeFn* native_obj = ALLOCATE_OBJ(ObjNativeFn, OBJ_NATIVEFN);
    native_obj->fn = fn;
    return native_obj;
}
