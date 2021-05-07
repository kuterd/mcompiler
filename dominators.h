#ifndef DOMINATORS_H
#define DOMINATORS_H

#include "zone_alloc.h"
#include "ir.h"
#include "hashmap.h"

struct domfrontiers {
    hashmap_t dfMap;
    zone_allocator zone;
};

struct dominators {
    struct basic_block **postorder;
    size_t *doms; 
    hashmap_t nodeToNum;
    size_t nodeCount;
    zone_allocator allocator;
    size_t elementCount;
};

// Compute the dominators.
void dominators_compute(struct dominators *doms, struct basic_block *entry);
void dominators_free(struct dominators *doms);

// Get the immeidate dominator of the \p block
struct basic_block* dominators_getIDom(struct dominators *doms, struct basic_block *block);

// Compute the dominator frontiers based on the dominators.
void domfrontiers_compute(struct domfrontiers *df, struct dominators *doms);

// Get a list of dominance frontiers.
struct basic_block** domfrontiers_get(struct domfrontiers *df, struct basic_block *block, size_t *size);
#endif
