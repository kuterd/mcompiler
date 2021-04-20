#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void* dmalloc(size_t size) {
    void *result = malloc(size);
    if (result == NULL) {
        puts("allocation failed");
        exit(1);
    }
    return result;
}

void *dzmalloc(size_t size) {
    void *result = calloc(size, 1);
    if (result == NULL) {
        puts("allocation failed");
        exit(1);
    }
    return result;
}

void writeLong(char *buffer, unsigned long num, size_t bytes) {
    for (int i = 0; i < bytes; i++) {
        buffer[i] = num & 0xFF;
        num = num >> 8;
    }
}

void writeInt(char *buffer, unsigned long num, size_t bytes) {
    for (int i = 0; i < bytes; i++) {
        buffer[i] = num & 0xFF;
        num = num >> 8;
    }
}


