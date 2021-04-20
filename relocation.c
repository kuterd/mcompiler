#include <stdint.h>
#include "relocation.h"

size_t getRelocSize(enum reloc_size type) {
    switch(type) {
        case INT8:
            return 1;    
        case INT16:
            return 2;    
        case INT32:
            return 4;
        case INT64:
            return 8;
    }
} 

void label_setOffset(label_t *label, unsigned long offset) {
    label->offset = offset;
    label->hasOffset = 1;
}

void relocation_set(label_t *label, enum reloc_type type, enum reloc_size size, int bias, size_t offset) {
    relocation_t *reloc = dmalloc(sizeof(relocation_t));
    reloc->offset = offset;
    reloc->bias = bias;
    reloc->size = size;
    reloc->type = type;
    reloc->next = label->relocations;
    label->relocations = reloc;
}

void relocation_emit(dbuffer_t *dbuffer, label_t *label, enum reloc_type type, 
        enum reloc_size size, int bias) {
    relocation_set(label, type, size, bias, dbuffer->usage);
    dbuffer_pushChars(dbuffer, 0, getRelocSize(size));    
}

void relocation_apply(relocation_t *relocation, void *buffer, unsigned long offset, unsigned long memLocation) {
    uint64_t value;
    if (relocation->type == ABSOLUTE)
        value = offset;
    else
        value = offset - (memLocation + relocation->offset + relocation->bias);
    
    switch (relocation->size) {
        case INT8:
            value = (uint8_t)value;
            break; 
        case INT16:
            value = (uint16_t)value;
            break; 
        case INT32:
            value = (uint32_t)value;
            break;
        case INT64:
            break;
    }
    writeLong(buffer + relocation->offset, value, getRelocSize(relocation->size)); 
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


