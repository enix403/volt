#include <stdio.h>

#include "hash_table.h"
#include "vm.h"
#include "code/object.h"



typedef struct {
    const char* start;
    int len;
} string_t;

void set_table(HashTable* table, string_t key, string_t val) {
    ObjString* keyobj = copy_string(key.start, key.len);
    ObjString* valobj = copy_string(val.start, val.len);
    Value val_ = MK_VAL_OBJ(valobj);
    hashtable_set(table, keyobj, val_);
}

char* get_table(HashTable* table, string_t key) {
    ObjString* keyobj = copy_string(key.start, key.len);
    Value res;
    hashtable_get(table, keyobj, &res);
    return AS_CSTRING(res);
}

int main() {
    vm_init();
    // printf("Testing...\n");

    HashTable table;
    hashtable_init(&table);

    string_t key1 = { "test_key_1", 10 };
    string_t val1 = { "test_value_1", 12 };

    string_t val2 = { "test_value_2", 12 };
    string_t key2 = { "test_key_2", 10 };

    set_table(&table, key1, val1);
    set_table(&table, key2, val2);

    printf("%s\n", get_table(&table, key1));
    printf("%s\n", get_table(&table, key2));

    hashtable_free(&table);
    vm_free();
    return 0;
}