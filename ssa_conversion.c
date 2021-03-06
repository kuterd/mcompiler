#include "ssa_conversion.h"
#include "dominators.h"
#include "ir.h"

#include <assert.h>

/// ---- SSA Conversion ----

// Step 1: Analyze blocks
//      Go over instructions.

// Step 2: Insert phis, dominance frontiers.
//      A block can assign multiple times to a reg.
//      We only care about the last one.
//      place the value at dominance frontiers
//      When we insert a new phi it is a new assignment.
//      so we need a worklist algorithm.
// Step 3: "Rename" values. Push them to Phi instructions.

// Information about a register.
struct reg_info {
    size_t rId;
    // the last assignment to this reg from every block.
    dbuffer_t assigns;

    // we will use this in the renaming stage.
    dbuffer_t valueStack;

    struct hm_bucket_entry bucket;
};

// Information about a phi with respect to the register it is creaated for.
struct phi_info {
    size_t rId;
    inst_phi_t *phiInst;

    struct hm_bucket_entry bucket;
};

// Information about a block
struct block_info {
    // int -> reg_block_info
    hashmap_t regMap;
    // array of reg_block_info*
    dbuffer_t regList;
    // int -> phi_info
    hashmap_t phiMap;
    // list of phis.
    dbuffer_t phiList;

    // list of load/assign instructions.
    dbuffer_t loadAssigns;

    // is this block have already been processed by the renaming step.
    int isRenamed;
};

// Information about a reg within the context of a block.
struct reg_block_info {
    // Last assignment to this variable with in this block, we will propagate
    // this value.
    inst_assign_var_t *lastAssign;

    size_t rId;
    struct hm_bucket_entry bucket;
};

struct phi_info *block_info_getOrCreatePhi(struct block_info *bInfo,
                                           basic_block_t *block,
                                           ir_context_t *ctx,
                                           zone_allocator *alloc, size_t rId,
                                           enum data_type type) {
    struct hm_bucket_entry *entry = hashmap_getInt(&bInfo->phiMap, rId);
    if (entry)
        return containerof(entry, struct phi_info, bucket);
    struct phi_info *phiInfo = znnew(alloc, struct phi_info);
    phiInfo->rId = rId;
    hashmap_setInt(&bInfo->phiMap, rId, &phiInfo->bucket);
    dbuffer_pushPtr(&bInfo->phiList, phiInfo);
    // Create the phi IR object.
    phiInfo->phiInst = inst_new_phi(ctx, type, 0);
    block_insertTop(block, &phiInfo->phiInst->inst);
    return phiInfo;
}

struct reg_block_info *_getOrCreateRegInfo(struct block_info *blockInfo,
                                           dbuffer_t *regList,
                                           zone_allocator *zone, size_t rId) {
    struct hm_bucket_entry *bucket = hashmap_getInt(&blockInfo->regMap, rId);
    if (bucket)
        return containerof(bucket, struct reg_block_info, bucket);

    struct reg_block_info *regInfo = znnew(zone, struct reg_block_info);
    regInfo->lastAssign = NULL;
    regInfo->rId = rId;

    hashmap_setInt(&blockInfo->regMap, rId, &regInfo->bucket);
    dbuffer_pushPtr(regList, regInfo);
    return regInfo;
}

// Get the last value that was assigned to this reg.
value_t *_getLastValueUnsafe(hashmap_t *variableMap, size_t vId) {
    struct hm_bucket_entry *entry = hashmap_getInt(variableMap, vId);
    if (!entry)
        return NULL;

    struct reg_info *rInfo = containerof(entry, struct reg_info, bucket);
    if (rInfo->valueStack.usage == 0)
        return NULL;
    return (value_t *)dbuffer_getLastPtr(&rInfo->valueStack);
}

value_t *_getLastValue(hashmap_t *variableMap, size_t vId) {
    value_t *result = _getLastValueUnsafe(variableMap, vId);
    assert(result && "Value not found, maybe a load before store ?");
    return result;
}

// push a value to the reg stack.
void _pushValue(hashmap_t *variableMap, size_t vId, value_t *value) {
    struct hm_bucket_entry *entry = hashmap_getInt(variableMap, vId);
    assert(entry != NULL && "Invalid push");

    struct reg_info *rbInfo = containerof(entry, struct reg_info, bucket);
    dbuffer_pushPtr(&rbInfo->valueStack, value);

    // TODO: Assign a user readable name to the variable, if possible.
}

// pop a value from the regs value stack.
void _popValue(hashmap_t *variableMap, size_t vId) {
    struct hm_bucket_entry *entry = hashmap_getInt(variableMap, vId);
    assert(entry != NULL && "Invalid vId");

    struct reg_info *rInfo = containerof(entry, struct reg_info, bucket);
    assert(rInfo->valueStack.usage != 0 && "Invalid stack state");

    dbuffer_popPtr(&rInfo->valueStack);
}

// Reanme variables, this is the final step of ssa conversion.
// We visit blocks in DFS, push assignments to the stack.
// we have to cleanup the stack before returning to the predecessor.
void ssa_rename(ir_context_t *ctx, struct block_info *bInfoArray,
                hashmap_t *variableMap, struct dominators *doms,
                basic_block_t *current) {
    // get the block number based on post order number.
    size_t number = dominators_getNumber(doms, current);
    struct block_info *bInfo = &bInfoArray[number];
    // if we already did renaming on this block, return early.
    if (bInfo->isRenamed)
        return;
    bInfo->isRenamed = 1;

    // Push phi instructions to the top of their stacks.
    size_t phiCount;
    struct phi_info **phiArray =
        (struct phi_info **)dbuffer_asPtrArray(&bInfo->phiList, &phiCount);

    for (size_t i = 0; i < phiCount; i++) {
        struct phi_info *phi = phiArray[i];
        _pushValue(variableMap, phi->rId, &phi->phiInst->inst.value);
    }

    // Iterate over loads and stores inside this block.
    size_t instCount;
    instruction_t **loadAssignArray =
        (instruction_t **)dbuffer_asPtrArray(&bInfo->loadAssigns, &instCount);

    // Renaming within the block.
    for (size_t i = 0; i < instCount; i++) {
        instruction_t *inst = loadAssignArray[i];
        if (inst->type == INST_LOAD_VAR) {
            inst_load_var_t *load = containerof(inst, inst_load_var_t, inst);
            value_t *lastValue = _getLastValue(variableMap, load->rId);
            value_replaceAllUses(&load->inst.value, lastValue);
            inst_remove(&load->inst);
        } else if (inst->type == INST_ASSIGN_VAR) {
            inst_assign_var_t *assign =
                containerof(inst, inst_assign_var_t, inst);
            _pushValue(variableMap, assign->rId, assign->var->value);
            inst_remove(&assign->inst);
        } else {
            assert(0 && "The instruction must be load or a assign");
        }
    }

    // Push values to phi instructions of successor blocks.
    for (struct block_successor_it it = block_successor_begin(current);
         !block_successor_end(it); it = block_successor_next(it)) {
        basic_block_t *sblock = block_successor_get(it);
        size_t bNumber = dominators_getNumber(doms, sblock);
        struct block_info *SbInfo = &bInfoArray[bNumber];

        size_t phiCount = 0;
        struct phi_info **phiArray =
            (struct phi_info **)dbuffer_asPtrArray(&SbInfo->phiList, &phiCount);

        // Iterate over phi instructions of this block.
        for (size_t i = 0; i < phiCount; i++) {
            struct phi_info *phiInfo = phiArray[i];
            value_t *lastValue = _getLastValueUnsafe(variableMap, phiInfo->rId);
            if (!lastValue)
                continue;
            inst_phi_insertValue(phiInfo->phiInst, ctx, current, lastValue);
        }
    }

    // Visit dominated blocks.
    for (struct dominator_child_it it = dominator_child_begin(doms, current);
         !dominator_child_end(it); it = dominator_child_next(it)) {
        basic_block_t *block = dominator_child_get(it);
        // Rename dominated block.
        ssa_rename(ctx, bInfoArray, variableMap, doms, block);
    }

    // Before returning to the caller, we should pop the values that we assigned
    // from the stack. We should remove same number of values we pushed from the
    // stack for each reg. Order doesn't matter ofc.
    for (size_t i = 0; i < instCount; i++) {
        instruction_t *inst = loadAssignArray[i];
        if (inst->type == INST_ASSIGN_VAR) {
            inst_assign_var_t *assign =
                containerof(inst, inst_assign_var_t, inst);
            _popValue(variableMap, assign->rId);
        }
    }

    // remove the phi values too.
    for (size_t i = 0; i < phiCount; i++) {
        struct phi_info *phiInfo = phiArray[i];
        _popValue(variableMap, phiInfo->rId);
    }
}

void ssa_convert(ir_context_t *ctx, function_t *fun, struct dominators *doms,
                 struct domfrontiers *df) {
    struct block_info *blockInfo =
        dmalloc(doms->elementCount * sizeof(struct block_info));

    // We will use this to allocate our temporary stuff.
    zone_allocator zone;
    zone_init(&zone);

    hashmap_t variableMap;
    hashmap_init(&variableMap, intKeyType);

    dbuffer_t variableList;
    dbuffer_init(&variableList);

    //  ---- Step 1 : init block info and analyze the blocks. ----
    for (size_t i = 0; i < doms->elementCount; i++) {
        // ---  Initialize block info. ---
        basic_block_t *block = doms->postorder[i];
        struct block_info *bInfo = &blockInfo[i];
        bInfo->isRenamed = 0;
        hashmap_init(&bInfo->regMap, intKeyType);
        dbuffer_init(&bInfo->regList);

        hashmap_init(&bInfo->phiMap, intKeyType);
        dbuffer_init(&bInfo->phiList);

        dbuffer_init(&bInfo->loadAssigns);
        // -------------------------------

        // Iterate over instructions inside the block.
        // we will process loads and assigns.
        LIST_FOR_EACH(&block->instructions) {
            instruction_t *inst = containerof(c, instruction_t, inst_list);
            if (inst->type == INST_LOAD_VAR) {
                inst_load_var_t *lVar =
                    containerof(inst, inst_load_var_t, inst);
                struct reg_block_info *rInfo = _getOrCreateRegInfo(
                    bInfo, &bInfo->regList, &zone, lVar->rId);
                dbuffer_pushPtr(&bInfo->loadAssigns, inst);
            } else if (inst->type == INST_ASSIGN_VAR) {
                inst_assign_var_t *aVar =
                    containerof(inst, inst_assign_var_t, inst);
                struct reg_block_info *rInfo = _getOrCreateRegInfo(
                    bInfo, &bInfo->regList, &zone, aVar->rId);
                rInfo->lastAssign = aVar;
                dbuffer_pushPtr(&bInfo->loadAssigns, inst);
            }
        }

        // Process the register operations in this block.
        // Save the last assignment to a register in this block.
        // We only want to consider the last assignment when we are processing
        // for inserting phi instructions.
        size_t regCount;
        struct reg_block_info **regBlockInfos =
            (struct reg_block_info **)dbuffer_asPtrArray(&bInfo->regList,
                                                         &regCount);

        for (size_t i = 0; i < regCount; i++) {
            struct reg_block_info *rBlockInfo = regBlockInfos[i];

            struct reg_info *regInfo;
            struct hm_bucket_entry *bucket =
                hashmap_getInt(&variableMap, rBlockInfo->rId);
            if (!bucket) {
                // function reg info does not exist for this reg, create it.
                // --- Initialize the reg_block_info ---
                regInfo = znnew(&zone, struct reg_info);
                regInfo->rId = rBlockInfo->rId;
                dbuffer_init(&regInfo->valueStack);
                dbuffer_init(&regInfo->assigns);
                hashmap_setInt(&variableMap, rBlockInfo->rId, &regInfo->bucket);
                // Add the newly created reg info to the list of variables.
                dbuffer_pushPtr(&variableList, regInfo);
            } else {
                regInfo = containerof(bucket, struct reg_info, bucket);
            }
            if (rBlockInfo->lastAssign)
                dbuffer_pushPtr(&regInfo->assigns, rBlockInfo->lastAssign);
        }
    }

    size_t variableCount;
    struct reg_info **vars =
        (struct reg_info **)dbuffer_asPtrArray(&variableList, &variableCount);

#if 0 
    for (size_t i = 0; i < variableCount; i++) {
        struct reg_info *var = vars[i];

        size_t assignCount;
        inst_assign_var_t **assigns =
            (inst_assign_var_t**)dbuffer_asPtrArray(&var->assigns, &assignCount); 

        dbuffer_t tmp;
        dbuffer_init(&tmp);
        for (size_t iA = 0; iA < assignCount; iA++) {
            inst_assign_var_t *assign = assigns[iA];
            inst_dump(ctx, &assign->inst, &tmp); 
        }
        dbuffer_pushChar(&tmp, 0);
        printf("%s\n", tmp.buffer); 
        dbuffer_free(&tmp);
    }

#endif

    // ---- Step 2, Create phi's. ----
    // Inserting a phi inside a effectively creates a new value. So this needs
    // to be a iterative

    // A worklist of value, block pairs.
    dbuffer_t worklist;
    dbuffer_init(&worklist);

    // we use this as a set.
    size_t *lastIteration = dzmalloc(doms->elementCount * sizeof(size_t));

    // iterate over variables. Examine blocks that assigned to it.
    for (size_t i = 0; i < variableCount; i++) {
        struct reg_info *var = vars[i];

        size_t assignCount;
        inst_assign_var_t **assigns = (inst_assign_var_t **)dbuffer_asPtrArray(
            &var->assigns, &assignCount);
        dbuffer_clear(&worklist);

        // go over the assignmensts.
        for (size_t iA = 0; iA < assignCount; iA++) {
            inst_assign_var_t *assign = assigns[iA];
            basic_block_t *block = assign->inst.parent;
            size_t bId = dominators_getNumber(doms, block);
            lastIteration[bId] = i;
            dbuffer_pushPtr(&worklist, assign->var->value);
            dbuffer_pushPtr(&worklist, block);
        }

        size_t worklistSize;
        void **worklistArray = dbuffer_asPtrArray(&worklist, &worklistSize);

        for (size_t iW = 0; iW < worklistSize; iW += 2) {
            value_t *value = (value_t *)worklistArray[iW];
            basic_block_t *block = (basic_block_t *)worklistArray[iW + 1];

            size_t dfCount;
            basic_block_t **dfArray = domfrontiers_get(df, block, &dfCount);
            // iterate over dominance frontiers.
            for (size_t iDf = 0; iDf < dfCount; iDf++) {
                basic_block_t *dfBlock = dfArray[iDf];
                size_t bId = dominators_getNumber(doms, dfBlock);
                struct block_info *bInfo = &blockInfo[bId];

                struct phi_info *phi = block_info_getOrCreatePhi(
                    bInfo, dfBlock, ctx, &zone, var->rId, value->dataType);
                // inst_phi_insertValue(phi->phiInst, ctx, block, value);

                // if we haven't visited this block for this variable, insert it
                // into our worklist.
                if (lastIteration[bId] < i) {
                    lastIteration[bId] = i; // now we did.
                    dbuffer_pushPtr(&worklist, &phi->phiInst->inst.value);
                    dbuffer_pushPtr(&worklist, dfBlock);
                }
            }

            // We might have inserted new elements, the size or the pointer
            // might have changed.
            worklistArray = dbuffer_asPtrArray(&worklist, &worklistSize);
        }
    }

    ssa_rename(ctx, blockInfo, &variableMap, doms, fun->entry);

    // ---- Done, free data and cleanup ----
    vars =
        (struct reg_info **)dbuffer_asPtrArray(&variableList, &variableCount);

    // cleanup variable info.
    for (size_t i = 0; i < variableCount; i++) {
        dbuffer_free(&vars[i]->assigns);
        dbuffer_free(&vars[i]->valueStack);
    }

    // Free the contents of block info
    for (size_t i = 0; i < doms->elementCount; i++) {
        struct block_info *bInfo = &blockInfo[i];

        // iterate over phi instructions and remove ones that are not needed.
        size_t phiCount;
        struct phi_info **phiList =
            (struct phi_info **)dbuffer_asPtrArray(&bInfo->phiList, &phiCount);
        for (size_t pI = 0; pI < phiCount; pI++) {
            inst_phi_t *phiInst = phiList[pI]->phiInst;
            // This phi instruction doesn't have any uses, we can just remove
            // it.
            if (!value_hasUse(&phiInst->inst.value))
                inst_remove(&phiInst->inst);
        }

        hashmap_free(&bInfo->regMap);
        dbuffer_free(&bInfo->regList);

        hashmap_free(&bInfo->phiMap);
        dbuffer_free(&bInfo->phiList);

        dbuffer_free(&bInfo->loadAssigns);
    }

    zone_free(&zone);
    dbuffer_free(&worklist);
    hashmap_free(&variableMap);

    free(blockInfo);
}
