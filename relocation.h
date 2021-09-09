#ifndef RELOCATION_H
#define RELOCATION_H

// For values that we don't know but will know in the future.

#include "buffer.h"
#include "utils.h"

enum reloc_size { INT8, INT16, INT32, INT64 };

// TODO: Platform specific stuff ?
enum reloc_type { RELATIVE, ABSOLUTE };

size_t getRelocSize(enum reloc_size size);

// TODO: Maybe use a dbuffer instead of linked list
struct relocation {
    enum reloc_type type;
    enum reloc_size size;
    int bias;
    size_t offset; // the offset in dbuffer.
    struct relocation *next;
};

typedef struct relocation relocation_t;

typedef struct {
    int hasOffset;
    size_t offset;
    relocation_t *relocations;
} label_t;

void relocation_set(label_t *label, enum reloc_type type, enum reloc_size size,
                    int bias, size_t offset);
void relocation_emit(dbuffer_t *dbuffer, label_t *label, enum reloc_type type,
                     enum reloc_size size, int bias);

// internal function.
// void relocation_apply(relocation_t *relocation, void *buffer, unsigned long
// offset, unsigned long memLocation);

void label_applyMemory(label_t *label, void *buffer, unsigned long memLocation);

void label_apply(label_t *label, void *buffer);
void label_setOffset(label_t *label, unsigned long offset);

#endif
