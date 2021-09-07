#include "ir.h"
#include "dominators.h"
#include "format.h"
#include <stdio.h>
/*
basic_block_t* create_test_block(struct ir_, fncontext *ctx, function_t *fn, basic_block_t *prev) {
    basic_block_t *block = block_new(ctx, fn);
    inst_jump_t *jmp = inst_new_jump(ctx, block);
    block_insert(prev, &jmp->inst);
}
*/
basic_block_t* create_test_cfg(ir_context_t *ctx, function_t *fn) {
    int i = 0;
    basic_block_t *prev = block_new(ctx, fn);
    prev->value.name = format_range("{int}", i + 1);
    
    basic_block_t *result = prev;
    for (i = 1; i < 5; i++) {
        basic_block_t *nblock = block_new(ctx, fn); 
        nblock->value.name = format_range("{int}", i + 1);
 
        inst_jump_t *jmp = inst_new_jump(ctx, nblock);
        block_insert(prev, &jmp->inst);
        prev = nblock;
    }
    block_insert(prev, &inst_new_return(ctx)->inst);
    return result; 
}

int main(int argc, char *args[]) {
    ir_context_t ctx;
    ir_context_init(&ctx);    
    
    function_t *fun = ir_new_function(&ctx, RANGE_STRING("test"));
    
    basic_block_t *entry = block_new(&ctx, fun);
    entry->value.name = RANGE_STRING("entry"); 
    basic_block_t *a = block_new(&ctx, fun);
    a->value.name = RANGE_STRING("a"); 
    basic_block_t *b = block_new(&ctx, fun);
    b->value.name = RANGE_STRING("b"); 
    
    value_constant_t *cnst = ir_constant_value(&ctx, 1); 


    inst_jump_cond_t *cond = inst_new_jump_cond(&ctx, a, b, &cnst->value);
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
