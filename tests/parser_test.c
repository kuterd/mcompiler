#include "parser.h"


int main() {
    char *exp = "10 + (20 * 30) + (10 * 30 + 10)";
    range_t expRange = range_fromString(exp);
    parser_t parser;
    parser_init(&parser, expRange);
    token_dumpAll(exp); 
    struct ast_node *node = parser_parseExpression(&parser);
    ast_dump(node, 0);
    return 0;
}
