#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <limits.h>

#include "buffer.h"

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

int range_parseInt(range_t range) {
    assert(range.size != 0 && "Empty range");
   
    // Signed int overflow is undefined behaviour. 
    long result = 0;
    int sign = 1;

    if (*range.ptr == '-') {
        range_skip(&range, 1);
        sign = -1;
    } else if (*range.ptr == '+') {
        range_skip(&range, 1);
    }

    for (int i = 0; i < range.size; i++) {
        result *= 10;
        result += range.ptr[i] - '0';
        if (sign == 1 && result >= INT_MAX) 
            break;
        else if (sign == -1 && result - 1 >= INT_MAX)
            break;
        //FIXME: Error handling. 
    }

    return (int)(sign == -1 ? -result : result); 
}

void position_ingest(position_t *position, char c) {
    position->offset++;
    if (c == '\n') {
        position->line++;
        position->col = 0;
    } else if (c != '\r') {
        position->col++; 
    }
}

void position_dump(position_t *position) {
    printf("Line: %u, Column: %u, Offset: %u\n", position->line, position->col, position->offset);
}

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

void dbuffer_removeRange(dbuffer_t *dbuffer, size_t offset, size_t size) {
    assert(dbuffer->usage >= size && "Invalid size");
    assert(offset < dbuffer->usage && "Invalid offset");
        
    size_t offsetEnd = offset + size;

    // Move memory that comes after the range to the begining.
    memmove(dbuffer->buffer + offset, dbuffer->buffer + offsetEnd, dbuffer->usage - offsetEnd);

    // Reduce the usage since we removed @size bytes.    
    dbuffer->usage -= size; 
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
    assert(bytes <= sizeof(size_t) && "invalid int size");
    //num = num << (4 - bytes) * 8;
    
    for (int i = 0; i < bytes; i++) {
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

void dbuffer_pushPtr(dbuffer_t *dbuffer, void *pointer) {
    dbuffer_ensureCap(dbuffer, sizeof(void*));
    dbuffer_pushData(dbuffer, &pointer, sizeof(void*));
}

void dbuffer_popPtr(dbuffer_t *dbuffer) {
    dbuffer->usage -= sizeof(void*);
}

void* dbuffer_getLastPtr(dbuffer_t *dbuffer) {
    return ((void**)dbuffer->buffer)[dbuffer->usage / sizeof(void*) - 1];
}

void dbuffer_pushUIntHexStr(dbuffer_t *dbuffer, unsigned int number) {
    size_t size = 0;   
    do {
        char f = number % 16;
        
        if (f < 10)
            f += '0';
        else
            f += 'A';

        dbuffer_pushChar(dbuffer, '0' + number % 10);
        number /= 10;
        size++;
    } while (number != 0);
   
    char *begin = dbuffer->buffer + dbuffer->usage - size; 
    for (size_t i = 0, count = size / 2; i < count; i++) {
        char tmp = begin[i];
        begin[i] = begin[size - i - 1];
        begin[size - i - 1] = tmp;
    } 
}

void dbuffer_pushUIntStr(dbuffer_t *dbuffer, unsigned int number) {
    size_t size = 0;   
    do {
        dbuffer_pushChar(dbuffer, '0' + number % 10);
        number /= 10;
        size++;
    } while (number != 0);
   
    char *begin = dbuffer->buffer + dbuffer->usage - size; 
    for (size_t i = 0, count = size / 2; i < count; i++) {
        char tmp = begin[i];
        begin[i] = begin[size - i - 1];
        begin[size - i - 1] = tmp;
    } 
}

void dbuffer_pushIntStr(dbuffer_t *dbuffer, int number) {
    if (number < 0) {
        dbuffer_pushChar(dbuffer, '-');
        number = -number;
    }
    dbuffer_pushUIntStr(dbuffer, (unsigned int)number); 
}


range_t dbuffer_asRange(dbuffer_t *dbuffer) {
    return (range_t) {.ptr=dbuffer->buffer, .size=dbuffer->usage};
}

void** dbuffer_asPtrArray(dbuffer_t *dbuffer, size_t *size) {
    *size = dbuffer->usage / sizeof(void*);
    return (void**)dbuffer->buffer;
}

void dbuffer_swap(dbuffer_t *a, dbuffer_t *b) {
    dbuffer_t bk = *a;
    *a = *b;
    *b = bk; 
}

void dbuffer_clear(dbuffer_t *dbuffer) {
    dbuffer->usage = 0;
}

void dbuffer_free(dbuffer_t *dbuffer) {
    free(dbuffer->buffer);
}


