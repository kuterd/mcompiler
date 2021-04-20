#ifndef PARSER_H
#define PARSER_H

#include "utils.h"
#include "buffer.h"

#define TOKEN_TYPES(o) \
    o(TK_EEOF)        \
    o(TK_ERROR)       \
    o(TK_ID)          \
    o(TK_KW_WHILE)    \
    o(TK_KW_IF)       \
    o(TK_KW_ELSE)     \
    o(TK_PLUS)        \
    o(TK_MINUS)       \
    o(TK_STRING)      \
    o(TK_MUL)         \
    o(TK_DIV)         \
    o(TK_NUMBER)      \
    o(TK_CURLY_OPEN)  \
    o(TK_CURLY_CLOSE) \
    o(TK_PARAN_OPEN)  \
    o(TK_PARAN_CLOSE) \

enum token_type {
    TOKEN_TYPES(COMMA)
};

struct token {
    enum token_type type; 
    position_t pos;
    range_t range;   
};

#define AST_NODE_TYPE(o)            \
    o(NUMBER, number)               \
    o(STRING, string)               \
    o(VARIABLE, variable)           \
    o(BINARY_EXP, binary_exp)       \
    o(WHILE, while)                 \
    o(IF, if)                       \
    o(MODULE, module)               \
    o(FUNCTION, function)           \
    o(FUNCTION_CALL, function_call) \
    o(BLOCK, block)                 \
    o(PREFIX, prefix)               \
    o(RETURN, return)

#define AST_TYPE(prefix) struct ast_##prefix
#define AST_AS_TYPE(ptr, prefix) containerof(ptr, AST_TYPE(prefix), node) 

#define COMMA2(e, p) e,
#define STR_COMMA2(e, p) #e,

enum ast_node_type {
    AST_NODE_TYPE(COMMA2)
};

extern char *kASTNodeName[];

struct ast_node {
    enum ast_node_type type;
}; 

struct ast_module {
    struct ast_node node;

    struct ast_node **childs;
    size_t childCount;
};

struct ast_function {
    struct ast_node node;
    range_t name;
    struct ast_node **childs;
    size_t childCount;
};

struct ast_function_call {
    struct ast_node node;
    range_t name;

    struct ast_node **childs;
    size_t childCount;
};

struct ast_while {
    struct ast_node node;
    union {
        struct ast_node *childs[2];
        struct {
            struct ast_node *condition;
            struct ast_node *block;
        };
    };
};

struct ast_block {
    struct ast_node node;

    struct ast_node **childs;
    size_t childCount;
};

struct ast_if {
    struct ast_node node;

    struct ast_node **childs;
    size_t childCount;
};

struct ast_binary_exp {
    struct ast_node node;
    enum token_type op;
    union {
        struct ast_node *childs[2];
        
        struct {
            struct ast_node *left;   
            struct ast_node *right;   
        };
    };
};

struct ast_number {
    struct ast_node node;
    int num;
};

struct ast_variable { 
    struct ast_node node;
    range_t varName;
};

struct ast_string {
    struct ast_node node;
    range_t contents;
};

struct ast_prefix {
    struct ast_node node;
    int isPlus; // plus or minus ?
    union {
        struct ast_node *childs[1];
        struct {
            struct ast_node *child;
        };
    };
};

struct ast_return {
    struct ast_node node;
};

// used for things like ast_module_new 
#define GEN_NEW_DECLERATION(enu, prefix)                    \
    AST_TYPE(prefix)* ast_##prefix##_new();

AST_NODE_TYPE(GEN_NEW_DECLERATION)
/*
#define GEN_VISITOR(enu, prefix)                                                      \
    void (*visit_##prefix)(struct ast_visitor *visitor, struct ast_##prefix *pref); \

// structure for visiting ast.
struct ast_visitor {
    AST_NODE_TYPE(GEN_VISITOR)
    void *info;
};

#define GEN_CASE(enu, prefix)                                        \
    case enu:                                                        \
        visitor->visit_##prefix(visitor, AST_AS_TYPE(node, prefix)); \
        break;

        void ast_visit(struct ast_visitor *visitor, struct ast_node *node) {
   switch(node->type) {
//        AST_NODE_TYPE(GEN_CASE)
   } 
}

#define GEN_CONST_CHILD(enu, prefix, chCount)        \
    case enu:                                        \
        *childs = AST_AS_TYPE(node, prefix)->childs; \
        childCount = chCount;                        \
        break;

#define GEN_VARIABLE_CHILD(enu, prefix)                    \
    case enu:                                              \
        *childs = AST_AS_TYPE(node, prefix)->childs;         \
        childCount = AST_AS_TYPE(node, prefix)->childCount; 

void ast_visitChilds(struct ast_visitor *visitor, struct ast_node *node) {
    size_t childCount;
    struct ast_node **childs; 

    switch(node->type) {
       //AST_NODE_CONST_CHILD(NODE_CONST_PASS, GEN_CONST_CHILD);

    }
}
*/

void ast_nodeInfo(struct ast_node *node, size_t i);
void ast_dump(struct ast_node *node, size_t i);

void token_dumpAll(char *string);

struct reader {
    position_t pos;
    range_t range;
};

typedef struct {
    struct reader reader;
    int hasPeek;
    struct token peek;
    char *error;
} parser_t;

struct ast_node* parser_parseExpression(parser_t *parser);

#undef COMMA2
#undef STR_COMMA2
#undef FIRST2
#undef SECOND2

#endif
