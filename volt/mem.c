#include "mem.h"
#include <stdlib.h>
#include <stdio.h>


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