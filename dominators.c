#include "dominators.h"
#include "buffer.h"
#include "dot_builder.h"
#include "hashmap.h"

#include "format.h"
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
void _visit(struct dominators *dom, dbuffer_t *postorder,
            basic_block_t *block) {
    struct hm_bucket_entry *entry = hashmap_getPtr(&dom->nodeToNum, block);
    // If we have a number for this node, return.
    if (entry)
        return;
    struct node_data *data = znnew(&dom->allocator, struct node_data);
    hashmap_setPtr(&dom->nodeToNum, block, &data->entry);

    struct block_successor_it it = block_successor_begin(block);
    for (; !block_successor_end(it); it = block_successor_next(it)) {
        _visit(dom, postorder, block_successor_get(it));
    }

    // Assign a number.
    data->number = postorder->usage / sizeof(void *);
    dbuffer_pushPtr(postorder, block);
}

size_t dominators_getNumber(struct dominators *dom, basic_block_t *block) {
    struct hm_bucket_entry *entry = hashmap_getPtr(&dom->nodeToNum, block);
    struct node_data *data = containerof(entry, struct node_data, entry);
    return data->number;
}

// `real` number for the element.
#define RNUM(c) (doms->elementCount - (c)-1)

size_t intersect(struct dominators *doms, size_t a, size_t b) {
    while (a != b) {
        while (a < b)
            a = doms->doms[a];
        while (b < a)
            b = doms->doms[b];
    }
    return a;
}

void dominators_compute(struct dominators *doms, basic_block_t *entry) {
    // Init the @doms
    zone_init(&doms->allocator);
    hashmap_init(&doms->nodeToNum, ptrKeyType);

    // Post order visit.
    dbuffer_t dPostorder;
    dbuffer_init(&dPostorder);
    _visit(doms, &dPostorder, entry);

    basic_block_t **postorder = (basic_block_t **)dPostorder.buffer;
    doms->postorder = postorder;

    // Allocate dom array.
    size_t elementCount = dPostorder.usage / sizeof(void *);
    doms->doms = dmalloc(elementCount * sizeof(size_t));
    doms->elementCount = elementCount;
    doms->domNodes = dmalloc(elementCount * sizeof(struct dom_node));

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
            basic_block_t *block = postorder[i];
            size_t newDom = doms->doms[i];

            struct block_predecessor_it it = block_predecessor_begin(block);
            if (!block_predecessor_end(it)) {
                basic_block_t *predBlock = block_predecessor_get(it);
                newDom = dominators_getNumber(doms, predBlock);
                it = block_predecessor_next(it);
            }

            for (; !block_predecessor_end(it);
                 it = block_predecessor_next(it)) {
                basic_block_t *predBlock = block_predecessor_get(it);
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

    // Construct a dominator tree from the reverse nodes.

    // Start by initializing every tree node.
    for (size_t i = 0; i < elementCount; i++) {
        struct dom_node *dNode = &doms->domNodes[i];
        dNode->number = i;
        dbuffer_initSize(&dNode->childs, 8 * sizeof(void *));
    }

    for (size_t i = 0; i < elementCount; i++) {
        size_t iDom = doms->doms[i];
        if (iDom == i)
            continue;
        dbuffer_pushPtr(&doms->domNodes[iDom].childs, &doms->domNodes[i]);
    }
}

basic_block_t **dominators_getChilds(struct dominators *doms, basic_block_t *,
                                     size_t *count);

basic_block_t *dominators_getIDom(struct dominators *doms,
                                  basic_block_t *block) {
    size_t blockNum = dominators_getNumber(doms, block);
    return doms->postorder[doms->doms[blockNum]];
}

void dominators_free(struct dominators *doms) {
    for (size_t i = 0; i < doms->elementCount; i++) {
        dbuffer_free(&doms->domNodes[i].childs);
    }

    zone_free(&doms->allocator);
    hashmap_free(&doms->nodeToNum);
    free(doms->postorder);
    free(doms->domNodes);
    free(doms->doms);
}

struct df_element {
    dbuffer_t dlist;
    struct hm_bucket_entry bucket;
};

struct df_element *_df_getElement(struct domfrontiers *df,
                                  basic_block_t *block) {
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
    zone_init(&df->zone);

    for (size_t i = 0; i < doms->elementCount; i++) {
        basic_block_t *block = doms->postorder[i];

        struct block_predecessor_it it = block_predecessor_begin(block);
        for (; !block_predecessor_end(it); it = block_predecessor_next(it)) {
            basic_block_t *runner = block_predecessor_get(it);
            basic_block_t *dom = dominators_getIDom(doms, block);

            while (runner != dom) {
                struct df_element *dfElem = _df_getElement(df, runner);
                dbuffer_pushPtr(&dfElem->dlist, block);
                runner = dominators_getIDom(doms, runner);
            }
        }
    }
}

// Get a list of dominance frontiers.
basic_block_t **domfrontiers_get(struct domfrontiers *df, basic_block_t *block,
                                 size_t *size) {
    struct df_element *dfElem = _df_getElement(df, block);
    *size = dfElem->dlist.usage / sizeof(void *);
    return (basic_block_t **)dfElem->dlist.buffer;
}

void _dumpDotNode(struct dominators *doms, struct Graph *graph,
                  ir_context_t *ctx, struct dom_node *node) {
    char nodeName[MAX_NODE_ID];
    getNodeId(node, nodeName);
    size_t childCount;
    struct dom_node **childs =
        (struct dom_node **)dbuffer_asPtrArray(&node->childs, &childCount);

    basic_block_t *block = doms->postorder[node->number];
    range_t name = value_getName(ctx, &block->value);
    char *props = format("label=\"!{range}\"", name);
    graph_setNodeProps(graph, nodeName, props);
    free(props);
    for (size_t i = 0; i < childCount; i++) {
        struct dom_node *childNode = childs[i];
        char childName[MAX_NODE_ID];
        getNodeId(childNode, childName);
        graph_addEdge(graph, nodeName, childName);
        _dumpDotNode(doms, graph, ctx, childNode);
    }
}

void dominators_dumpDot(struct dominators *doms, ir_context_t *ctx) {
    struct Graph graph;
    graph_init(&graph, "dominators", 1);
    _dumpDotNode(doms, &graph, ctx, &doms->domNodes[doms->elementCount - 1]);
    puts(graph_finalize(&graph));
}

struct dom_node *dominators_getNode(struct dominators *dom,
                                    basic_block_t *block) {
    return &dom->domNodes[dominators_getNumber(dom, block)];
}

struct dominator_child_it dominator_child_begin(struct dominators *doms,
                                                basic_block_t *block) {
    size_t i = dominators_getNumber(doms, block);
    struct dom_node *node = &doms->domNodes[i];

    return (struct dominator_child_it){.doms = doms, .node = node, .i = 0};
}

struct dominator_child_it dominator_child_next(struct dominator_child_it it) {
    it.i++;
    return it;
}

basic_block_t *dominator_child_get(struct dominator_child_it it) {
    assert(!dominator_child_end(it) && "extected valid iteartor");

    return it.doms
        ->postorder[((struct dom_node **)it.node->childs.buffer)[it.i]->number];
}

int dominator_child_end(struct dominator_child_it it) {
    return it.i >= it.node->childs.usage / sizeof(void *);
}
