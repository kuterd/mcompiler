#include "dominators.h"
#include "ir.h"
#include "ir_creation.h"
#include "parser.h"
#include "ssa_conversion.h"

char *exp = "void fib() {                       "
            "  int64 num = 100;                 "
            "  int64 a = 1;                     "
            "  int64 b = 0;                     "
            "  while(num > 0) {                 "
            "    num = num - 1;                 "
            "    int64 o = b;                   "
            "    b = a;                         "
            "    a = a + o;                     "
            "  }                                "
            "  return;                          "
            "}                                  ";

int main(int argc, char *args[]) {
    //  --- Parse the function. ---
    range_t expRange = range_fromString(exp);
    parser_t parser;
    parser_init(&parser, expRange);
    struct ast_node *node = parser_parseFunction(&parser);

    if (parser.error)
        puts(parser.error);
    else if (!node)
        puts("unknown parser error");

    // --- Convert to IR. ---
    ir_context_t ctx;
    ir_context_init(&ctx);

    struct ir_creator creator;
    ir_creator_init(&creator, &ctx);

    function_t *function =
        ir_creator_createFunction(&creator, AST_AS_TYPE(node, function));
    basic_block_t *block = function->entry;

    // --- Covnert to SSA based IR. ---
    struct dominators doms;
    dominators_compute(&doms, block);

    struct domfrontiers df;
    domfrontiers_compute(&df, &doms);

    struct ir_print_annotations anno;
    anno.doms = &doms;
    anno.df = &df;

    ssa_convert(&ctx, function, &doms, &df);
    function_dumpDot(&ctx, function, &anno);

    dominators_free(&doms);
    return 0;
}
