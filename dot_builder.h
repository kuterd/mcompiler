// Kuter Dinel [Based on old works]
// Builds a graphwiz file.
#ifndef DOT_BUILDER_H
#define DOT_BUILDER_H

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "buffer.h"

#define PRETTY_PRINT

struct Graph {
    int isDirected;
    dbuffer_t dbuffer;
};

void graph_init(struct Graph *graph, char *name, int isDirected);
void graph_addEdgeVProp(struct Graph *graph, char *aName, char *bName, char *edgeProps); 
void graph_addEdge(struct Graph *graph, char *aName, char *bName);
void graph_setNodeProps(struct Graph *graph, char *nodeName, char *props);
char* graph_finalize(struct Graph *graph);

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
  dbuffer_pushChar(&graph->buffer, 0); // null terminate.
  return (char*)graph->dbuffer.buffer; 
}

#undef _
#undef _p
#endif
