#ifndef DBUFFER_H
#define DBUFFER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "utils.h"

#define DBUFFER_INIT 1000

#define RANGE_STRING(str) ((range_t) {.ptr=(str), .size=sizeof(str) - 1})
#define RANGE_COMPARE(r, s) range_cmp( (r), RANGE_STRING(s))

// Used to represent a memory range, like std::string_view

typedef struct {
    char *ptr;
    size_t size;
} range_t;

void range_skip(range_t *range, size_t amount) { 
    assert(range->size >= amount && "invalid skip");
    range->size -= amount;
    range->ptr += amount;
}

void range_shorten(range_t *range, size_t amount) {
    assert(range->size >= amount && "invalid amount");
    range->size -= amount;
} 

range_t range_subs(range_t *range, size_t size) {
    assert(range->size >= size && "invalid size");
    return (range_t) {.ptr = range->ptr, .size=size};    
}

range_t range_sub(range_t *range, size_t start, size_t size) {
    assert(range->size > start && "invalid start");  
    assert(range->size - start >= size && "invalid size");
    return (range_t) {.ptr = range->ptr + start, .size=size};    
}

// Please use the RANGE_STRING if the string is a constant.
range_t range_fromString(char *string) {
    return (range_t){.ptr=string, .size=strlen(string)};
}

int range_cmp(range_t a, range_t b) {
    if (a.size != b.size)
        return 0;
    return memcmp(a.ptr, b.ptr, a.size) == 0;
} 

range_t range_copy(range_t *range) {
    char *buffer = dmalloc(range->size);
    return (range_t) {.ptr = buffer, .size = range->size}; 
}

// Dynamicly sized buffer.
// Will adjust it's size to fit all elements.
// Currently it can only grow.
typedef struct {
    void *buffer;
    size_t capacity;
    size_t usage;
} dbuffer_t;

void dbuffer_write(dbuffer_t *dbuffer, FILE *file) {
    fwrite(dbuffer->buffer, 1, dbuffer->usage, file);     
}

void dbuffer_initSize(dbuffer_t *dbuffer, size_t size) {
    dbuffer->usage = 0;
    dbuffer->capacity = size;
    dbuffer->buffer = dmalloc(size); 
}

void dbuffer_init(dbuffer_t *dbuffer) {
    dbuffer_initSize(dbuffer, DBUFFER_INIT);
}

#define max(a, b) ((a) > (b) ? (a) : (b))

void dbuffer_ensureCap(dbuffer_t *dbuffer, size_t size) {
    if (size > dbuffer->capacity - dbuffer->usage) {
        size_t newSize = max(dbuffer->capacity * 2, dbuffer->capacity * 3 / 2 + size);    
        void *newBuffer = dmalloc(newSize);
        memcpy(newBuffer, dbuffer->buffer, dbuffer->usage);
        free(dbuffer->buffer);
        dbuffer->buffer = newBuffer;
     }
}

void dbuffer_pushData(dbuffer_t *dbuffer, void *buffer, size_t size) {
    dbuffer_ensureCap(dbuffer, size);
    memcpy(dbuffer->buffer + dbuffer->usage, buffer, size);
    dbuffer->usage += size;
}

void dbuffer_pushStr(dbuffer_t *dbuffer, void *str) {
    size_t size = strlen(str);
    dbuffer_pushData(dbuffer, str, size);
}

void dbuffer_pushChar(dbuffer_t *dbuffer, unsigned char ch) {
    dbuffer_pushData(dbuffer, &ch, 1);
}
void dbuffer_pushLong(dbuffer_t *dbuffer, unsigned long num, size_t bytes) {
    
    for (int i = 0; i < bytes; i++) {
        dbuffer_pushChar(dbuffer, num & 0xFF);
        num = num >> 8;
    }
}

// Push litle endian bytes  
void dbuffer_pushInt(dbuffer_t *dbuffer, unsigned int num, size_t bytes) {
    assert(bytes <= 4 && "invalid int size");
    //num = num << (4 - bytes) * 8;
    
    for (int i = 0; i < bytes; i++) {
        printf("%d\n", num);
        dbuffer_pushChar(dbuffer, num & 0xFF);
        num = num >> 8;
    }
} 

void dbuffer_push(dbuffer_t *dbuffer, size_t bytes, ...) {
    va_list list;    
    va_start(list, bytes);
    unsigned char buffer[bytes];
    
    for (size_t i = 0; i < bytes; i++)
        buffer[i] = (unsigned char)va_arg(list, unsigned int);
    
    dbuffer_pushData(dbuffer, buffer, bytes);
 
    va_end(list);
}

void dbuffer_pushChars(dbuffer_t *dbuffer, char c, size_t size) {
    dbuffer_ensureCap(dbuffer, size);
    memset(dbuffer->buffer + dbuffer->usage, c, size);     
    dbuffer->usage += size;
}

void dbuffer_pushRange(dbuffer_t *dbuffer, range_t *range) {
    dbuffer_pushData(dbuffer, range->ptr, range->size);
}

range_t dbuffer_asRange(dbuffer_t *dbuffer) {
    return (range_t) {.ptr=dbuffer->buffer, .size=dbuffer->usage};
}

#endif
