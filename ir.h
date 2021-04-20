#ifndef IR_H
#define IR_H

#include "buffer.h"

#define IR_VALUE_TYPES()
#define IR_VALUE_AS_TYPE(ptr, type) containerof(ptr, type, value)

// Get value from instruction, only valid if there is a value.
// type is the type of the instruction.
#define INST_AS_VALUE(ptr, type) (containerof(ptr, type, inst)->value)

// load_var, store_var instructions are only valid before ssa conversion.

#define INSTRUCTIONS(o)        \
    o(PHI, inst_phi)           \
    o(LOAD_VAR, inst_load_reg) \
    o(ASIGN_VAR, ins_asign_reg)\
    o(JUMP)                    \
    o(JUMP_COND)               \
    o(FUNCTION_CALL)           \

// instructions that produce a value only.
#define INSTRUCTIONS_VALUE(o)  \
    o(PHI)                     \
    o(LOAD_REG)                \
    o(FUNCTION_CALL)

struct function {
    range_t *name;
    struct block *entry;
};

struct ir_context {
    size_t variableCount; 
    range_t *varNames; // varId -> varName
};

struct basic_block {
    // instructions that are contained.
    struct list_head instructions; 
};

void block_getPreds(struct basic_blocks *blocks, size_t *count, struct basic_blocks **childs);

enum data_type {
    VOID,
    INT64
};

enum value_type {
    CONSTANT,
    INST
};

struct instruction;

// anything that has a value.
struct value {
    enum data_type dataType;
    enum value_type type;

    // Users.    
    struct instruction *embededUsers[3];
    dbuffer_t *externalUses;
};

enum instruction_type {
    INSTRUCTIONS(COMMA)
};

// A instruction might not have a value, it takes in values.
struct instruction {
    struct list_head inst_list; 
    enum instruction_type type; 
};

struct phi_incoming {
    struct value *value;
    // the block that this value belongs to.
    struct block *block; 
};

struct inst_phi {
    struct value value;
    struct instruction inst;
    size_t incomingCount;
    struct phi_incoming *incoming;
};

enum binary_ops {
    ADD,
    SUB,
    MUL,
    DIV
    // more to come.
};

struct inst_binary {
    struct instruction inst;
    enum binary_ops op;
    union {
        struct value *uses[2];
        struct {
            struct value *left;
            struct value *right;
        }
    } 
};

struct inst_load_var {
    struct instruction inst;
    struct value value;
    size_t rId;
};

struct inst_store_var {
    struct instruction inst;
    size_t rId;
    struct value *uses[1];
};


#endif
