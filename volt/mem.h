#pragma once

#define GROW_CAPACITY(cap) \
    ((cap) < 8 ? 8 : (cap) * 2)


#define GROW_ARRAY(type, buffer, old_count, new_count) \
    (type*)reallocate(buffer, sizeof(type) * (old_count), sizeof(type) * (new_count))


// reallocate doesn't care about old_size when new_size is 0
// so we can pass 0 as old_size
// (though this information could be used in debugging)
#define FREE_ARRAY(buffer) \
    reallocate(buffer, 0, 0)

/*
** if new_size == 0, it frees the buffer (regardless of what old_size is)
** if new_size > old_size, it grows the buffer
** if new_size < old_size, it shrinks the buffer
** if old_size == 0, it allocates a new block of memory and returns it
* in any case returns NULL on failure
*/
void* reallocate(void* buffer, int old_size, int new_size);