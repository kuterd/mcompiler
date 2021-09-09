// General buffer utilities.

#ifndef DBUFFER_H
#define DBUFFER_H

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#define DBUFFER_INIT 1000

#define RANGE_STRING(str) ((range_t){.ptr = (str), .size = sizeof(str) - 1})
#define RANGE_COMPARE(r, s) range_cmp((r), RANGE_STRING(s))

// Used to represent a memory range, like std::string_view
typedef struct {
    char *ptr;
    size_t size;
} range_t;

// skip @amount bytes
void range_skip(range_t *range, size_t amount);

void range_shorten(range_t *range, size_t amount);

range_t range_subs(range_t *range, size_t size);

// get a sub range from the range [@start, @start + @size]
range_t range_sub(range_t *range, size_t start, size_t size);

// parse a int represented as a range.
int range_parseInt(range_t range);

// Please use the RANGE_STRING if the string is a constant.
range_t range_fromString(char *string);

// Compare range.
int range_cmp(range_t a, range_t b);

// Create a copy of range.
range_t range_copy(range_t *range);

// Used to track position inside a text file, used for parsing.
typedef struct {
    size_t line;
    size_t col;
    size_t offset;
} position_t;

// Process a character. this will adjust the line and col
// fields of the position
void position_ingest(position_t *position, char c);

// print the position.
void position_dump(position_t *position);

// Dynamicly sized buffer.
// Will adjust it's size to fit all elements.
// Currently it can only grow.
typedef struct {
    void *buffer;
    size_t capacity;
    size_t usage;
} dbuffer_t;

// write dbuffer to file
void dbuffer_write(dbuffer_t *dbuffer, FILE *file);
// initialize with a initial size.
void dbuffer_initSize(dbuffer_t *dbuffer, size_t size);
// initialize a dbuffer with the default size.
void dbuffer_init(dbuffer_t *dbuffer);

#define max(a, b) ((a) > (b) ? (a) : (b))

// ensure that we have @size bytes available.
void dbuffer_ensureCap(dbuffer_t *dbuffer, size_t size);

// remove a memory range from the dbuffer. this moves bytes around.
void dbuffer_removeRange(dbuffer_t *dbuffer, size_t offset, size_t size);

// push memory range.
void dbuffer_pushData(dbuffer_t *dbuffer, void *buffer, size_t size);

// push null delimitered string.
void dbuffer_pushStr(dbuffer_t *dbuffer, void *str);

// push char
void dbuffer_pushChar(dbuffer_t *dbuffer, unsigned char ch);

// Push litle endian bytes
void dbuffer_pushLong(dbuffer_t *dbuffer, unsigned long num, size_t bytes);

// Push litle endian bytes
void dbuffer_pushInt(dbuffer_t *dbuffer, unsigned int num, size_t bytes);

// push variable argument
void dbuffer_push(dbuffer_t *dbuffer, size_t bytes, ...);

// push a char repeatadly
void dbuffer_pushChars(dbuffer_t *dbuffer, char c, size_t size);

// push a range to dbuffer.
void dbuffer_pushRange(dbuffer_t *dbuffer, range_t *range);

// push ptr to the dbuffer
void dbuffer_pushPtr(dbuffer_t *dbuffer, void *pointer);

// pop pointer from dbuffer
void dbuffer_popPtr(dbuffer_t *dbuffer);

// get the last pointer value.
void *dbuffer_getLastPtr(dbuffer_t *dbuffer);

// push unsigned int as str.
void dbuffer_pushUIntHexStr(dbuffer_t *dbuffer, unsigned int number);

// Push unsigned int as str
void dbuffer_pushUIntStr(dbuffer_t *dbuffer, unsigned int number);

// Push Int as str.
void dbuffer_pushIntStr(dbuffer_t *dbuffer, int number);

// get as a range.
range_t dbuffer_asRange(dbuffer_t *dbuffer);

// get as a poiner list from the dbuffer.
void **dbuffer_asPtrArray(dbuffer_t *dbuffer, size_t *size);

// Swap the internals of dbuffers.
void dbuffer_swap(dbuffer_t *a, dbuffer_t *b);

// clear the contents of the dbuffer.
void dbuffer_clear(dbuffer_t *dbuffer);

void dbuffer_free(dbuffer_t *dbuffer);

#endif
