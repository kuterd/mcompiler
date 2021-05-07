// General buffer utilities.

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

void range_skip(range_t *range, size_t amount);

void range_shorten(range_t *range, size_t amount);
range_t range_subs(range_t *range, size_t size);
range_t range_sub(range_t *range, size_t start, size_t size);

int range_parseInt(range_t range);

// Please use the RANGE_STRING if the string is a constant.
range_t range_fromString(char *string);
int range_cmp(range_t a, range_t b);
range_t range_copy(range_t *range);

// Used to track position inside a text file, used for parsing.
typedef struct {
    size_t line;
    size_t col;
    size_t offset;
} position_t;

void position_ingest(position_t *position, char c);

void position_dump(position_t *position);

// Dynamicly sized buffer.
// Will adjust it's size to fit all elements.
// Currently it can only grow.
typedef struct {
    void *buffer;
    size_t capacity;
    size_t usage;
} dbuffer_t;

void dbuffer_write(dbuffer_t *dbuffer, FILE *file);
void dbuffer_initSize(dbuffer_t *dbuffer, size_t size);
void dbuffer_init(dbuffer_t *dbuffer);

#define max(a, b) ((a) > (b) ? (a) : (b))

void dbuffer_ensureCap(dbuffer_t *dbuffer, size_t size);

void dbuffer_removeRange(dbuffer_t *dbuffer, size_t offset, size_t size);
 
void dbuffer_pushData(dbuffer_t *dbuffer, void *buffer, size_t size);

void dbuffer_pushStr(dbuffer_t *dbuffer, void *str);

void dbuffer_pushChar(dbuffer_t *dbuffer, unsigned char ch);

// Push litle endian bytes  
void dbuffer_pushLong(dbuffer_t *dbuffer, unsigned long num, size_t bytes);

// Push litle endian bytes  
void dbuffer_pushInt(dbuffer_t *dbuffer, unsigned int num, size_t bytes);

void dbuffer_push(dbuffer_t *dbuffer, size_t bytes, ...);

void dbuffer_pushChars(dbuffer_t *dbuffer, char c, size_t size);

void dbuffer_pushRange(dbuffer_t *dbuffer, range_t *range);

void dbuffer_pushPointer(dbuffer_t *dbuffer, void *pointer);

void dbuffer_popPointer(dbuffer_t *dbuffer);

void dbuffer_pushUIntHexStr(dbuffer_t *dbuffer, unsigned int number);

void dbuffer_pushUIntStr(dbuffer_t *dbuffer, unsigned int number);

void dbuffer_pushIntStr(dbuffer_t *dbuffer, int number);
 
range_t dbuffer_asRange(dbuffer_t *dbuffer);

void dbuffer_swap(dbuffer_t *a, dbuffer_t *b);

void dbuffer_clear(dbuffer_t *dbuffer);

void dbuffer_free(dbuffer_t *dbuffer);



#endif
