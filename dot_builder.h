// Kuter Dinel [Based on old works]
// Builds a graphwiz file.
#ifndef DOT_BUILDER_H
#define DOT_BUILDER_H

#include "buffer.h"
#define MAX_NODE_ID (sizeof(void*) * 2)

struct Graph {
    int isDirected;
    dbuffer_t dbuffer;
};

void graph_init(struct Graph *graph, char *name, int isDirected);
void graph_addEdgeVProp(struct Graph *graph, char *aName, char *bName, char *edgeProps); 
void graph_addEdge(struct Graph *graph, char *aName, char *bName);
void graph_setNodeProps(struct Graph *graph, char *nodeName, char *props);
char* graph_finalize(struct Graph *graph);

void getNodeId(void *node, char *result);

#endif
