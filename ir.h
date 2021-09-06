#ifndef IR_H
#define IR_H

#include "buffer.h"
#include "hashmap.h"

#define INST_TYPE(prefix) struct inst_##prefix

#define IR_VALUE_AS_TYPE(ptr, type) containerof(ptr, type, value)
#define IR_INST_AS_TYPE(ptr, type) containerof(ptr, type, inst)
#define IR_VALUE_AS_INST(ptr, type) containerof(containerof(ptr, struct instruction, value), type, inst)

// load_var, store_var instructions are only valid before SSA conversion.
// instructions that can jump to other blocks con only be the last instruction inside a basic_block
// macro definition of instruction types.
// o(enum, pretty_name)
#define INSTRUCTIONS(o)                 \
    o(INST_PHI, phi)                    \
    o(INST_LOAD_VAR, load_var)          \
    o(INST_ASSIGN_VAR,assign_var)       \
    o(INST_BINARY, binary)              \
    o(INST_JUMP, jump)                  \
    o(INST_JUMP_COND, jump_cond)        \
    o(INST_FUNCTION_CALL, function_call)\
    o(INST_RETURN, return)

// Names of instructions.
extern char *kInstNames[];

// Names of data types.
extern char *kDataTypeNames[];

// Data types.
enum data_type {
    VOID,
    INT64,
    PTR,
    DT_BLOCK
};

// The IR context, manages all IR objects. 
struct ir_context {
    struct list_head functions;
    // The allocator for all IR objects.
    zone_allocator alloc;
    // Special instructions that contain heap objects.
    struct list_head specialInstructions;
};

#include "dominators.h"

// Type of the value. 
enum value_type {
    INST,
    CONST,
    UNKNOWN_CONST, 
    ARGUMENT,
    V_BLOCK
};

// Anything that has a value.
struct value {
    enum data_type dataType;
    enum value_type type;

    struct list_head uses; 
    range_t name;
};

// Argument of a function.
struct value_argument {
    // Inherit from value.
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

// A block, can only contain a jump at the end
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

// A constant value.
struct value_constant {
    struct value value;

    //FIXME: fix this when we add other sizes.
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
    struct instruction *inst; 
    struct value *value;
    struct list_head useList;
};

// A general instruction, lives inside a basic block.
struct instruction {
    struct value value;
    struct list_head inst_list; 
    enum instruction_type type;
     
    struct basic_block *parent;
};

// A phi instruction, used for the SSA form.
struct inst_phi {
    struct instruction inst;
     
    dbuffer_t useBuffer; 

    // These values must be kept up to date with the useBuffer
    size_t useCount;

    // always in pairs of block and value.
    struct use **uses;

    struct list_head specialList; 
};

enum binary_ops {
    BO_ADD,
    BO_SUB,
    BO_MUL,
    BO_DIV,
    BO_EQUALS,
    BO_LESS,
    BO_GREATER,
    BO_LESS_EQ,
    BO_GREATER_EQ
    // more to come.
};

// A binary instruction(add, subtract, multiply ...).
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

// A load instruction, only valid before SSA conversion.
struct inst_load_var {
    struct instruction inst;
    size_t rId;
    struct use *uses[0];
};

// An assign instruction, only valid before SSA conversion.
struct inst_assign_var {
    struct instruction inst;
    size_t rId;
    
    union {
       struct use *uses[1];
       struct use *var;
    };
};

// A function call instruction.
struct inst_function_call {
    struct instruction inst;
    
    // A function call have variable number of uses,
    // this is needed for passing arguments.
    size_t useCount;
    struct use **uses;
};

// A jump instruction, jumps to a basic block.
struct inst_jump {
    struct instruction inst;
    struct use *uses[1];
};

// A conditional jump instruction, jumps to basic block conditionally.
struct inst_jump_cond {
    struct instruction inst;
    struct use *uses[3];
};

// A return instruction.
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

// set the name of the value. doesn't take ownership, creates a copy. 
void value_setName(struct ir_context *ctx, struct value *value, range_t name);

// get the name of the value, this will assign a name if needed.
range_t value_getName(struct ir_context *ctx, struct value *value);

// replace all uses of a value with another value.
void value_replaceAllUses(struct value *value, struct value *replacement);

// dump a function.
void function_dump(struct ir_context *ctx, struct function *fun, struct ir_print_annotations *annotations);

// dump a function as a dot file.
void function_dumpDot(struct ir_context *ctx, struct function *fun, struct ir_print_annotations *annotations);

// dump a instruction.
void inst_dump(struct ir_context *ctx, struct instruction *inst);

//void block_dump(struct ir_context *ctx, struct basic_block *block);

// initialize the ir context.
void ir_context_init(struct ir_context *context);

// free the ir context
void ir_context_free(struct ir_context *context);

// Create a var load instruction (only valid before ssa conversion)
struct inst_load_var* inst_new_load_var(struct ir_context *ctx, size_t i, enum data_type type);

// Create a assign instruction (only valid before ssa conversion 
struct inst_assign_var* inst_new_assign_var(struct ir_context *ctx, size_t i, struct value *value); 

// Create a binary op instruction.
struct inst_binary* inst_new_binary(struct ir_context *ctx, enum binary_ops type, struct value *a, struct value *b); 

// Create a jump instruction
struct inst_jump* inst_new_jump(struct ir_context *ctx, struct basic_block *block); 

// Create a conditional jump instruction.
struct inst_jump_cond* inst_new_jump_cond(struct ir_context *ctx, struct basic_block *a, struct basic_block *b, struct value *cond);

// Create a new return instruction
// FIXME: Missing return value.
struct inst_return* inst_new_return(struct ir_context *ctx);

// Create a new phi value.
struct inst_phi* inst_new_phi(struct ir_context *ctx, enum data_type type, size_t valueCount);

// Insert a new value to the phi instruction.

void inst_phi_insertValue(struct inst_phi *phi, struct ir_context *ctx, struct basic_block *block, struct value *value);

// Create a new function.
// FIXME: Missing return value.
struct function* ir_new_function(struct ir_context *context, range_t name);

// Set a use of the instruction.
void inst_setUse(struct ir_context *ctx, struct instruction *inst, size_t useOffset, struct value *value);

// Insert a instruction after the inst.
void inst_insertAfter(struct instruction *inst, struct instruction *add);

// Remove a instruction from the block it is on.
void inst_remove(struct instruction *inst);

// Create a new block
struct basic_block* block_new(struct ir_context *ctx, struct function *fn);

// Insert a instruction at the top.
void block_insertTop(struct basic_block *block, struct instruction *inst);

// Insert a instruction at the end.
void block_insert(struct basic_block *block, struct instruction *inst);

// Get the uses for a instruction.
struct use** inst_getUses(struct instruction *inst, size_t *count);

// Create a new constant value.
struct value_constant* ir_constant_value(struct ir_context *ctx, int64_t value);

/// ---- Iterators ----

// iterator of blocks that can branch to this block.
struct block_predecessor_it {
    struct list_head *list;
    struct list_head *current;
};

struct block_predecessor_it block_predecessor_begin(struct basic_block *block);

int block_predecessor_end(struct block_predecessor_it it);
struct block_predecessor_it block_predecessor_next(struct block_predecessor_it it);
struct basic_block* block_predecessor_get(struct block_predecessor_it it);

// iteartor of blocks that are can be branched from this block. 
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
