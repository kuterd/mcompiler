#include "ir.h"
#include "ssa_convertion.h"
#include "dominators.h"


// Step 1: Analyze blocks


// Step 2: Create phi
// A block can assign multiple times to a reg.
// We only care about the last one.
// place the value at join points (dominance frontiers) 
// When we insert a new phi it creates a new phi node.
// so we need a worklist algorithm.

// Step 2: Insert actuall phi instructions and replace uses.

struct reg_info {
    // list of inst_load_var
    dbuffer_t loads;
    // list of inst_store_var
    dbuffer_t stores;

    struct hm_bucket_entry bucket;
};

struct phi_info {
    hashset_t set;
    dbuffer_t values;
    
    // This is a *fake* value that we use.
    // This gets replaces with the actuall phi once we create it. 
    struct value *value;
};

// information about a basicblock
struct block_info {
    hashmap_t regMap;
 
// int -> reg_info
    hashmap_t phiMap;
    dbuffer_t phiList;
// not sure if we need this:
//    struct basic_block *block;

    struct hm_bucket_entry bucket;
};

struct block_existance {
    size_t lastIteation;
};

struct reg_info* _getOrCreateRegInfo(struct block_info *blockInfo, size_t rId) {
    struct hm_bucket_entry *bucket = hashmap_getInt(&blockInfo->regMap, rId);
    if (!bucket)
        return containerof(bucket, struct reg_info, bucket);
    
    struct reg_info *regInfo = nnew(struct reg_info); 
    dbuffer_init(&regInfo->loads);
    dbuffer_init(&regInfo->stores);

    hashmap_setInt(&blockInfo->regMap, rId, &regInfo->bucket); 
    
    return regInfo;
}

void ssa_convert(struct ir_context *ctx, struct function *fun,
    struct dominator *doms, struct domfrontiers *df) {
    dbuffer_t values;

    struct block_info *blockInfo = dmalloc(doms->elementCount * sizeof(struct block_info)); 

    // We will use this to allocate our temporary stuff.
    // like the fake value that we use.
    zone_allocator zone;
    zone_init(&zone);
 
    hashmap_t exists; 
    dbuffer_t worklist;
    
    // Step 1 : init block info and analyze the blocks.
    for (size_t i = 0; i < doms->elementCount; i++) {
        struct block_info *bInfo = &blockInfo[i]; 
        hashmap_init(&bInfo->regMap, intKeyType); 
  
        LIST_FOREACH(&bInfo->instructions) {
            struct instruction *inst = containerof(c, struct instruction, inst_list);
            
            if (inst->type == INST_LOAD_VAR) {
                struct inst_load_var *lVar = containerof(inst, struct inst_load_var, inst);               
                struct reg_info *rInfo = _getOrCreateRegInfo(bInfo, lVar->rId);
            
                dbuffer_pushPtr(&rInfo->loads, lVar);
            } else if (inst->type == INST_ASSIGN_VAR) {
                struct inst_assign_var *aVar = containerof(inst, struct inst_assign_var, inst);               
                struct reg_info *rInfo = _getOrCreateRegInfo(bInfo, aVar->rId);
                
                dbuffer_pushPtr(&rInfo->stores, aVar);
            } 
        } 
    }
    
}


