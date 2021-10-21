#include "ir.h"
#include "buffer.h"
#include "dominators.h"
#include "dot_builder.h"
#include "format.h"

// NOTE: we should probably move this to a macro.
char *kBinaryOpNames[] = {"add",  "sub",     "mul",     "div",       "equals",
                          "less", "greater", "less_eq", "greater_eq"};

char *kDataTypeNames[] = {"void", "int64", "ptr", "block"};

#define STR_SECOND(a, b) #b,
char *kInstNames[] = {INSTRUCTIONS(STR_SECOND)};
#undef STR_SECOND

void ir_context_init(ir_context_t *context) {
    LIST_INIT(&context->functions);
    LIST_INIT(&context->specialInstructions);
    zone_init(&context->alloc);
}

void ir_context_free(ir_context_t *context) { zone_free(&context->alloc); }

void _value_init(value_t *value, enum value_type type,
                 enum data_type dataType) {
    value->type = type;
    LIST_INIT(&value->uses);
    value->dataType = dataType;
}

value_constant_t *ir_constant_value(ir_context_t *ctx, int64_t value) {
    // TODO: Constant uniqeing.
    value_constant_t *result = znnew(&ctx->alloc, value_constant_t);
    _value_init(&result->value, CONST, INT64);
    result->number = value;
    return result;
}

void inst_setUse(ir_context_t *ctx, instruction_t *inst, size_t useOffset,
                 value_t *value) {
    size_t useCount = 0;
    use_t **uses = inst_getUses(inst, &useCount);

    assert(useOffset < useCount && "invalid use");
    use_t *use = uses[useOffset];
    // NOTE: Maybe not store uses as a
    if (use == NULL)
        use = znnew(&ctx->alloc, use_t);
    else
        // if there is a use already, remove it from the value list.
        list_deattach(&use->useList);

    use->inst = inst;
    use->value = value;

    list_add(&value->uses, &use->useList);
    uses[useOffset] = use;
}

void inst_insertAfter(instruction_t *inst, instruction_t *add) {
    list_addAfter(&inst->inst_list, &add->inst_list);
}

void inst_remove(instruction_t *inst) { list_deattach(&inst->inst_list); }

basic_block_t *block_new(ir_context_t *ctx, function_t *fn) {
    basic_block_t *block = znnew(&ctx->alloc, basic_block_t);
    block->parent = fn;
    _value_init(&block->value, V_BLOCK, DT_BLOCK);
    LIST_INIT(&block->instructions);
}

void _block_dump(ir_context_t *ctx, basic_block_t *block, dbuffer_t *dbuffer,
                 int dot, struct ir_print_annotations *annotations);

function_t *value_getFunction(value_t *value) {
    if (value->type == V_BLOCK) {
        basic_block_t *block = containerof(value, basic_block_t, value);
        return block->parent;
    } else if (value->type == INST) {
        instruction_t *inst = containerof(value, instruction_t, value);
        return inst->parent->parent;
    }
    // FIXME: Support arguments.
    return NULL;
}

range_t value_getName(ir_context_t *ctx, value_t *value) {
    if (value->name.size != 0)
        return value->name;
    // Generate a name for this value.
    function_t *func = value_getFunction(value);

    size_t num = func->valueNameCounter++;
    range_t name = format_range("{int}", num);
    value_setName(ctx, value, name);
    free(name.ptr);
    return value->name;
}

void value_setName(ir_context_t *ctx, value_t *value, range_t name) {
    char *ptr = zone_alloc(&ctx->alloc, name.size);
    memcpy(ptr, name.ptr, name.size);
    value->name.size = name.size;
    value->name.ptr = ptr;
}

void value_replaceAllUses(value_t *value, value_t *replacement) {
    for (struct list_head *current = value->uses.next, *next;
         current != &value->uses; current = next) {
        use_t *use = containerof(current, use_t, useList);
        next = use->useList.next;

        // remove the use from the old value's use list.
        list_deattach(&use->useList);
        list_add(&replacement->uses, &use->useList);
        use->value = replacement;
    };
}

void function_dump(ir_context_t *ctx, function_t *fun,
                   struct ir_print_annotations *annotations) {
    printf("%s function %.*s\n", kDataTypeNames[fun->returnType],
           fun->value.name.size, fun->value.name.ptr);
    hashset_t visited;
    hashset_init(&visited, ptrKeyType);

    dbuffer_t content;
    dbuffer_init(&content);

    dbuffer_t toVisit, newVisit;
    dbuffer_init(&toVisit);
    dbuffer_init(&newVisit);
    dbuffer_pushPtr(&toVisit, fun->entry);

    while (toVisit.usage != 0) {
        for (size_t i = 0; i < toVisit.usage / sizeof(void *); i++) {
            basic_block_t *block = ((basic_block_t **)toVisit.buffer)[i];
            // Skip if we already visited this block
            if (hashset_existsPtr(&visited, block))
                continue;
            hashset_insertPtr(&visited, block);

            dbuffer_clear(&content);
            _block_dump(ctx, block, &content, 0, annotations);
            printf("%.*s\n", content.usage, content.buffer);

            struct block_successor_it it = block_successor_begin(block);
            for (; !block_successor_end(it); it = block_successor_next(it)) {
                basic_block_t *succ = block_successor_get(it);
                dbuffer_pushPtr(&newVisit, succ);
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

void function_dumpDot(ir_context_t *ctx, function_t *fun,
                      struct ir_print_annotations *annotations) {
    hashset_t visited;
    hashset_init(&visited, ptrKeyType);

    dbuffer_t toVisit, newVisit;
    dbuffer_init(&toVisit);
    dbuffer_init(&newVisit);
    dbuffer_pushPtr(&toVisit, fun->entry);
    dbuffer_t content;
    dbuffer_init(&content);

    struct Graph graph;
    graph_init(&graph, "function_graph", 1);
    graph_setNodeProps(&graph, "node", "shape=rect");

    char nodeId[MAX_NODE_ID];

    while (toVisit.usage != 0) {
        for (size_t i = 0; i < toVisit.usage / sizeof(void *); i++) {
            basic_block_t *block = ((basic_block_t **)toVisit.buffer)[i];
            // Skip if we already visited this block
            if (hashset_existsPtr(&visited, block))
                continue;
            hashset_insertPtr(&visited, block);
            dbuffer_clear(&content);
            dbuffer_pushStr(&content, "label=\"");
            // TODO: Proper escape sequance.
            _block_dump(ctx, block, &content, 1, annotations);
            getNodeId(block, nodeId);
            dbuffer_pushChar(&content, '\"');
            dbuffer_pushChar(&content, 0);

            graph_setNodeProps(&graph, nodeId, content.buffer);

            struct block_successor_it it = block_successor_begin(block);
            for (; !block_successor_end(it); it = block_successor_next(it)) {
                basic_block_t *succ = block_successor_get(it);
                char childId[MAX_NODE_ID];
                getNodeId(succ, childId);
                graph_addEdge(&graph, nodeId, childId);
                dbuffer_pushPtr(&newVisit, succ);
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

void inst_dumpd(ir_context_t *ctx, instruction_t *inst, dbuffer_t *dbuffer,
                size_t dot);

void inst_dump(ir_context_t *ctx, instruction_t *inst) {
    dbuffer_t dbuffer;
    dbuffer_init(&dbuffer);
    inst_dumpd(ctx, inst, &dbuffer, 0);
    dbuffer_pushChar(&dbuffer, 0);
    puts(dbuffer.buffer);
    dbuffer_free(&dbuffer);
}

// We need to have a hack for proper dot formatting.
#define _B_NEW_LINE dbuffer_pushStr(dbuffer, dot ? "\\l" : "\n")

void _block_dump(ir_context_t *ctx, basic_block_t *block, dbuffer_t *dbuffer,
                 int dot, struct ir_print_annotations *annotations) {
    range_t bName = value_getName(ctx, &block->value);

    format_dbuffer("!{range}:", dbuffer, bName);
    _B_NEW_LINE;

    // Print the dominator information, if we have it.
    if (annotations && annotations->doms) {
        basic_block_t *dominator = dominators_getIDom(annotations->doms, block);
        range_t dominatorName = value_getName(ctx, &dominator->value);

        format_dbuffer("// idom[{range}] = {range}", dbuffer, bName,
                       dominatorName);
        _B_NEW_LINE;
    }

    // Print the dominance frontiers information, if we have it.
    if (annotations && annotations->df) {
        size_t size = 0;
        basic_block_t **dfList =
            domfrontiers_get(annotations->df, block, &size);
        format_dbuffer("// df[{range}] = [", dbuffer, bName);
        for (size_t i = 0; i < size; i++) {
            basic_block_t *df = dfList[i];
            range_t dfName = value_getName(ctx, &df->value);
            format_dbuffer("{range}", dbuffer, dfName);
            if (i != size - 1)
                dbuffer_pushStr(dbuffer, " ,");
        }
        dbuffer_pushChar(dbuffer, ']');
        _B_NEW_LINE;
    }

    LIST_FOR_EACH(&block->instructions) {
        instruction_t *inst = containerof(c, instruction_t, inst_list);
        inst_dumpd(ctx, inst, dbuffer, dot);
    }
}

void block_dump(ir_context_t *ctx, basic_block_t *block) {
    dbuffer_t dbuffer;
    dbuffer_init(&dbuffer);
    _block_dump(ctx, block, &dbuffer, 0, NULL);
    dbuffer_pushChar(&dbuffer, 0);
    puts(dbuffer.buffer);
    dbuffer_free(&dbuffer);
}

void inst_dumpd(ir_context_t *ctx, instruction_t *inst, dbuffer_t *dbuffer,
                size_t dot) {
    if (inst->value.dataType != VOID) {
        range_t vName = value_getName(ctx, &inst->value);
        format_dbuffer("%{range} = ", dbuffer, vName);
    }
    format_dbuffer("{str} ", dbuffer, kInstNames[inst->type]);

    // Custom inline paramaters.
    switch (inst->type) {
    case INST_LOAD_VAR:
        format_dbuffer("{int}", dbuffer,
                       IR_INST_AS_TYPE(inst, inst_load_var_t)->rId);
        break;
    case INST_ASSIGN_VAR:
        format_dbuffer("{int}, ", dbuffer,
                       IR_INST_AS_TYPE(inst, inst_assign_var_t)->rId);
        break;
    case INST_BINARY:
        format_dbuffer(
            "{str} ", dbuffer,
            kBinaryOpNames[IR_INST_AS_TYPE(inst, inst_binary_t)->op]);
        break;
    }

    // Uses
    size_t count = 0;
    use_t **uses = inst_getUses(inst, &count);

    for (int i = 0; i < count; i++) {
        value_t *value = uses[i]->value;
        switch (value->type) {
        case V_BLOCK:
        case INST: {
            range_t vName = value_getName(ctx, value);
            format_dbuffer("%{range}", dbuffer, vName);
            break;
        }
        case CONST: {
            // Just materialize the constant inline.
            value_constant_t *vConst =
                containerof(value, value_constant_t, value);
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

// Insert a instruction at the top.
void block_insertTop(basic_block_t *block, instruction_t *inst) {
    list_addAfter(&block->instructions, &inst->inst_list);
    inst->parent = block;
}

void block_insert(basic_block_t *block, instruction_t *add) {
    add->parent = block;
    list_add(&block->instructions, &add->inst_list);
}

void block_numberInst(basic_block_t *block) {
    size_t i = 0;
    LIST_FOR_EACH(&block->instructions) {
        instruction_t *inst = containerof(c, instruction_t, inst_list);
        inst->i = i++;
    }
}

instruction_t *block_lastInstruction(basic_block_t *block) {
    if (list_empty(&block->instructions))
        return NULL;
    return containerof(block->instructions.prev, instruction_t, inst_list);
}

// Boiler plate stuff
#define GEN_NEW_INST(enu, prefix)                                              \
    INST_TYPE(prefix) *                                                        \
        _inst_new_##prefix(ir_context_t *ctx, enum value_type type) {          \
        INST_TYPE(prefix) *result = znnew(&ctx->alloc, INST_TYPE(prefix));     \
        *result = (INST_TYPE(prefix)){};                                       \
        _value_init(&result->inst.value, INST, type);                          \
        result->inst.type = enu;                                               \
        return result;                                                         \
    }

INSTRUCTIONS(GEN_NEW_INST)
#undef GEN_NEW_INST

// TODO: Consider allowing the creation of functions without a entry block.
// this would be very usefull for ir creation.
function_t *ir_new_function(ir_context_t *ctx, range_t name) {
    function_t *fun = znnew(&ctx->alloc, function_t);
    fun->valueNameCounter = 0;

    list_add(&ctx->functions, &fun->functions);
    return fun;
}

inst_load_var_t *inst_new_load_var(ir_context_t *ctx, size_t i,
                                   enum data_type type) {
    inst_load_var_t *var = _inst_new_load_var(ctx, type);
    var->rId = i;
    return var;
}

inst_assign_var_t *inst_new_assign_var(ir_context_t *ctx, size_t i,
                                       value_t *value) {
    inst_assign_var_t *var = _inst_new_assign_var(ctx, VOID);
    var->rId = i;

    inst_setUse(ctx, &var->inst, 0, value);
    return var;
}

inst_binary_t *inst_new_binary(ir_context_t *ctx, enum binary_ops type,
                               value_t *a, value_t *b) {
    assert(a->dataType == b->dataType && "data type mismatch");

    inst_binary_t *bin = _inst_new_binary(ctx, a->dataType);
    bin->op = type;

    inst_setUse(ctx, &bin->inst, 0, a);
    inst_setUse(ctx, &bin->inst, 1, b);
    return bin;
}

inst_jump_t *inst_new_jump(ir_context_t *ctx, basic_block_t *block) {
    inst_jump_t *jump = _inst_new_jump(ctx, VOID);
    inst_setUse(ctx, &jump->inst, 0, &block->value);
    return jump;
}

inst_jump_cond_t *inst_new_jump_cond(ir_context_t *ctx, basic_block_t *a,
                                     basic_block_t *b, value_t *cond) {
    inst_jump_cond_t *jump = _inst_new_jump_cond(ctx, VOID);
    inst_setUse(ctx, &jump->inst, 0, &a->value);
    inst_setUse(ctx, &jump->inst, 1, &b->value);
    inst_setUse(ctx, &jump->inst, 2, cond);

    return jump;
}

inst_return_t *inst_new_return(ir_context_t *ctx) {
    return _inst_new_return(ctx, VOID);
}

inst_phi_t *inst_new_phi(ir_context_t *ctx, enum data_type type,
                         size_t valueCount) {
    inst_phi_t *phi = _inst_new_phi(ctx, type);

    // We store block and value.
    size_t useReserve = max(valueCount, 8) * 2;
    dbuffer_initSize(&phi->useBuffer, sizeof(void *) * useReserve);
    phi->useCount = valueCount * 2;
    phi->uses = (use_t **)&phi->useBuffer.buffer;

    list_add(&ctx->specialInstructions, &phi->specialList);

    return phi;
}

void inst_phi_insertValue(inst_phi_t *phi, ir_context_t *ctx,
                          basic_block_t *block, value_t *value) {
    // iterate over the uses to make sure we don't have already have the value.
    for (size_t i = 0; i < phi->useCount; i += 2) {
        if (phi->uses[i]->value == &block->value &&
            phi->uses[i + 1]->value == value)
            return;
    }

    // I am not sure if this should be allocated from the ir context allocator.
    dbuffer_pushPtr(&phi->useBuffer, NULL);
    dbuffer_pushPtr(&phi->useBuffer, NULL);

    phi->uses = (use_t **)phi->useBuffer.buffer;
    phi->useCount += 2;

    // set the uses for the values.
    inst_setUse(ctx, &phi->inst, phi->useCount - 2, &block->value);
    inst_setUse(ctx, &phi->inst, phi->useCount - 1, value);
}

// Instruction that use a constant number of values.
#define INST_CONSTANT_USE(o)                                                   \
    o(INST_LOAD_VAR, load_var, 0) o(INST_ASSIGN_VAR, assign_var, 1)            \
        o(INST_BINARY, binary, 2) o(INST_JUMP, jump, 1)                        \
            o(INST_JUMP_COND, jump_cond, 3)

#define INST_VARIABLE_USE(o)                                                   \
    o(INST_PHI, phi) o(INST_FUNCTION_CALL, function_call)

// Note: Maybe the instruction_t itself can hold the pointer to the uses.

#define GEN_INST_CONSTANT_USE(enu, prefix, c)                                  \
    case enu: {                                                                \
        INST_TYPE(prefix) *instType =                                          \
            IR_INST_AS_TYPE(inst, INST_TYPE(prefix));                          \
        *count = c;                                                            \
        return instType->uses;                                                 \
    }

#define GEN_INST_VARIABLE_USE(enu, prefix)                                     \
    case enu: {                                                                \
        INST_TYPE(prefix) *instType =                                          \
            IR_INST_AS_TYPE(inst, INST_TYPE(prefix));                          \
        *count = instType->useCount;                                           \
        return instType->uses;                                                 \
    }

/*
void ir_addFunction(ir_context_t *context, function_t *function) {
    hashmap_setRange(&context->functionNames, function->name);
    list_add(&context->functions, &function->functions, );
}
*/

use_t **inst_getUses(instruction_t *inst, size_t *count) {
    switch (inst->type) {
        INST_CONSTANT_USE(GEN_INST_CONSTANT_USE)
        INST_VARIABLE_USE(GEN_INST_VARIABLE_USE)
    }
}

// ---- Iterators ----

struct block_predecessor_it block_predecessor_begin(basic_block_t *block) {
    return (struct block_predecessor_it){.list = &block->value.uses,
                                         .current = block->value.uses.next};
}

int block_predecessor_end(struct block_predecessor_it it) {
    return it.list == it.current;
}

struct block_predecessor_it
block_predecessor_next(struct block_predecessor_it it) {
    assert(!block_predecessor_end(it) && "invalid iterator");
    it.current = it.current->next;
    return it;
}

basic_block_t *block_predecessor_get(struct block_predecessor_it it) {
    assert(!block_predecessor_end(it) && "invalid iterator");
    use_t *u = containerof(it.current, use_t, useList);
    return u->inst->parent;
}

void _block_successor_read(struct block_successor_it *it) {
    if (!it->inst)
        return;
    size_t count = 0;
    use_t **uses = inst_getUses(it->inst, &count);

    it->next = NULL;
    for (; it->i < count; it->i++) {
        value_t *value = uses[it->i]->value;
        if (value->type == V_BLOCK) {
            it->next = containerof(value, basic_block_t, value);
            it->i++;
            return;
        }
    }
}

struct block_successor_it block_successor_begin(basic_block_t *block) {
    instruction_t *inst = block_lastInstruction(block);

    // Tecnically, blocks should end either with a return or with a jump.
    // Gracefully handle if the last instructions is a phi.
    // If we ever add more instructions that
    // reference blocks, they should be included here.
    if (inst && inst->type == INST_PHI)
        inst = NULL;

    struct block_successor_it it = (struct block_successor_it){};
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

basic_block_t *block_successor_get(struct block_successor_it it) {
    assert(!block_successor_end(it) && "invalid iterator");
    return it.next;
}

void pre_visit(basic_block_t *block, dbuffer_t *dbuffer, hashset_t *hashset) {
    if (hashset_existsPtr(hashset, block))
        return;
    hashset_insertPtr(hashset, block);

    struct block_successor_it it = block_successor_begin(block);
    for (; !block_successor_end(it); it = block_successor_next(it)) {
        pre_visit(block_successor_get(it), dbuffer, hashset);
    }

    dbuffer_pushPtr(dbuffer, block);
}

basic_block_t** block_computePostorder(function_t *fn) {
    hashset_t hashset; // set of pointers.
    hashset_init(&hashset, ptrKeyType);
    
    dbuffer_t postorder;
    dbuffer_init(&postorder);

    pre_visit(fn->entry, &postorder, &hashset);

    // It is safe to return dbuffers this way.
    return postorder.buffer;   
}

