#include "ir.h"
#include "format.h"
#include "buffer.h"
#include "dot_builder.h"

char *kBinaryOpNames[] = {
    "add",
    "sub",
    "mul",
    "div"
};

char *kDataTypeNames[] = {
    "void",
    "int64",
    "ptr",
    "block"
};

#define STR_SECOND(a, b) #b,
char *kInstNames[] = {
    INSTRUCTIONS(STR_SECOND)
};
#undef STR_SECOND

void ir_context_init(struct ir_context *context) {
    LIST_INIT(&context->functions);
    zone_init(&context->alloc);
}

void ir_context_free(struct ir_context *context) {
   zone_free(&context->alloc);
}

void _value_init(struct value *value, enum value_type type, enum data_type dataType) { 
   value->type = type;
   LIST_INIT(&value->uses); 
   value->dataType = dataType;
}

struct value_constant* ir_constant_value(struct ir_context *ctx, int64_t value) {
    //TODO: Constant uniqeing.
   struct value_constant *result = znnew(&ctx->alloc, struct value_constant);
   _value_init(&result->value, CONST, INT64);
   result->number = value;
   return result; 
}

void inst_setUse(struct ir_context *ctx, struct instruction *inst, size_t useOffset, struct value *value) {
    size_t useCount = 0;
    struct use **uses = inst_getUses(inst, &useCount); 
    
    assert(useOffset < useCount && "invalid use"); 
    //assert(uses[useOffset] != NULL && "There is a use already");

    struct use *use = znnew(&ctx->alloc, struct use);
    use->inst = inst;
    use->value = value;
    
    list_add(&value->uses, &use->useList);
    uses[useOffset] = use;    
}

void inst_replaceUse(struct use **use, struct value *value) {
    //TODO
}

void inst_insertAfter(struct instruction *inst, struct instruction *add) {
    list_addAfter(&inst->inst_list, &add->inst_list); 
}

struct basic_block* block_new(struct ir_context *ctx) {
    struct basic_block* block = znnew(&ctx->alloc, struct basic_block);
    _value_init(&block->value, V_BLOCK, DT_BLOCK);
    LIST_INIT(&block->instructions);
}

// We assign numbers to values that should not be printed inline.
// this is currently only instructions. 
//
// %3 = load_var 1
// add %3, 10 
struct element_name {
    size_t num; // Number assigned to the value. 
    struct hm_bucket_entry entry;
};

struct ir_print_context {
    struct ir_context *irContext;
    size_t valueNameCounter;
};

void block_dump(struct ir_print_context *context, struct basic_block *block, dbuffer_t *dbuffer,
                int dot, struct ir_print_annotations *annotations);


//TODO: Use the value.name
range_t _get_valueName(struct ir_print_context *context, struct value *value) {
    if (value->name.size != 0)
        return value->name;

    size_t num = ++context->valueNameCounter;
    range_t name = format_range("{int}", num);
    value_setName(context->irContext, value, name);
    free(name.ptr);
    return value->name;
}

void value_setName(struct ir_context *ctx, struct value *value, range_t name) {
    char *ptr = zone_alloc(&ctx->alloc, name.size);
    memcpy(ptr, name.ptr, name.size);  
    value->name.size = name.size;
    value->name.ptr = ptr;
}

void function_dump(struct ir_context *ctx, struct function *fun, struct ir_print_annotations *annotations) {
    // Init the print context 
    struct ir_print_context context;
    context.irContext = ctx;
    context.valueNameCounter = 0;

    printf("%s function %.*s\n", kDataTypeNames[fun->returnType],
            fun->value.name.size, fun->value.name.ptr);
    hashset_t visited;
    hashset_init(&visited, ptrKeyType); 

    dbuffer_t toVisit, newVisit;
    dbuffer_init(&toVisit);
    dbuffer_init(&newVisit);
    dbuffer_pushPointer(&toVisit, fun->entry); 
    dbuffer_t content;
    dbuffer_init(&content);
            
    while (toVisit.usage != 0) {
        for (size_t i = 0; i < toVisit.usage / sizeof(void*); i++) {
            struct basic_block *block = ((struct basic_block**)toVisit.buffer)[i];
            // Skip if we already visited this block
            if (hashset_existsPtr(&visited, block))
                continue;
            hashset_insertPtr(&visited, block);        
            
            dbuffer_clear(&content);
            block_dump(&context, block, &content, 0, annotations);
            printf("%.*s\n", content.usage, content.buffer);

            struct block_successor_it it = block_successor_begin(block); 
            for (; !block_successor_end(it); it = block_successor_next(it)) {
                struct basic_block *succ = block_successor_get(it);
                dbuffer_pushPointer(&newVisit, succ); 
            }


        }
        dbuffer_clear(&toVisit);
        dbuffer_swap(&toVisit, &newVisit);
    }
    
    dbuffer_free(&toVisit);
    dbuffer_free(&newVisit);
    dbuffer_free(&content);

    hashset_free(&visited);
}

void function_dumpDot(struct ir_context *ctx, struct function *fun, struct ir_print_annotations *annotations) {
    struct ir_print_context context;
    context.irContext = ctx;
    context.valueNameCounter = 0;

    hashset_t visited;
    hashset_init(&visited, ptrKeyType); 

    dbuffer_t toVisit, newVisit;
    dbuffer_init(&toVisit);
    dbuffer_init(&newVisit);
    dbuffer_pushPointer(&toVisit, fun->entry); 
    dbuffer_t content;
    dbuffer_init(&content);

    struct Graph graph;
    graph_init(&graph, "function_graph", 1);
    graph_setNodeProps(&graph, "node", "shape=rect"); 
    
    char nodeId[MAX_NODE_ID];

    while (toVisit.usage != 0) {
        for (size_t i = 0; i < toVisit.usage / sizeof(void*); i++) {
            struct basic_block *block = ((struct basic_block**)toVisit.buffer)[i];
            // Skip if we already visited this block
            if (hashset_existsPtr(&visited, block))
                continue;
            hashset_insertPtr(&visited, block);        
            dbuffer_clear(&content);
            dbuffer_pushStr(&content, "label=\"");
            //TODO: Proper escape sequance. 
            block_dump(&context, block, &content, 1, annotations);
            getNodeId(block, nodeId);
            dbuffer_pushChar(&content, '\"'); 
            dbuffer_pushChar(&content, 0);

            graph_setNodeProps(&graph, nodeId, content.buffer); 

            struct block_successor_it it = block_successor_begin(block); 
            for (; !block_successor_end(it); it = block_successor_next(it)) {
                struct basic_block *succ = block_successor_get(it);
                char childId[MAX_NODE_ID];
                getNodeId(succ, childId);
                graph_addEdge(&graph, nodeId, childId); 
                dbuffer_pushPointer(&newVisit, succ); 
            }
        }
        dbuffer_clear(&toVisit);
        dbuffer_swap(&toVisit, &newVisit);
    }
    
    dbuffer_free(&toVisit);
    dbuffer_free(&newVisit);
    dbuffer_free(&content);
    
    hashset_free(&visited);

    char *result = graph_finalize(&graph);
    puts(result);
    free(result);
}

// We need to have a hack for proper dot formatting.
#define _B_NEW_LINE dbuffer_pushStr(dbuffer, dot ? "\\l" : "\n") 

void block_dump(struct ir_print_context *context, struct basic_block *block, dbuffer_t *dbuffer,
                int dot, struct ir_print_annotations *annotations) {
    range_t bName = _get_valueName(context, &block->value); 
    
    format_dbuffer("!{range}:", dbuffer, bName);
    _B_NEW_LINE; 
    
    // Print the dominator information, if we have it. 
    if (annotations && annotations->doms) {
        struct basic_block *dominator = dominators_getIDom(annotations->doms, block);
        range_t dominatorName = _get_valueName(context, &dominator->value); 
 
        format_dbuffer("// idom[{range}] = {range}", dbuffer, bName, dominatorName);
        _B_NEW_LINE; 
    }

    // Print the dominance frontiers information, if we have it.
    if (annotations && annotations->df) {
        size_t size = 0;
        struct basic_block **dfList = domfrontiers_get(annotations->df, block, &size);
        format_dbuffer("// df[{range}] = [", dbuffer, bName);
        for (size_t i = 0; i < size; i++) {
            struct basic_block *df = dfList[i];
            range_t dfName = _get_valueName(context, &df->value); 
            format_dbuffer("{range}", dbuffer, dfName);
            if (i != size - 1)
                dbuffer_pushStr(dbuffer, " ,");
        }
        dbuffer_pushChar(dbuffer, ']');
        _B_NEW_LINE; 
    }

    LIST_FOR_EACH(&block->instructions) {
        struct instruction *inst = containerof(c, struct instruction, inst_list);
        
        if (inst->value.dataType != VOID) {
            range_t vName = _get_valueName(context, &inst->value);
            format_dbuffer("%{range} = ", dbuffer, vName);
        }
        format_dbuffer("{str} ", dbuffer, kInstNames[inst->type]);

        //Custom inline paramaters. 
        switch(inst->type)  {
            case INST_LOAD_VAR:
                format_dbuffer("{int}", dbuffer, IR_INST_AS_TYPE(inst, struct inst_load_var)->rId);
                break;
            case INST_ASIGN_VAR:
                format_dbuffer("{int}, ", dbuffer, IR_INST_AS_TYPE(inst, struct inst_asign_var)->rId);
                break;
            case INST_BINARY:
                format_dbuffer("{str} ", dbuffer,
                        kBinaryOpNames[IR_INST_AS_TYPE(inst, struct inst_binary)->op]);
                break;
        }

        // Uses
        size_t count = 0;
        struct use **uses = inst_getUses(inst, &count);
        
        for (int i = 0; i < count; i++) {
            struct value *value = uses[i]->value;
            switch(value->type) {
                case V_BLOCK: 
                case INST: {
                    range_t vName = _get_valueName(context, value);
                    format_dbuffer("%{range}", dbuffer, vName); 
                    break;
                }
                case CONST: {
                    // Just materialize the constant inline.
                    struct value_constant *vConst = containerof(value, struct value_constant, value);
                    format_dbuffer("{int}", dbuffer, vConst->number); 
                    break;
                }
                default:
                    assert(0 && "Unknown value type");
            }
            if (i != count - 1)
               dbuffer_pushStr(dbuffer, ", ");
        }

        _B_NEW_LINE; 
    }
}

void block_insert(struct basic_block *block, struct instruction *add) {
   add->parent = block; 
   list_add(&block->instructions, &add->inst_list);
} 

struct instruction* block_lastInstruction(struct basic_block *block) {
    if (list_empty(&block->instructions))
        return NULL;
    return containerof(block->instructions.prev, struct instruction, inst_list);
}

// Boiler plate stuff
#define GEN_NEW_INST(enu, prefix) INST_TYPE(prefix)*                       \
    _inst_new_##prefix(struct ir_context *ctx, enum value_type type) {     \
    INST_TYPE(prefix) *result = znnew(&ctx->alloc, INST_TYPE(prefix));     \
    _value_init(&result->inst.value, INST, type);                          \
    result->inst.type = enu;                                               \
    return result;                                                         \
}

INSTRUCTIONS(GEN_NEW_INST)
#undef GEN_NEW_INST

// TODO: Consider allowing the creation of functions without a entry block.
// this would be very usefull for ir creation.
struct function* ir_new_function(struct ir_context *ctx, range_t name, struct basic_block *entry) {
    struct function *fun = znnew(&ctx->alloc, struct function);
    fun->entry = entry;

    list_add(&ctx->functions, &fun->functions);
    return fun;
}

struct inst_load_var* inst_new_load_var(struct ir_context *ctx, size_t i, enum data_type type) {
    struct inst_load_var *var = _inst_new_load_var(ctx, type);
    var->rId = i;
    return var; 
}

struct inst_asign_var* inst_new_asign_var(struct ir_context *ctx, size_t i, struct value *value) {
    struct inst_asign_var *var = _inst_new_asign_var(ctx, VOID);
    var->rId = i;
    
    inst_setUse(ctx, &var->inst, 0, value);
    return var;
}

struct inst_binary* inst_new_binary(struct ir_context *ctx, enum binary_ops type, struct value *a, struct value *b) {
    assert(a->dataType == b->dataType && "data type mismatch");
    
    struct inst_binary *bin = _inst_new_binary(ctx, INT64);
    bin->op = type;

    inst_setUse(ctx, &bin->inst, 0, a);
    inst_setUse(ctx, &bin->inst, 1, b);
    return bin;
}

struct inst_jump* inst_new_jump(struct ir_context *ctx, struct basic_block *block) {
    struct inst_jump *jump = _inst_new_jump(ctx, VOID);
    inst_setUse(ctx, &jump->inst, 0, &block->value);
    return jump;
}

struct inst_jump_cond* inst_new_jump_cond(struct ir_context *ctx, struct basic_block *a, struct basic_block *b, struct value *cond) {
    struct inst_jump_cond *jump = _inst_new_jump_cond(ctx, VOID);
   
    inst_setUse(ctx, &jump->inst, 0, cond);
    inst_setUse(ctx, &jump->inst, 1, &a->value);
    inst_setUse(ctx, &jump->inst, 2, &b->value);
 
    return  jump;
}

struct inst_return* inst_new_return(struct ir_context *ctx) {
    return _inst_new_return(ctx, VOID);
}

// Instruction that use a constant number of values.
#define INST_CONSTANT_USE(o)               \
    o(INST_LOAD_VAR, load_var, 0)          \
    o(INST_ASIGN_VAR, asign_var, 1)        \
    o(INST_BINARY, binary, 2)              \
    o(INST_JUMP, jump, 1)                  \
    o(INST_JUMP_COND, jump_cond, 3)

#define INST_VARIABLE_USE(o)            \
    o(INST_PHI, phi)                    \
    o(INST_FUNCTION_CALL, function_call)

//Note: Maybe the struct instruction itself can hold the pointer to the uses. 

#define GEN_INST_CONSTANT_USE(enu, prefix, c)                                \
    case enu: {                                                              \
    INST_TYPE(prefix) *instType = IR_INST_AS_TYPE(inst, INST_TYPE(prefix));  \
    *count = c;                                                              \
    return instType->uses;                                                   \
} 

#define GEN_INST_VARIABLE_USE(enu, prefix)                                 \
case enu: {                                                                \
    INST_TYPE(prefix) *instType = IR_INST_AS_TYPE(inst, INST_TYPE(prefix));\
    *count = instType->useCount;                                           \
    return instType->uses;                                                 \
} 
/*
void ir_addFunction(struct ir_context *context, struct function *function) {
    hashmap_setRange(&context->functionNames, function->name);
    list_add(&context->functions, &function->functions, );
} 
*/

struct use** inst_getUses(struct instruction *inst, size_t *count) {
    switch(inst->type) {
        INST_CONSTANT_USE(GEN_INST_CONSTANT_USE)
        INST_VARIABLE_USE(GEN_INST_VARIABLE_USE)
    }
}

// ---- Iterators ---- 

struct block_predecessor_it block_predecessor_begin(struct basic_block *block) {
    return (struct block_predecessor_it) {.list = &block->value.uses, .current = block->value.uses.next}; 
}

int block_predecessor_end(struct block_predecessor_it it) {
    return it.list == it.current;
}

struct block_predecessor_it block_predecessor_next(struct block_predecessor_it it) {
    assert(!block_predecessor_end(it) && "invalid iterator");
    it.current = it.current->next;
    return it;
}

struct basic_block* block_predecessor_get(struct block_predecessor_it it) {
    assert(!block_predecessor_end(it) && "invalid iterator");
    struct use *u = containerof(it.current, struct use, useList);
    return u->inst->parent;
}

void _block_successor_read(struct block_successor_it *it) {
    size_t count = 0;
    struct use **uses = inst_getUses(it->inst, &count);
    
    it->next = NULL;
    for (; it->i < count; it->i++) {
        struct value *value = uses[it->i]->value;
        if (value->type == V_BLOCK) {
            it->next = containerof(value, struct basic_block, value);
            it->i++;
            return;
        }    
    }
}

struct block_successor_it block_successor_begin(struct basic_block *block) {
    struct instruction *inst = block_lastInstruction(block);
    struct block_successor_it it;
    it.i = 0;
    it.inst = inst;
    _block_successor_read(&it); 
    return it;
}

int block_successor_end(struct block_successor_it it) {
    return it.next == NULL;
}

struct block_successor_it block_successor_next(struct block_successor_it it) {
    assert(!block_successor_end(it) && "invalid iterator");
     _block_successor_read(&it); 
    return it;
}

struct basic_block* block_successor_get(struct block_successor_it it) {
    assert(!block_successor_end(it) && "invalid iterator");
    return it.next; 
}
