#ifndef DOMINATORS_H
#define DOMINATORS_H

#include "hashmap.h"
#include "ir.h"
#include "zone_alloc.h"

#include <assert.h>

struct dom_node {
    // list of dom_node ptrs.
    dbuffer_t childs;
    size_t number;
};

struct domfrontiers {
    hashmap_t dfMap;
    zone_allocator zone;
};

struct dominators {
    struct dom_node *domNodes;
    size_t *doms;

    hashmap_t nodeToNum;
    size_t nodeCount;
    // We should try to move this to somewhere else.
    basic_block_t **postorder;
    size_t elementCount;

    zone_allocator allocator;
};

// TODO: We want to eleminate this eventually.
size_t dominators_getNumber(struct dominators *dom, basic_block_t *block);

// get the dom node for this block.
struct dom_node *dominators_getNode(struct dominators *dom,
                                    basic_block_t *block);

// Compute the dominators.
void dominators_compute(struct dominators *doms, basic_block_t *entry);

// Free the dominator data.
void dominators_free(struct dominators *doms);

// Get the immeidate dominator of the \p block
basic_block_t *dominators_getIDom(struct dominators *doms,
                                  basic_block_t *block);

// Compute the dominator frontiers based on the dominators.
void domfrontiers_compute(struct domfrontiers *df, struct dominators *doms);

// Get a list of dominance frontiers.
basic_block_t **domfrontiers_get(struct domfrontiers *df, basic_block_t *block,
                                 size_t *size);

void dominators_dumpDot(struct dominators *doms, ir_context_t *ctx);

struct dominator_child_it {
    struct dominators *doms;
    struct dom_node *node;
    size_t i;
};

// initialize a iterator for children that are dominated by this block.
struct dominator_child_it dominator_child_begin(struct dominators *doms,
                                                basic_block_t *block);

// Get the next
struct dominator_child_it dominator_child_next(struct dominator_child_it it);

// Is this the end of the iterator/
int dominator_child_end(struct dominator_child_it it);

// Get the block from the dominator.
basic_block_t *dominator_child_get(struct dominator_child_it it);

#endif
