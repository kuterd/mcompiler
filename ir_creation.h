#ifndef IR_CREATION_H
#define IR_CREATION_H

struct ir_creator {
    // Variable to virtual register mapping.
    size_t regCount;

    // Current Function.
    function_t *function;

    // Current Block.
    basic_block_t *block;
    struct block_info *blockInfo;
    ir_context_t *ctx;
};

void ir_creator_init(struct ir_creator *creator, ir_context_t *ctx);
function_t *ir_creator_createFunction(struct ir_creator *creator,
                                      struct ast_function *func);

#endif
