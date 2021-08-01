#include "buffer.h"
#include "dominators.h"
#include "hashmap.h"

#include "ir.h"

#include <limits.h>
#include <stdint.h>

struct node_data {
    int number;
    struct hm_bucket_entry entry;
};

// Steps:
// 1) Create postorder (will be accessed in reverse)
// 2) process elements to create the dom tree as a disjoint set.
void _visit(struct dominators *dom, dbuffer_t *postorder, struct basic_block *block) {
    struct hm_bucket_entry *entry = hashmap_getPtr(&dom->nodeToNum, block);
    // If we have a number for this node, return.
    if (entry)
        return;
    struct node_data *data = znnew(&dom->allocator, struct node_data);
    hashmap_setPtr(&dom->nodeToNum, block, &data->entry); 
    
    struct block_successor_it it = block_successor_begin(block);
    for (; !block_successor_end(it) ; it = block_successor_next(it)) {
        _visit(dom, postorder, block_successor_get(it));
    }
    
    // Assign a number.    
   data->number = postorder->usage / sizeof(void*);
   dbuffer_pushPtr(postorder, block);
}

size_t dominators_getNumber(struct dominators *dom, struct basic_block *block) {
    struct hm_bucket_entry *entry = hashmap_getPtr(&dom->nodeToNum, block);
    struct node_data *data = containerof(entry, struct node_data, entry);
    return data->number;
}

// `real` number for the element.
#define RNUM(c) (doms->elementCount - (c) - 1) 

size_t intersect(struct dominators *doms, size_t a, size_t b) {
   while (a != b) {
       while (a < b)
           a = doms->doms[a]; 
       while (b < a)
           b = doms->doms[b]; 
   }
   return a;
}

void dominators_compute(struct dominators *doms, struct basic_block *entry) {
    // Init the @doms
    zone_init(&doms->allocator);
    hashmap_init(&doms->nodeToNum, ptrKeyType);
     
    // Post order visit.
    dbuffer_t dPostorder;
    dbuffer_init(&dPostorder);
    _visit(doms, &dPostorder, entry);

    struct basic_block **postorder = (struct basic_block **)dPostorder.buffer;
    doms->postorder = postorder;    
 
    // Allocate dom array.
    size_t elementCount = dPostorder.usage / sizeof(void*); 
    doms->doms = dmalloc(elementCount * sizeof(size_t));
    doms->elementCount = elementCount;
 
    // set all elements to SIZE_MAX 
    memset(doms->doms, 0xFF, elementCount * sizeof(size_t));    

    if (elementCount == 0)
        return;
     // doms[start_node] = start_node;
    doms->doms[elementCount - 1] = elementCount - 1;

    int changed = 1;
    while (changed) {
       changed = 0;
       for (size_t i = elementCount - 1; i < elementCount; i--) {
            struct basic_block *block = postorder[i];
            size_t newDom = doms->doms[i];

            struct block_predecessor_it it = block_predecessor_begin(block);
            if (!block_predecessor_end(it)) {
               struct basic_block *predBlock = block_predecessor_get(it);
               newDom = dominators_getNumber(doms, predBlock);
               it = block_predecessor_next(it); 
            }

            for(; !block_predecessor_end(it); it = block_predecessor_next(it)) {
               struct basic_block *predBlock = block_predecessor_get(it);
               size_t predBlockNum = dominators_getNumber(doms, predBlock);
                
               if (doms->doms[predBlockNum] == SIZE_MAX)
                   continue; 
               
               newDom = intersect(doms, predBlockNum, newDom); 
            }
            if (doms->doms[i] != newDom)
                changed = 1; 
            doms->doms[i] = newDom;
        } 
    }
}

struct basic_block* dominators_getIDom(struct dominators *doms, struct basic_block *block) {
    size_t blockNum = dominators_getNumber(doms, block);
    return doms->postorder[doms->doms[blockNum]];
}

void dominators_free(struct dominators *doms) {
    zone_free(&doms->allocator);
    hashmap_free(&doms->nodeToNum);
}

struct df_element {
    dbuffer_t dlist;
    struct hm_bucket_entry bucket;
};

struct df_element* _df_getElement(struct domfrontiers *df, struct basic_block *block) {
    struct hm_bucket_entry *entry = hashmap_getPtr(&df->dfMap, block); 
    if (entry != NULL)
        return containerof(entry, struct df_element, bucket); 
    struct df_element *element = znnew(&df->zone, struct df_element);
    hashmap_setPtr(&df->dfMap, block, &element->bucket);
    dbuffer_init(&element->dlist);
    return element;
}

// Compute the dominator frontiers based on the dominators.
void domfrontiers_compute(struct domfrontiers *df, struct dominators *doms) {
    hashmap_init(&df->dfMap, ptrKeyType);
    for (size_t i = 0; i < doms->elementCount; i++) {
        struct basic_block *block = doms->postorder[i];
        
        struct block_predecessor_it it = block_predecessor_begin(block);
        for(; !block_predecessor_end(it); it = block_predecessor_next(it)) {
            struct basic_block *runner = block_predecessor_get(it);
            struct basic_block *runnerDom = dominators_getIDom(doms, runner);
            struct basic_block *dom = dominators_getIDom(doms, block);
            
            while (runner != dom) {
                struct df_element *dfElem = _df_getElement(df, runner); 
                dbuffer_pushPtr(&dfElem->dlist, block);
                runner = runnerDom; 
                runnerDom = dominators_getIDom(doms, runner);
            }
        } 
    } 
}

// Get a list of dominance frontiers.
struct basic_block** domfrontiers_get(struct domfrontiers *df, struct basic_block *block, size_t *size) {
    struct df_element *dfElem = _df_getElement(df, block);   
    *size = dfElem->dlist.usage / sizeof(void*); 
    return (struct basic_block**)dfElem->dlist.buffer;
}
