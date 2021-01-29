#pragma once

#include "code/value.h"
#include "code/object.h"
#include "bool.h"

typedef struct {
    ObjString* key;
    Value value;
} HashTableEntry;

typedef struct {
    int capacity;
    int count; // no. of actual entries + no. of tombstones
    HashTableEntry* entries;
} HashTable;

void hashtable_init(HashTable* table);
void hashtable_free(HashTable* table);

// Fully duplicate a hashtable
void hashtable_add_all(HashTable* from, HashTable* to);

/*
* Set the key with the given value
* returns true on adding a new key, and false on overwriting an existing ket
*/
bool hashtable_set(HashTable* table, ObjString* key, Value val);

// returns true on success, false otherwise
bool hashtable_get(HashTable* table, ObjString* key, Value* result_val);

// returns true on success, false otherwise
bool hashtable_delete(HashTable* table, ObjString* key);

// return pointer to key of the entry from looked up from a c-style string  
ObjString* hashtable_findstr(HashTable* table, const char* key_str, int len, strhash_t hash);