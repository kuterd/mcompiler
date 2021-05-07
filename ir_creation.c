#include "ir.h"
#include "parser.h"
#include "utils.h"
#include "hashmap.h"

// This creates non ssa ir, ssa conversion happens later.

struct variable {
    range_t variableName;
    size_t rId;
    struct hm_bucket_entry entry; 
};

struct ir_creator {
    // Variable to virtual register mapping.
    hashmap_t variableMap;
    size_t regCount;
    
    // Current Function.
    struct function *function;

    // Current Block.
    struct basic_block *block;
};

#define _ creator->

void ir_creator_init(struct ir_creator *creator) {
    hashmap_init(& _ variableMap,  rangeKeyType);
    dbuffer_init(& _ ranges)Map;
    _ regCount = 0;
    _ block = NULL; 
}

size_t _getReg(struct ir_creator *creator, range_t range) {
    struct hm_bucket_entry *entry = hashmap_getRange(_ variableMap, range); 
    if (entry)
        return containerof(entry, struct register_entry, entry)->rId; 
    // error out
    return 0; 
}

//Get or create range, for the current variable.
size_t _getOrCreateReg(struct ir_creator *creator, range_t range) {
    struct hm_bucket_entry *entry = hashmap_getRange(_ variableMap, range); 
    if (entry)
        return containerof(entry, struct register_entry, entry)->rId; 
    dbuffer_pushData(_ ranges, &range, sizeof(range_t));
    
    struct register_entry reg = nnew(struct register_entry); 
    reg->rId = _ regCount++;
    hashmap_setRange(range, &reg->entry); 
    
    return reg->rId;    
}

struct value* create_value(struct ir_creator *creator, struct ast_node *node);

struct value* create_number(struct ir_creator *creator, struct ast_number *number) { 
    return &ir_constant_value(creator->function, number->num)->value;
}

struct instruction* create_binary(struct ir_creator *creator, struct ast_binary_exp *binary) {
    struct value *left = create_value(binary->left); 
    struct value *right = create_value(binary->right);
    
    struct inst_binary *result = inst_new_binary(INT64);  
    ir_setUse(result, &result->left, left); 
    ir_setUse(result, &result->right, right); 

    switch(binary->op) {
        case TK_PLUS:
            result->op = BO_ADD;
            break;
        case TK_MINUS:
            result->op = BO_SUB;
            break;
        case TK_MUL:
            result->op = BO_MUL;
            break;
        case TK_DIV:
             result->op = BO_DIV;
            break;
    }

    return &result->inst;
}

struct intruction* create_variable(struct ir_creator *creator, struct ast_variable *var) {
    struct inst_load_var *var = inst_new_load_var(INT64): 
    var->rId = _getReg(var->varName); 
    
    return &var->inst;
}

struct value* create_value(struct ir_creator *creator, struct ast_node *node) {
    switch(node->type) {
        case BINARY_EXP:
            return &create_binary(creator, AST_AS_TYPE(node, struct ast_binary_exp))->value;
        case VARIABLE:
            return &create_variable(creator, AST_AS_TYPE(node, struct ast_variable))->value;
        case NUMBER:
            return create_number(creator, AST_AS_TYPE(node, struct ast_number));
    }        
}
