#include "ir.h"

struct basic_block* generate_testBlock(struct ir_context *ctx, struct function *fn, int i) {
    struct basic_block *bb = block_new(ctx, fn);
    
    struct value_constant *con = ir_constant_value(ctx, i);
    struct inst_assign_var *var = inst_new_assign_var(ctx, 1, &con->value);
    block_insert(bb, &var->inst);

    return bb;
}

int main(int argc, char *args[]) {
    struct ir_context ctx;    
    ir_context_init(&ctx); 

    struct function *fun = ir_new_function(&ctx, RANGE_STRING("test"));
    struct basic_block *bb = block_new(&ctx, fun);
    fun->entry = bb;

    struct value_constant *con = ir_constant_value(&ctx, 10);
    struct inst_assign_var *var = inst_new_assign_var(&ctx, 1, &con->value);
    struct inst_load_var *load = inst_new_load_var(&ctx, 1, INT64);
    value_setName(&ctx, &load->inst.value, RANGE_STRING("loaded_value"));

    //struct value_constant *a = ir_constant_value(&ctx, 15);
    struct value_constant *b = ir_constant_value(&ctx, 20);

    struct inst_binary *add = inst_new_binary(&ctx, BO_ADD, &load->inst.value, &b->value);

    struct basic_block *b1 = generate_testBlock(&ctx, fun, 13);
    struct basic_block *b2 = generate_testBlock(&ctx, fun, 10);
  
    struct inst_jump *jumpTop = inst_new_jump(&ctx, bb);
    block_insert(b1, &jumpTop->inst);

    struct inst_jump_cond *jump = inst_new_jump_cond(&ctx, b1, b2, &con->value);
    
    block_insert(bb, &var->inst);
    block_insert(bb, &load->inst);
    block_insert(bb, &add->inst);
    block_insert(bb, &jump->inst);

    function_dumpDot(&ctx, fun, NULL);
    ir_context_free(&ctx);
    return 0;
}
