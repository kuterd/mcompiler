#include "parser.h"
#include "ir.h"
#include "ir_creation.h"
#include "dominators.h"

char *exp = 
    "void test() {                     "
    "   int64 a = 10 * 20 / 2;         "
    "   if (a >= 20 + 10) {            "
    "        a = 20 * 10 + 2;          "
    "        if (a >= 30)              "
    "           int64 b = 10;          "
    "        int64 c = 10 * 20;        "
    "                                  "
    "   }                              "
    "   a = 10;                        "
    "   int64 c = 12345;               "
    "}                                 ";

int main() {
    range_t expRange = range_fromString(exp);
    parser_t parser;
    parser_init(&parser, expRange);
    struct ast_node *node = parser_parseFunction(&parser);

    if (parser.error)
        puts(parser.error);
    else if (!node)
        puts("unknown parser error");
    
    struct ir_context ctx;
    ir_context_init(&ctx);

    struct ir_creator creator;
    ir_creator_init(&creator, &ctx);    
    function_t *function = 
        ir_creator_createFunction(&creator, AST_AS_TYPE(node, function));     
    basic_block_t *block = function->entry;
 
    function_dumpDot(&ctx, function, NULL);

    return 0;
}
