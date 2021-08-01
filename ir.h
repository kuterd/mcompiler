#ifndef IR_H
#define IR_H

#include "buffer.h"
#include "hashmap.h"
#include "dominators.h"

#define INST_TYPE(prefix) struct inst_##prefix

#define IR_VALUE_AS_TYPE(ptr, type) containerof(ptr, type, value)
#define IR_INST_AS_TYPE(ptr, type) containerof(ptr, type, inst)
#define IR_VALUE_AS_INST(ptr, type) containerof(containerof(ptr, struct instruction, value), type, inst)

// Get value from instruction, only valid if there is a value.
// type is the type of the instruction.

// load_var, store_var instructions are only valid before SSA conversion.
// instructions that can jump to other blocks con only be the last instruction inside a basic_block

// o(enum, pretty_name)
#define INSTRUCTIONS(o)                 \
    o(INST_PHI, phi)                    \
    o(INST_LOAD_VAR, load_var)          \
    o(INST_ASSIGN_VAR,assign_var)       \
    o(INST_BINARY, binary)              \
    o(INST_JUMP, jump)                  \
    o(INST_JUMP_COND, jump_cond)        \
    o(INST_FUNCTION_CALL, function_call)\
    o(INST_RETURN, return)              \

extern char *kInstNames[];
extern char *kDataTypeNames[];
enum data_type {
    VOID,
    INT64,
    PTR,
    DT_BLOCK,
};

enum value_type {
    INST,
    CONST,
    UNKNOWN_CONST, 
    ARGUMENT,
    V_BLOCK
};

struct ir_context {
    struct list_head functions;
    // The allocator for all IR objects.
    zone_allocator alloc;
};

// anything that has a value.
struct value {
    enum data_type dataType;
    enum value_type type;

    struct list_head uses; 
    range_t name;
};

// Argument of a function.
struct value_argument {
   struct value value; 
};

struct function {
    // Function is a UNKNOWN_CONST PTR
    struct value value;
    struct basic_block *entry;
    struct list_head functions;
    enum data_type returnType;
    size_t argumentCount; 
    size_t valueNameCounter;
};

struct basic_block {
    // We consider basic block a value.
    // NOTE: Should this be a pointer too ? 
    struct value value;
    // linked list of instructions.
    struct list_head instructions;
    
    // The function that contains this block.
    struct function *parent;
};

struct instruction;

struct value_constant {
    struct value value;

    // fix this when we add other sizes.
    union {    
        int64_t number;
    };
};

#define COMMA_SECOND(a, b) a,
enum instruction_type {
    INSTRUCTIONS(COMMA_SECOND)
};
#undef COMMA_SECOND

// This is the edge between a instruction and a value.
// a value can have mulitple users.
struct use {
    struct instruction *inst;// :(
    struct value *value;
    struct list_head useList;
};

struct instruction {
    struct value value;
    struct list_head inst_list; 
    enum instruction_type type;
     
    struct basic_block *parent;
};

struct inst_phi {
    struct instruction inst;
    size_t useCount;
    
    struct use **uses;
};

enum binary_ops {
    BO_ADD,
    BO_SUB,
    BO_MUL,
    BO_DIV
    // more to come.
};

struct inst_binary {
    struct instruction inst;
    enum binary_ops op;
    union {
        struct use *uses[2];
        struct {
            struct use *left;
            struct use *right;
        };
    };
};

struct inst_load_var {
    struct instruction inst;
    size_t rId;
    struct use *uses[0];
};

struct inst_assign_var {
    struct instruction inst;
    size_t rId;
    
    union {
       struct use *uses[1];
       struct use *var;
    };
};

struct inst_function_call {
    struct instruction inst;
    size_t useCount;
    struct use **uses;
};

struct inst_jump {
    struct instruction inst;
    struct use *uses[1];
};

struct inst_jump_cond {
    struct instruction inst;
    struct use *uses[3];
};

struct inst_return {
    struct instruction inst;
    size_t hasReturn;
    struct use *uses[1];
};

// Optional stuff that we can print in the IR.
struct ir_print_annotations {
    struct dominators *doms;
    struct domfrontiers *df;
};

void value_setName(struct ir_context *ctx, struct value *value, range_t name);
range_t value_getName(struct ir_context *ctx, struct value *value);

void function_dump(struct ir_context *ctx, struct function *fun, struct ir_print_annotations *annotations);
void function_dumpDot(struct ir_context *ctx, struct function *fun, struct ir_print_annotations *annotations);
void inst_dump(struct ir_context *ctx, struct instruction *inst);
//void block_dump(struct ir_context *ctx, struct basic_block *block);

void ir_context_init(struct ir_context *context);
void ir_context_free(struct ir_context *context);

struct inst_load_var* inst_new_load_var(struct ir_context *ctx, size_t i, enum data_type type); 
struct inst_assign_var* inst_new_assign_var(struct ir_context *ctx, size_t i, struct value *value); 
struct inst_binary* inst_new_binary(struct ir_context *ctx, enum binary_ops type, struct value *a, struct value *b); 
struct inst_jump* inst_new_jump(struct ir_context *ctx, struct basic_block *block); 
struct inst_jump_cond* inst_new_jump_cond(struct ir_context *ctx, struct basic_block *a, struct basic_block *b, struct value *cond);
struct inst_return* inst_new_return(struct ir_context *ctx);

struct function* ir_new_function(struct ir_context *context, range_t name);

void inst_setUse(struct ir_context *ctx, struct instruction *inst, size_t useOffset, struct value *value);
void inst_replaceUse(struct use **use, struct value *value);

void inst_insertAfter(struct instruction *inst, struct instruction *add);

struct basic_block* block_new(struct ir_context *ctx, struct function *fn);
void block_insert(struct basic_block *block, struct instruction *inst);

struct use** inst_getUses(struct instruction *inst, size_t *count);
struct value_constant* ir_constant_value(struct ir_context *ctx, int64_t value);

/// ---- Iterators ----

struct block_predecessor_it {
    struct list_head *list;
    struct list_head *current;
};

struct block_predecessor_it block_predecessor_begin(struct basic_block *block);
int block_predecessor_end(struct block_predecessor_it it);
struct block_predecessor_it block_predecessor_next(struct block_predecessor_it it);
struct basic_block* block_predecessor_get(struct block_predecessor_it it);

struct block_successor_it {
    struct basic_block *next;    
    struct instruction *inst;
    size_t i;
};

int block_successor_end(struct block_successor_it it);
struct block_successor_it block_successor_begin(struct basic_block *block);
struct block_successor_it block_successor_next(struct block_successor_it it);
struct basic_block* block_successor_get(struct block_successor_it it);

#endif
