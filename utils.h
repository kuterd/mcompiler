// Kuter Dinel 2021
// Random utilies that don't have anywhere better to go.
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

// The macro that makes possible to get a struct from a pointer to one
// of it's fields. Used for our linked lists.
#define containerof(ptr, type, member)                          \
  ({                                                            \
    const typeof(((type*)0)->member) *_ptr_value = (ptr);       \
    (type*)((void*)_ptr_value - offsetof(type, member));        \
  })

#define COMMA(v) v,
#define STR_COMMA(v) #v,

// malloc that you can assume will always return a valid pointer.
// we might add a byte counting function later.
void* dmalloc(size_t size);
void *dzmalloc(size_t size);
void writeLong(char *buffer, unsigned long num, size_t bytes);
void writeInt(char *buffer, unsigned long num, size_t bytes);
#endif
