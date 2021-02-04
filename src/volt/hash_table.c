#include "volt/hash_table.h"
#include <string.h>
#include "volt/mem.h"

// the maximun load factor for a table
#define HTABLE_MAX_LOAD 0.75

static HashTableEntry* find_entry(HashTableEntry* entries, int capacity, ObjString* key)
{
    strhash_t index = key->hash % capacity;
    HashTableEntry* tombstone = NULL;

    // this loop assumes that there is atleast one empty entry (excluding tombstones)
    for (;;) {
        HashTableEntry* ent = entries + index;

        if (ent->key == NULL) {
            // Check if it is NOT a tombstone.
            // The default value of an empty entry is NIL ( which was set in adjust_capacity() ) 
            if (IS_VAL_NIL(ent->value)) {
                // if we previouly found a tombstone then return it
                return tombstone == NULL ? ent : tombstone; 
            }
            else { 
                // it is a tombstone

                // save the tombstone for later
                // instead of returning it immediately
                // because we might later find an entry with
                // the given key
                tombstone = ent;
            }
        }
        else if (ent->key == key) {
            // we found the key
            return ent;
        }
        index = (index + 1) % capacity;
    }
}

static void adjust_capacity(HashTable* table, int new_capacity)
{
    HashTableEntry* new_entries = ALLOCATE(HashTableEntry, new_capacity);
    for (int i = 0; i < new_capacity; i++) {
        new_entries[i].key = NULL;
        new_entries[i].value = MK_VAL_NIL;
    }

    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        HashTableEntry* old_entry = table->entries + i;
        if (old_entry->key == NULL) {
            continue;
        }

        HashTableEntry* dest = find_entry(table->entries, new_capacity, old_entry->key);
        dest->key = old_entry->key;
        dest->value = old_entry->value;
        table->count++;
    }

    FREE_ARRAY(HashTableEntry, table->entries, table->capacity);
    table->entries = new_entries;
    table->capacity = new_capacity;
}

void hashtable_init(HashTable* table)
{
    table->capacity = 0;
    table->count = 0;
    table->entries = NULL;
}

void hashtable_free(HashTable* table)
{
    FREE_ARRAY(HashTableEntry, table->entries, table->capacity);
    hashtable_init(table);
}

void hashtable_add_all(HashTable* from, HashTable* to)
{
    for (int i = 0; i < from->capacity; i++) {
        HashTableEntry* source_ent = from->entries + i;
        if (source_ent->key != NULL)
            hashtable_set(to, source_ent->key, source_ent->value);
    }
}

bool hashtable_set(HashTable* table, ObjString* key, Value val)
{
    // grow the array when we hit the maximum load factor
    if (table->count + 1 > table->capacity * HTABLE_MAX_LOAD) {
        int new_capacity = GROW_CAPACITY(table->capacity);
        adjust_capacity(table, new_capacity);
    }

    HashTableEntry* entry = find_entry(table->entries, table->capacity, key);
    bool is_new_key = (entry->key == NULL);
    if (is_new_key && IS_VAL_NIL(entry->value))
        table->count++; // only increase count if not reusing a tombstone
    entry->key = key;
    entry->value = val;

    return is_new_key;
}

bool hashtable_get(HashTable* table, ObjString* key, Value* result_val)
{
    if (table->count == 0)
        return false;

    HashTableEntry* ent = find_entry(table->entries, table->capacity, key);
    if (ent->key == NULL)
        return false;

    *result_val = ent->value;
    return true;
}

bool hashtable_delete(HashTable* table, ObjString* key)
{
    if (table->count == 0)
        return false;
    
    HashTableEntry* ent = find_entry(table->entries, table->capacity, key);

    // if no entry is found, return false
    if (ent == NULL)
        return false;

    // place a tombstone
    ent->key = NULL;
    ent->value = MK_VAL_BOOL(true);

    // Note that table->count is not decremented
    // because it includes tombstones

    return true;
}


ObjString* hashtable_findstr(HashTable* table, const char* key_str, int len, strhash_t hash)
{
    if (table->count == 0)
        return NULL;

    strhash_t index = hash % table->capacity;
    
    for (;;) {
        HashTableEntry* ent = table->entries + index;

        if (ent->key == NULL) {
            // return null if we found a non-empty tombstone
            if (IS_VAL_NIL(ent->value))
                return NULL;
        }
        else if (ent->key->length == len && ent->key->hash == hash &&
                memcmp(ent->key->chars, key_str, len) == 0) {
            return ent->key;
        }

        index = (index + 1) % table->capacity;
    }
}
