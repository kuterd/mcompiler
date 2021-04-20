// format("{dbuffer}

#ifndef FORMAT_H
#define FORMAT_H

#include <stdio.h>
#include "buffer.h"


// we don't need this once we have the hash map system.
// maybe we can do a cool system with function pointers.
#define NAME_TOKENS(o, a)  \
  o(INT,"int",a)           \
  o(UINT, "uint", a)       \
  o(DBUFFER, "dbuffer", a) \
  o(RANGE, "range", a)     \
  o(PRINTER, "printer", a) \
  o(LIST, "list", a)       \
  o(NULL_STR, "str", a)    \
  o(CURLY_OPEN, "co", a)    \
  o(CURLY_CLOSE, "cc", a)

// eliminate this.
#define FIRST2(a,b,o) o(a)
#define SECOND2(a,b,o) o(b)
#define TWO(a,b,o) o(a,b)

#define TOKEN_TYPES(o) \
  o(EEOF)              \
    o(ERROR)           \
    o(STRING)          \
    NAME_TOKENS(FIRST2, o)


char *format_token_names[] = {TOKEN_TYPES(STR_COMMA)};

enum format_token_type {
  TOKEN_TYPES(COMMA)
};

struct format_token {
    enum format_token_type type;
    range_t contents;
};

void format_token_dump(struct format_token *token) {
    printf("Token type: %s\n", format_token_names[token->type]); 
    //i (token->type == ERROR || token->type == EEOF)
      //  return;
    printf("Token contents: %.*s\n", token->contents.size, token->contents.ptr);
}

#define COMPARE_GEN(TYPE, str)     \
    if (RANGE_COMPARE(range, str)) \
        return TYPE;               \

//TODO use hash map once it's done.
enum format_token_type format_getTokenType(range_t range) {
    NAME_TOKENS(TWO, COMPARE_GEN)
    
    return ERROR;
}

void format_nextToken(range_t *range, struct format_token *token) {
    token->type = ERROR;
    token->contents = *range;
    token->contents.size = 0;
    
    if (range->size == 0) {
        token->type = EEOF;
        return; 
    }
   
    while (*range->ptr != '{' && range->size != 0) {
        token->contents.size++;  
        range_skip(range, 1);
    }

    if (token->contents.size != 0) {
        token->type = STRING;
        return;
    }

    // skip the { 
    token->contents.ptr++;
    range_skip(range, 1);
    token->contents.size = 0;

    while (*range->ptr != '}' && range->size != 0) {
        range_skip(range, 1);
        token->contents.size++;
    }
    
    if (token->contents.size == 0) {
       token->type = ERROR;
        return;
    }
 
    token->type = format_getTokenType(token->contents);
    range_skip(range, 1);
    return;
}

void format_dumpTokens(char *format) {
    range_t range = range_fromString(format);
    struct format_token token;
    do {    
        format_nextToken(&range, &token);     
        format_token_dump(&token);
    } while (token.type != ERROR && token.type != EEOF); 
}


void format_token_emit(dbuffer_t *buffer, struct format_token *token, va_list args) {
    switch (token->type) {
        case UINT: {
            unsigned int num = va_arg(args, unsigned int);
            dbuffer_pushUIntStr(buffer, num);
            break; 
        } 
        case INT: {
            int num = va_arg(args, int);
            dbuffer_pushIntStr(buffer, num);
            break;
        } 
        case NULL_STR: {
            char *ptr = va_arg(args, char*);
            range_t range = range_fromString(ptr);
            dbuffer_pushRange(buffer, &range);
            break;
        }
        case RANGE: {
            range_t range = va_arg(args, range_t);
            dbuffer_pushRange(buffer, &range);
            break;
        }
        case STRING:
            dbuffer_pushRange(buffer, &token->contents);
            break;
        case CURLY_OPEN:
            dbuffer_pushChar(buffer, '{');
            break;
        case CURLY_CLOSE:
            dbuffer_pushChar(buffer, '}');
            break;
    };
}


char *format_list(char *format, va_list args);

char* format(char *format, ...) {
    va_list args;
    va_start(args, format);
    char *result = format_list(format, args);
    va_end(args);
    return result;
}

void format_listDbuffer(char *fmt, dbuffer_t *result, va_list args) {
    range_t range = range_fromString(fmt);
    
    struct format_token token;
    do {    
        format_nextToken(&range, &token);     
        format_token_emit(result, &token, args); 
    } while (token.type != ERROR && token.type != EEOF); 
    
    dbuffer_pushChar(result, 0); 
}

char* format_list(char *fmt, va_list args) {
    dbuffer_t result;
    dbuffer_init(&result);
    format_listDbuffer(fmt, &result, args);

    return result.buffer;
}

void format_print(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char *result = format_list(fmt, args);
    va_end(args);
    printf("%s", result);
    free(result);
}

void format_log(char *prefix, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    printf(prefix);

    char *result = format_list(fmt, args);
    printf("%s", result);
    free(result);
    va_end(args);
}


#undef FIRST2
#undef SECOND2
#undef TWO
#undef TOKEN_TYPES
#endif
