#include "ir.h"
#include "dominators.h"
#include "format.h"
#include <stdio.h>
/*
struct basic_block* create_test_block(struct ir_, fncontext *ctx, struct function *fn, struct basic_block *prev) {
    struct basic_block *block = block_new(ctx, fn);
    struct inst_jump *jmp = inst_new_jump(ctx, block);
    block_insert(prev, &jmp->inst);
}
*/
struct basic_block* create_test_cfg(struct ir_context *ctx, struct function *fn) {
    int i = 0;
    struct basic_block *prev = block_new(ctx, fn);
    prev->value.name = format_range("{int}", i + 1);
    
    struct basic_block *result = prev;
    for (i = 1; i < 5; i++) {
        struct basic_block *nblock = block_new(ctx, fn); 
        nblock->value.name = format_range("{int}", i + 1);
 
        struct inst_jump *jmp = inst_new_jump(ctx, nblock);
        block_insert(prev, &jmp->inst);
        prev = nblock;
    }
    block_insert(prev, &inst_new_return(ctx)->inst);
    return result; 
}

int main(int argc, char *args[]) {
    struct ir_context ctx;
    ir_context_init(&ctx);    
    
    struct function *fun = ir_new_function(&ctx, RANGE_STRING("test"));
    
    struct basic_block *entry = block_new(&ctx, fun);
    entry->value.name = RANGE_STRING("entry"); 
    struct basic_block *a = block_new(&ctx, fun);
    a->value.name = RANGE_STRING("a"); 
    struct basic_block *b = block_new(&ctx, fun);
    b->value.name = RANGE_STRING("b"); 
    
    struct value_constant *cnst = ir_constant_value(&ctx, 1); 


    struct inst_jump_cond *cond = inst_new_jump_cond(&ctx, a, b, &cnst->value);
    block_insert(entry, &cond->inst);
    fun->entry = entry;    

    struct dominators doms;
    dominators_compute(&doms, entry);

    dominators_dumpDot(&doms, &ctx);
/*
    struct domfrontiers df;
    domfrontiers_compute(&df, &doms);

    struct ir_print_annotations anno;
    anno.doms = &doms;
    anno.df = &df;
    function_dump(&ctx, fun, &anno);
*/
    ir_context_free(&ctx);
    return 0;
}
