#include "parser.h"
#include "codegen.h"
#include "x86_64.h"
#include "buffer.h"


int main() {
    char *exp = "10 + (20 * 30) + (10 * 30 + 10)";
    range_t expRange = range_fromString(exp);
    parser_t parser;
    parser_init(&parser, expRange);

    struct ast_node *node = parser_parseExpression(&parser);
 


    return 0;
}
