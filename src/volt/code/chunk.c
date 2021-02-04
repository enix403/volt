#include "volt/code/chunk.h"

#include <stddef.h>
#include "volt/mem.h"

void chunk_init(Chunk *cnk) {
    cnk->capacity = 0;
    cnk->count = 0;
    cnk->code = NULL;
    valarray_init(&cnk->constants);
}
void chunk_write(Chunk *cnk, byte_t byte) {
    if (cnk->capacity <= cnk->count) {
        int old_cap = cnk->capacity;
        cnk->capacity = GROW_CAPACITY(old_cap);
        cnk->code = GROW_ARRAY(byte_t, cnk->code, old_cap, cnk->capacity);
    }

    cnk->code[cnk->count] = byte;
    cnk->count++;
}
void chunk_free(Chunk *cnk) {
    FREE_ARRAY(byte_t, cnk->code, cnk->count);
    valarray_free(&cnk->constants);
    chunk_init(cnk);
}

int chunk_addconst(Chunk* cnk, Value val) {
    valarray_write(&cnk->constants, val);
    return cnk->constants.count - 1;
}
