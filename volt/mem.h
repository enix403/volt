#pragma once

// doubles a number with a minimun output of 8
#define GROW_CAPACITY(cap) \
    ((cap) < 8 ? 8 : (cap) * 2)

#define GROW_ARRAY(type, buffer, old_count, new_count) \
    (type*)reallocate(buffer, sizeof(type) * (old_count), sizeof(type) * (new_count))

// free array of count old_count
#define FREE_ARRAY(type, buffer, old_count) \
    reallocate(buffer, sizeof(type) * (old_count) , 0)

// free an object
#define FREE(type, pointer) \
    reallocate(pointer, sizeof(type), 0)

// a thin wrapper around allocate
#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

/*
** if new_size == 0, it frees the buffer (regardless of what old_size is)
** if new_size > old_size, it grows the buffer
** if new_size < old_size, it shrinks the buffer
** if old_size == 0, it allocates a new block of memory and returns it
* in any case returns NULL on failure
*/
void* reallocate(void* buffer, int old_size, int new_size);
void free_objects();