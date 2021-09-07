#ifndef IR_H
#define IR_H

#include "buffer.h"
#include "hashmap.h"

#define INST_TYPE(prefix) inst_##prefix##_t

#define IR_VALUE_AS_TYPE(ptr, type) containerof(ptr, type, value)
#define IR_INST_AS_TYPE(ptr, type) containerof(ptr, type, inst)
#define IR_VALUE_AS_INST(ptr, type) containerof(containerof(ptr, instruction_t, value), type, inst)

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

// forward decleration for function.
struct function;
typedef struct function function_t;

// Data types.
enum data_type {
    VOID,
    INT64,
    PTR,
    DT_BLOCK
};

// The IR context, manages all IR objects. 
typedef struct {
    struct list_head functions;
    // The allocator for all IR objects.
    zone_allocator alloc;
    // Special instructions that contain heap objects.
    struct list_head specialInstructions;
} ir_context_t;

// Type of the value. 
enum value_type {
    INST,
    CONST,
    UNKNOWN_CONST, 
    ARGUMENT,
    V_BLOCK
};

// Anything that has a value.
typedef struct {
    enum data_type dataType;
    enum value_type type;

    struct list_head uses; 
    range_t name;
} value_t;

// Argument of a function.
typedef struct {
    // Inherit from value.
    value_t value; 
} value_argument_t;

// A block, can only contain a jump at the end
typedef struct {
    // We consider basic block a value.
    // NOTE: Should this be a pointer too ? 
    value_t value;

    // linked list of instructions.
    struct list_head instructions;
    
    // The function that contains this block.
    function_t *parent;
} basic_block_t;

struct function {
    // Function is a UNKNOWN_CONST PTR
    value_t value;
    basic_block_t *entry;
    struct list_head functions;
    enum data_type returnType;
    size_t argumentCount; 
    size_t valueNameCounter;
};

struct instruction;
typedef struct instruction instruction_t;

// A constant value.
typedef struct {
    value_t value;

    //FIXME: fix this when we add other sizes.
    union {    
        int64_t number;
    };
} value_constant_t;

#define COMMA_SECOND(a, b) a,
enum instruction_type {
    INSTRUCTIONS(COMMA_SECOND)
};
#undef COMMA_SECOND

// This is the edge between a instruction and a value.
// a value can have mulitple users.
typedef struct {
    instruction_t *inst; 
    value_t *value;
    struct list_head useList;
} use_t;

// A general instruction, lives inside a basic block.
struct instruction {
    value_t value;
    struct list_head inst_list; 
    enum instruction_type type;
     
    basic_block_t *parent;
};

// The magic phi instruction, used for the SSA form.
typedef struct {
    instruction_t inst;
     
    dbuffer_t useBuffer; 

    // These values must be kept up to date with the useBuffer
    size_t useCount;

    // always in pairs of block and value.
    use_t **uses;

    struct list_head specialList; 
} inst_phi_t;

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
typedef struct {
    instruction_t inst;
    enum binary_ops op;
    union {
        use_t *uses[2];
        struct {
            use_t *left;
            use_t *right;
        };
    };
} inst_binary_t;

// A load instruction, only valid before SSA conversion.
typedef struct {
    instruction_t inst;
    size_t rId;
    use_t *uses[0];
} inst_load_var_t;

// An assign instruction, only valid before SSA conversion.
typedef struct {
    instruction_t inst;
    size_t rId;
    
    union {
       use_t *uses[1];
       use_t *var;
    };
} inst_assign_var_t;

// A function call instruction.
typedef struct {
    instruction_t inst;
    
    // A function call have variable number of uses,
    // this is needed for passing arguments.
    size_t useCount;
    use_t **uses;
} inst_function_call_t;

// A jump instruction, jumps to a basic block.
typedef struct {
    instruction_t inst;
    use_t *uses[1];
} inst_jump_t;

// A conditional jump instruction, jumps to basic block conditionally.
typedef struct {
    instruction_t inst;
    use_t *uses[3];
} inst_jump_cond_t;

// A return instruction.
typedef struct {
    instruction_t inst;
    size_t hasReturn;
    use_t *uses[1];
} inst_return_t;

struct dominators;
struct domfrontiers;

// Optional stuff that we can print in the IR.
struct ir_print_annotations {
    struct dominators *doms;
    struct domfrontiers *df;
};

// set the name of the value. doesn't take ownership, creates a copy. 
void value_setName(ir_context_t *ctx, value_t *value, range_t name);

// get the name of the value, this will assign a name if needed.
range_t value_getName(ir_context_t *ctx, value_t *value);

// replace all uses of a value with another value.
void value_replaceAllUses(value_t *value, value_t *replacement);

// dump a function.
void function_dump(ir_context_t *ctx, function_t *fun, struct ir_print_annotations *annotations);

// dump a function as a dot file.
void function_dumpDot(ir_context_t *ctx, function_t *fun, struct ir_print_annotations *annotations);

// dump a instruction.
void inst_dump(ir_context_t *ctx, instruction_t *inst);

//void block_dump(ir_context_t *ctx, basic_block_t *block);

// initialize the ir context.
void ir_context_init(ir_context_t *context);

// free the ir context
void ir_context_free(ir_context_t *context);

// Create a var load instruction (only valid before ssa conversion)
inst_load_var_t* inst_new_load_var(ir_context_t *ctx, size_t i, enum data_type type);

// Create a assign instruction (only valid before ssa conversion 
inst_assign_var_t* inst_new_assign_var(ir_context_t *ctx, size_t i, value_t *value); 

// Create a binary op instruction.
inst_binary_t* inst_new_binary(ir_context_t *ctx, enum binary_ops type, value_t *a, value_t *b); 

// Create a jump instruction
inst_jump_t* inst_new_jump(ir_context_t *ctx, basic_block_t *block); 

// Create a conditional jump instruction.
inst_jump_cond_t* inst_new_jump_cond(ir_context_t *ctx, basic_block_t *a, basic_block_t *b, value_t *cond);

// Create a new return instruction
// FIXME: Missing return value.
inst_return_t* inst_new_return(ir_context_t *ctx);

// Create a new phi value.
inst_phi_t* inst_new_phi(ir_context_t *ctx, enum data_type type, size_t valueCount);

// Insert a new value to the phi instruction.

void inst_phi_insertValue(inst_phi_t *phi, ir_context_t *ctx, basic_block_t *block, value_t *value);

// Create a new function.
// FIXME: Missing return value.
function_t* ir_new_function(ir_context_t *context, range_t name);

// Set a use of the instruction.
void inst_setUse(ir_context_t *ctx, instruction_t *inst, size_t useOffset, value_t *value);

// Insert a instruction after the inst.
void inst_insertAfter(instruction_t *inst, instruction_t *add);

// Remove a instruction from the block it is on.
void inst_remove(instruction_t *inst);

// Create a new block
basic_block_t* block_new(ir_context_t *ctx, function_t *fn);

// Insert a instruction at the top.
void block_insertTop(basic_block_t *block, instruction_t *inst);

// Insert a instruction at the end.
void block_insert(basic_block_t *block, instruction_t *inst);

// Get the uses for a instruction.
use_t** inst_getUses(instruction_t *inst, size_t *count);

// Create a new constant value.
value_constant_t* ir_constant_value(ir_context_t *ctx, int64_t value);

/// ---- Iterators ----

// iterator of blocks that can branch to this block.
struct block_predecessor_it {
    struct list_head *list;
    struct list_head *current;
};

struct block_predecessor_it block_predecessor_begin(basic_block_t *block);

int block_predecessor_end(struct block_predecessor_it it);
struct block_predecessor_it block_predecessor_next(struct block_predecessor_it it);
basic_block_t* block_predecessor_get(struct block_predecessor_it it);

// iteartor of blocks that are can be branched from this block. 
struct block_successor_it {
    basic_block_t *next;    
    instruction_t *inst;
    size_t i;
};

int block_successor_end(struct block_successor_it it);
struct block_successor_it block_successor_begin(basic_block_t *block);
struct block_successor_it block_successor_next(struct block_successor_it it);
basic_block_t* block_successor_get(struct block_successor_it it);

#endif
