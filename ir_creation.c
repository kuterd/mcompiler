#include "ir_creation.h"
#include "hashmap.h"
#include "ir.h"
#include "parser.h"
#include "utils.h"

// TODO: Handle errors gracefully.
// This creates non SSA IR, SSA conversion happens later.

struct variable {
    size_t rId;
    enum token_type dataType;
    struct hm_bucket_entry entry;
};

// information about variables/declarations inside this block.
// we need might need to look at parents.
struct block_info {
    hashmap_t variableMap;

    zone_allocator zone;
    struct block_info *parent;
};

enum data_type convertDataType(enum token_type type) {
    switch (type) {
    case TK_KW_INT64:
        return INT64;
    case TK_KW_VOID:
        return VOID;
    }
}

// NOTE: A parser block can be one or more IR "basic_block".

#define _ creator->

void ir_creator_init(struct ir_creator *creator, ir_context_t *ctx) {
    _ regCount = 0;
    _ block = NULL;
    _ blockInfo = NULL;
    _ ctx = ctx;
}

basic_block_t *create_block(struct ir_creator *creator, struct ast_block *block,
                            basic_block_t **last);

function_t *ir_creator_createFunction(struct ir_creator *creator,
                                      struct ast_function *func) {
    assert(func->childCount == func->argumentCount + 1 &&
           "Incorrect child count for function");

    function_t *result = ir_new_function(_ ctx, func->name);
    _ function = result;

    // FIXME: When we add arguments.
    result->entry =
        create_block(creator, AST_AS_TYPE(func->childs[0], block), NULL);
    return result;
}

struct variable *bInfo_getReg(struct block_info *bInfo, range_t range) {
    struct hm_bucket_entry *entry =
        hashmap_getRange(&bInfo->variableMap, range);
    if (entry)
        return containerof(entry, struct variable, entry);
    return NULL;
}

// Traverse blocks upwards, find the reg
struct variable *_findReg(struct ir_creator *creator, range_t range) {
    struct block_info *current = creator->blockInfo;

    while (current) {
        struct variable *var = bInfo_getReg(current, range);
        if (var)
            return var;
        current = current->parent;
    }
    assert(0 && "Couldn't find reg");
    return NULL;
}

void _createReg(struct ir_creator *creator, range_t range,
                enum token_type dataType) {
    struct block_info *bInfo = _ blockInfo;
    struct hm_bucket_entry *entry =
        hashmap_getRange(&bInfo->variableMap, range);
    assert(!entry && "Was expecting the entry to be empty");
    struct variable *vInfo = znnew(&bInfo->zone, struct variable);
    hashmap_setRange(&bInfo->variableMap, range, &vInfo->entry);
    vInfo->rId = creator->regCount++;
    vInfo->dataType = dataType;
}

value_t *create_value(struct ir_creator *creator, struct ast_node *node);

value_t *create_number(struct ir_creator *creator, struct ast_number *number) {
    return &ir_constant_value(_ ctx, number->num)->value;
}

value_t *create_value(struct ir_creator *creator, struct ast_node *node);

instruction_t *create_assignment(struct ir_creator *creator,
                                 struct ast_binary_exp *exp) {
    struct ast_variable *variable = AST_AS_TYPE(exp->left, variable);
    struct variable *var = _findReg(creator, variable->varName);

    value_t *val = create_value(creator, exp->right);
    inst_assign_var_t *assign =
        inst_new_assign_var(creator->ctx, var->rId, val);

    block_insert(_ block, &assign->inst);
}

value_t *create_binary(struct ir_creator *creator,
                       struct ast_binary_exp *binary) {
    enum binary_ops op;
    switch (binary->op) {
    case TK_PLUS:
        op = BO_ADD;
        break;
    case TK_MINUS:
        op = BO_SUB;
        break;
    case TK_MUL:
        op = BO_MUL;
        break;
    case TK_DIV:
        op = BO_DIV;
        break;
    case TK_GREATER:
        op = BO_GREATER;
        break;
    case TK_LESS_THAN:
        op = BO_LESS;
        break;
    case TK_GREATER_EQ:
        op = BO_GREATER_EQ;
        break;
    case TK_LESS_EQ:
        op = BO_LESS_EQ;
        break;
    case TK_EQUALS:
        op = BO_EQUALS;
        break;
    default:
        assert(0 && "Unchandled binary operator");
        // Add all.
    }
    value_t *left = create_value(creator, binary->left);
    value_t *right = create_value(creator, binary->right);

    inst_binary_t *result = inst_new_binary(_ ctx, op, left, right);

    block_insert(_ block, &result->inst);
    return &result->inst.value;
}

instruction_t *create_variable(struct ir_creator *creator,
                               struct ast_variable *var) {
    struct variable *varInfo = _findReg(creator, var->varName);
    inst_load_var_t *loadVar = inst_new_load_var(
        _ ctx, varInfo->rId, convertDataType(varInfo->dataType));
    block_insert(_ block, &loadVar->inst);
    return &loadVar->inst;
}

value_t *create_value(struct ir_creator *creator, struct ast_node *node) {
    // TODO: Handle calls.
    switch (node->type) {
    case BINARY_EXP:
        return create_binary(creator, AST_AS_TYPE(node, binary_exp));
    case VARIABLE:
        return &create_variable(creator, AST_AS_TYPE(node, variable))->value;
    case NUMBER:
        return create_number(creator, AST_AS_TYPE(node, number));
    defualt:
        assert(0 && "Unknown node type for value creation");
    }
    return NULL;
}

void create_statement(struct ir_creator *creator, struct ast_node *node) {
    if (node->type == BINARY_EXP) {
        struct ast_binary_exp *exp = AST_AS_TYPE(node, binary_exp);
        assert(exp->op == TK_ASSIGN && "statement must be a assignment");

        create_assignment(creator, exp);
    } else if (node->type == IF) {
        struct ast_if *if_node = AST_AS_TYPE(node, if);
        value_t *cond = create_value(creator, if_node->condition);

        basic_block_t *last;
        basic_block_t *bblock =
            create_block(creator, AST_AS_TYPE(if_node->ifBlock, block), &last);

        // basic_block can only have a jump at the end, so we need to create a
        // new block for the rest of this function. we do not need to create a
        // new block info since the variable scope is the same.

        basic_block_t *rest = block_new(_ ctx, _ function);
        inst_jump_cond_t *cjump = inst_new_jump_cond(_ ctx, bblock, rest, cond);

        block_insert(_ block, &cjump->inst);

        // When the block associated with the if is complete, we want to jump to
        // the rest block to continue execution.
        inst_jump_t *jump = inst_new_jump(_ ctx, rest);
        block_insert(last, &jump->inst);

        _ block = rest;
    } else if (node->type == WHILE) {
        struct ast_while *while_node = AST_AS_TYPE(node, while);

        // this is where the loop condition lives.
        basic_block_t *head = block_new(_ ctx, _ function);

        // jump to the head on entry.
        inst_jump_t *ejump = inst_new_jump(_ ctx, head);
        block_insert(_ block, &ejump->inst);

        basic_block_t *last;
        // the loop body.
        basic_block_t *body =
            create_block(creator, AST_AS_TYPE(while_node->block, block), &last);
        // after we finished executing the loop body jump back to the loop head.
        inst_jump_t *jump = inst_new_jump(_ ctx, head);
        block_insert(body, &jump->inst);

        // this is the part of code that comes after the function.
        basic_block_t *exit = block_new(_ ctx, _ function);

        // Materialize the condition.
        _ block = head;
        value_t *cond = create_value(creator, while_node->condition);

        // jump to body or exit.
        inst_jump_cond_t *cjump = inst_new_jump_cond(_ ctx, body, exit, cond);
        block_insert(_ block, &cjump->inst);

        // we are done.
        _ block = exit;
    } else if (node->type == DECLARATION) {
        struct ast_declaration *decl = AST_AS_TYPE(node, declaration);
        struct ast_binary_exp *exp = AST_AS_TYPE(decl->assignment, binary_exp);
        struct ast_variable *var = AST_AS_TYPE(exp->left, variable);
        _createReg(creator, var->varName, decl->dataType);

        create_assignment(creator, exp);
    } else if (node->type == RETURN) {
        inst_return_t *returnInst = inst_new_return(_ ctx);
        block_insert(_ block, &returnInst->inst);
    }
}

basic_block_t *create_block(struct ir_creator *creator, struct ast_block *block,
                            basic_block_t **last) {
    struct block_info *blockInfo = malloc(sizeof(struct block_info));
    hashmap_init(&blockInfo->variableMap, rangeKeyType);
    blockInfo->parent = _ blockInfo;
    creator->blockInfo = blockInfo;
    zone_init(&blockInfo->zone);

    basic_block_t *bblock = block_new(_ ctx, _ function);
    basic_block_t *oldBlock = _ block;
    _ block = bblock;

    // Create statements.
    for (size_t i = 0; i < block->childCount; i++) {
        struct ast_node *child = block->childs[i];
        create_statement(creator, child);
    }

    zone_free(&blockInfo->zone);
    hashmap_free(&blockInfo->variableMap);

    // Processing the statements can change the value of creator->block, for
    // example when processing a if block. Sometimes we need the last block.
    if (last != NULL)
        *last = _ block;

    // Return to the old block context.
    _ blockInfo = _ blockInfo->parent;
    _ block = oldBlock;

    free(blockInfo);

    return bblock;
}
