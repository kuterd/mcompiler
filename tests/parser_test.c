#include "format.h"
#include "parser.h"

char *exp = "void test() {                     "
            "   if (a == 1) {                  "
            "        int64 b = (20 * 10 + 2);  "
            "        int64 a = 20;             "
            "   }                              "
            "}                                 ";

int main() {
    range_t expRange = range_fromString(exp);
    parser_t parser;
    parser_init(&parser, expRange);
    token_dumpAll(exp);
    struct ast_node *node = parser_parseFunction(&parser);

    if (node) {
        ast_dump(node, 0);
    } else if (parser.error) {
        puts(parser.error);
    } else {
        puts("unknown parser error");
    }
    return 0;
}
