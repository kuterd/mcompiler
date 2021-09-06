#include "ir.h"
#include "parser.h"
#include "ssa_conversion.h"
#include "dominators.h"
#include "ir_creation.h"

char *exp = 
    "void test() {                     "
    "   int64 a = 123;                 "
    "   if (a == 1) {                  "
    "     a = 312;                     "
    "   }                              "
    "   if (a == 2) {                  "
    "     a = 456;                     "
    "   }                              "
    "}                                 ";

int main (int argc, char *args[]) {
    
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
    struct ir_context ctx;
    ir_context_init(&ctx);

    struct ir_creator creator;
    ir_creator_init(&creator, &ctx);    
 
    struct function *function = 
        ir_creator_createFunction(&creator, AST_AS_TYPE(node, function));     
    struct basic_block *block = function->entry;

    // --- Covnert to SSA based IR. ---
    struct dominators doms;
    dominators_compute(&doms, block);    

    struct domfrontiers df;
    domfrontiers_compute(&df, &doms);
 
    struct ir_print_annotations anno;
    anno.doms = &doms;
    anno.df = &df;

//    function_dumpDot(&ctx, function, NULL);
   
    ssa_convert(&ctx, function, &doms, &df); 
    function_dumpDot(&ctx, function, NULL);
    
    //dominators_dumpDot(&doms, &ctx);

    dominators_free(&doms);
    return 0;
}
