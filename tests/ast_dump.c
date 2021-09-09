#include "parser.h"
#include "utils.h"

int main() {

    char *exp = "(a + 30 + 10)";
    range_t expRange = range_fromString(exp);

    parser_t parser;
    parser_init(&parser, expRange);
    struct ast_node *node = parser_parseExpression(&parser);

    char *result = ast_dumpDot(node);

    puts(result);

    return 0;
}
