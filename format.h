// format("{dbuffer}

#ifndef FORMAT_H
#define FORMAT_H

#include "buffer.h"
#include "utils.h"

// TODO use hash map once it's done.
// enum format_token_type format_getTokenType(range_t range);

void format_dumpTokens(char *format);

char *format(char *format, ...);
range_t format_range(char *format, ...);

void format_listDbuffer(char *fmt, dbuffer_t *result, va_list args);
range_t format_list(char *fmt, size_t emitZero, va_list args);
void format_print(char *fmt, ...);
void format_log(char *prefix, char *fmt, ...);
void format_dbuffer(char *fmt, dbuffer_t *buffer, ...);

#endif
