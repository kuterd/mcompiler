#include "ir.h"
#include <assert.h>

basic_block_t* generate_testBlock(ir_context_t *ctx, function_t *fn, int i) {
    basic_block_t *bb = block_new(ctx, fn);
    
    value_constant_t *con = ir_constant_value(ctx, i);
    inst_assign_var_t *var = inst_new_assign_var(ctx, 1, &con->value);
    block_insert(bb, &var->inst);

    return bb;
}

void test_dumpDot() {
    ir_context_t ctx;    
    ir_context_init(&ctx); 

    function_t *fun = ir_new_function(&ctx, RANGE_STRING("test"));
    basic_block_t *bb = block_new(&ctx, fun);
    fun->entry = bb;

    value_constant_t *con = ir_constant_value(&ctx, 10);
    inst_assign_var_t *var = inst_new_assign_var(&ctx, 1, &con->value);
    inst_load_var_t *load = inst_new_load_var(&ctx, 1, INT64);
    value_setName(&ctx, &load->inst.value, RANGE_STRING("loaded_value"));

    //value_constant_t *a = ir_constant_value(&ctx, 15);
    value_constant_t *b = ir_constant_value(&ctx, 20);

    inst_binary_t *add = inst_new_binary(&ctx, BO_ADD, &load->inst.value, &b->value);

    basic_block_t *b1 = generate_testBlock(&ctx, fun, 13);
    basic_block_t *b2 = generate_testBlock(&ctx, fun, 10);
  
    inst_jump_t *jumpTop = inst_new_jump(&ctx, bb);
    block_insert(b1, &jumpTop->inst);

    inst_jump_cond_t *jump = inst_new_jump_cond(&ctx, b1, b2, &con->value);
    
    block_insert(bb, &var->inst);
    block_insert(bb, &load->inst);
    block_insert(bb, &add->inst);
    block_insert(bb, &jump->inst);

    function_dumpDot(&ctx, fun, NULL);
    ir_context_free(&ctx);
}

void test_replace() {
    ir_context_t ctx;
    ir_context_init(&ctx);
    
    function_t *fun = ir_new_function(&ctx, RANGE_STRING("test"));
    basic_block_t *block = block_new(&ctx, fun);  
    fun->entry = block;
   
    value_t *v1 = &ir_constant_value(&ctx, 123)->value;
    value_t *v2 = &ir_constant_value(&ctx, 321)->value; 

    inst_assign_var_t *assign = inst_new_assign_var(&ctx, 1, v1);
    assert(assign->uses[0]->value == v1); 
 
    block_insert(block, &assign->inst); 
    value_replaceAllUses(v1, v2);
    
    assert(assign->uses[0]->value == v2 && "v2 must have a use"); 
    assert(list_empty(&v1->uses) && "v1 must not have any uses");
    assert(!list_empty(&v2->uses) && "v2 must have uses");
}

int main(int argc, char *args[]) {
    test_dumpDot();
    test_replace();
    return 0;
}
