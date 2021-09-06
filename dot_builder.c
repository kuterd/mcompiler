#include "dot_builder.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#define PRETTY_PRINT

#define _(str) dbuffer_pushStr(&graph->dbuffer, (str))

#ifdef PRETTY_PRINT
#define _p(str) _(str) 
#else
#define _p(str) 
#endif

void graph_init(struct Graph *graph, char *name,
		       int isDirected) {
  graph->isDirected = isDirected;
  dbuffer_init(&graph->dbuffer);

  if (isDirected)
    _("digraph ");
  else 
    _("graph ");
  
  _(name);
  _(" {");
  _p("\n");
}

void graph_addEdge(struct Graph *graph, char *aName, char *bName) { 
  graph_addEdgeVProp(graph, aName, bName, NULL);
}

void graph_addEdgeVProp(struct Graph *graph, char *aName, char *bName, char *edgeProps) { 
  _p("\t");

  _(aName);
  _(" -> ");
  _(bName);

  if(edgeProps != NULL) {
    _("[");
    _(edgeProps);
    _("]");
  }

  _(";");
  _p("\n");
}

void graph_setNodeProps(struct Graph *graph, char *nodeName, char *props) {
  _(nodeName);
  _("[");
  _(props);
  _("]");

  _p("\n");
}

char* graph_finalize(struct Graph *graph) {
  _("}");
  dbuffer_pushChar(&graph->dbuffer, 0); // null terminate.
  return (char*)graph->dbuffer.buffer; 
}

void getNodeId(void *node, char *result) {
    snprintf(result, MAX_NODE_ID, "node_%x", (unsigned long long)node); 
}

#undef _
#undef _p

