#ifndef RELOCATION_H
#define RELOCATION_H

#include "utils.h"
#include "dbuffer.h"

enum reloc_type {
    RELATIVE1,
    RELATIVE2,
    RELATIVE4,
    ABSOLUTE8 
};

size_t getRelocSize(enum reloc_type type) {
    switch(type) {
        case RELATIVE1:
            return 1;    
        case RELATIVE2:
            return 2;    
        case RELATIVE4:
            return 4;
        case ABSOLUTE8:
            return 8;
    }
} 

struct relocation {
    enum reloc_type type;  
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

void label_setOffset(label_t *label, unsigned long offset) {
    label->offset = offset;
    label->hasOffset = 1;
}

void relocation_set(label_t *label, enum reloc_type type, int bias, size_t offset) {
    size_t size = getRelocSize(type);
    relocation_t *reloc = dmalloc(sizeof(relocation_t));
    reloc->offset = offset;
    reloc->bias = bias;
    reloc->type = type;
    reloc->next = label->relocations;
    label->relocations = reloc;
}

#if 0
void relocation_apply(void *buffer, size_t offset, int bias, size_t relocOffset) {
    unsigned long value;    
    switch (relocation->type) {
        case absolute8:
            value = label->offset;
            break;
        case relative4:
            // this is fine
            value = (unsigned long)((int)offset - relocOffset + bias);
            break;
    }
    
    writelong(buffer + relocation->offset, value, getrelocsize(relocation->type));    
}
#endif

void relocation_apply(relocation_t *relocation, void *buffer, unsigned long offset, unsigned long memLocation) {
    unsigned long value;    
    switch (relocation->type) {
        case ABSOLUTE8:
            value = offset;
            break;
        case RELATIVE1:
            value = (unsigned long)(char)(offset - (memLocation + relocation->offset) + relocation->bias);
            break; 
        case RELATIVE2:
            value = (unsigned long)(short)(offset - (memLocation + relocation->offset) + relocation->bias);
            break; 
        case RELATIVE4:
            value = (unsigned long)(int)(offset - (memLocation + relocation->offset) + relocation->bias);
            break;
    }
    writeLong(buffer + relocation->offset, value, getRelocSize(relocation->type));    
}

void label_applyMemory(label_t *label, void *buffer, unsigned long memLocation) {
    for (relocation_t *current = label->relocations, *next;
         current != NULL; current = next) {
        next = current->next;
        relocation_apply(current, buffer, label->offset, memLocation); 
        free(current);
     }
    label->relocations = NULL;
}

void label_apply(label_t *label, void *buffer) {
    label_applyMemory(label, buffer, 0);
}

#endif
