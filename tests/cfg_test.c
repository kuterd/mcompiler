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
    struct basic_block *loop = block_new(&ctx, fun);
    loop->value.name = RANGE_STRING("loop"); 
    struct basic_block *loop2 = block_new(&ctx, fun);
    loop2->value.name = RANGE_STRING("loop2"); 
    
    struct inst_jump *jmp = inst_new_jump(&ctx, loop);
    block_insert(entry, &jmp->inst);

    struct inst_jump *jmp2 = inst_new_jump(&ctx, loop2);
    block_insert(loop, &jmp2->inst);

    struct inst_jump *jmp3 = inst_new_jump(&ctx, loop);
    block_insert(loop2, &jmp3->inst);
//    struct basic_block *entry = create_test_cfg(&ctx);
    fun->entry = entry;    


    struct dominators doms;
    dominators_compute(&doms, entry);

    struct domfrontiers df;
    domfrontiers_compute(&df, &doms);

    struct ir_print_annotations anno;
    anno.doms = &doms;
    anno.df = &df;
    function_dump(&ctx, fun, &anno);

    ir_context_free(&ctx);
    return 0;
}
