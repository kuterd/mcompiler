#include "parser.h"
#include "format.h"
#include "dot_builder.h"

// ---- Tokenizer ----
char *token_type_names[] = {
    TK_TOKEN_TYPES(STR_COMMA)
};

//@return is 1 if EOF.
int reader_advance(struct reader *reader, size_t i) {
    while (i > 0) {
        if (reader->range.size == 0)
           return 1;
        char c = *reader->range.ptr;
        position_ingest(&reader->pos, c);
        range_skip(&reader->range, 1);
        i--; 
    }
    return 0;
}

#define _ reader->

void read_id(struct reader *reader, struct token *token) {
    token->range.size = 0;

    while (_ range.size > 0) {
        char c = * _ range.ptr;
        if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z'))
            break;
        reader_advance(reader, 1);
        token->range.size++;
    }
    
    if (range_cmp(token->range, RANGE_STRING("while"))) {
        token->type = TK_KW_WHILE;
        return;
    } else if (range_cmp(token->range, RANGE_STRING("if"))) {
        token->type = TK_KW_IF;
        return;
    } else if (range_cmp(token->range, RANGE_STRING("else"))) {
        token->type = TK_KW_ELSE;
        return; 
    }
    token->type = TK_ID;
}

void read_num(struct reader *reader, struct token *token) {
    token->type = TK_NUMBER;
    token->range.size = 0;
    while (_ range.size > 0) {
        char c = * _ range.ptr;
        if (c < '0' || c > '9')
            break;
        reader_advance(reader, 1);
        token->range.size++;
    }
}

void reader_skipWhite(struct reader *reader) {
   while (_ range.size > 0) {
        char c = * _ range.ptr;
        if (c != '\t' && c != '\n' && c != ' ' && c != '\r')
            break;
        reader_advance(reader, 1);
   }
}

void token_next(struct reader *reader, struct token *token) {
    reader_skipWhite(reader);
    *token = (struct token) {.type = TK_ERROR, .pos = _ pos}; 
    if ( _ range.size == 0) {
        token->type = TK_EEOF;
        return;
    }

    token->range = _ range;
    token->range.size = 1;
    char c = * _ range.ptr; 
    switch(c) {
        case '{':
            token->type = TK_CURLY_OPEN;
            reader_advance(reader, 1); 
            return;
        case '}':
            token->type = TK_CURLY_CLOSE;
            reader_advance(reader, 1); 
            return;
        case '(':
            token->type = TK_PARAN_OPEN;
            reader_advance(reader, 1); 
            return;
        case ')':
            token->type = TK_PARAN_CLOSE;
            reader_advance(reader, 1); 
            return;
        case '+':
            token->type = TK_PLUS;
            reader_advance(reader, 1);
            return;
        case '-':
            token->type = TK_MINUS;
            reader_advance(reader, 1);
            return;
         case '*':
            token->type = TK_MUL;
            reader_advance(reader, 1);
            return;
         case '/':
            token->type = TK_DIV;
            reader_advance(reader, 1);
            return;
        case 'a' ... 'z':
            // flow through.
        case 'A' ... 'Z':
            read_id(reader, token);
            break;
        case '0' ... '9':          
            read_num(reader, token);
            break;
    }
}

void token_dump(struct token *token) {
    char *type = token_type_names[token->type];
    printf("Type: %s\n", type);
    if (token->type == TK_ERROR || token->type == TK_EEOF) 
        return;

    printf("Contents:%.*s\n", token->range.size, token->range.ptr);
    printf("Position: ");
    position_dump(&token->pos);
    puts("");
}

void token_dumpAll(char *string) {
    range_t range = range_fromString(string);
    struct reader reader = (struct reader) {.range = range};

    struct token token = (struct token){};     
    do {
        token_next(&reader, &token); 
        token_dump(&token);
    } while (token.type != TK_ERROR && token.type != TK_EEOF);

}

// used for things like ast_module_new 
#define GEN_NEW_DEFINITION(enu, prefix)                                       \
    AST_TYPE(prefix)* ast_##prefix##_new() {                                  \
        AST_TYPE(prefix) *result = nnew(AST_TYPE(prefix));                    \
        *result = (AST_TYPE(prefix)){.node = (struct ast_node) {.type = enu}};\
        return result;                                                        \
    }

AST_NODE_TYPE(GEN_NEW_DEFINITION)

/// ---- AST Manuplation----

#define STR_COMMA2(e, p) #e,

char *kASTNodeName[] = {
    AST_NODE_TYPE(STR_COMMA2)
};

#define VISIT_NO_CHILD(enu)                          \
    case enu:                                        \
        *childCount = 0;                             \
        break;

#define VISIT_CONST_CHILD(enu, prefix, chCount)      \
    case enu:                                        \
        *childs = AST_AS_TYPE(node, prefix)->childs; \
        *childCount = chCount;                       \
        break;

#define VISIT_VARIABLE_CHILD(enu, prefix)                    \
    case enu:                                                \
        *childs = AST_AS_TYPE(node, prefix)->childs;         \
        *childCount = AST_AS_TYPE(node, prefix)->childCount; 

void ast_getChilds(struct ast_node *node, size_t *childCount, struct ast_node ***childs) {
     switch(node->type) {
        VISIT_NO_CHILD(NUMBER)
        VISIT_NO_CHILD(STRING)
        VISIT_NO_CHILD(VARIABLE)
        VISIT_CONST_CHILD(WHILE, while, 2)
        VISIT_CONST_CHILD(PREFIX, prefix, 1)
        VISIT_CONST_CHILD(BINARY_EXP, binary_exp, 2)
        VISIT_VARIABLE_CHILD(MODULE, module)
     }
}

void ident_dbuffer(dbuffer_t *buffer, size_t i) { 
    dbuffer_pushChars(buffer, '\t', i);
}

void ast_nodeInfoBuffer(struct ast_node *node, dbuffer_t *buffer, size_t i) {
    ident_dbuffer(buffer, i);
    format_dbuffer("Node Type: {str}", buffer, kASTNodeName[node->type]);
    
    switch(node->type) {
        case NUMBER:
            ident_dbuffer(buffer, i);
            format_dbuffer("({int})", buffer, AST_AS_TYPE(node, number)->num);
            break;
        case BINARY_EXP:
            ident_dbuffer(buffer, i);
            format_dbuffer("({str})", buffer, token_type_names[AST_AS_TYPE(node, binary_exp)->op]);
            break;
        case VARIABLE:
            ident_dbuffer(buffer, i);
            format_dbuffer("({range})", buffer, AST_AS_TYPE(node, variable)->varName); 
            break;
    } 
}

void ast_nodeInfo(struct ast_node *node, size_t i) {
    dbuffer_t dbuffer;
    dbuffer_init(&dbuffer);
    ast_nodeInfoBuffer(node, &dbuffer, i);
    dbuffer_pushChar(&dbuffer, 0);
    puts(dbuffer.buffer);
}

void ast_dump(struct ast_node *node, size_t i) {
    ast_nodeInfo(node, i);
    size_t childCount;
    struct ast_node **childs;
    ast_getChilds(node, &childCount, &childs);
    
    for (size_t ii = 0; ii < childCount; ii++) {
        ast_dump(childs[ii], i + 1);
    } 
}

void _ast_dumpDot(struct ast_node *node, struct Graph *graph) {
    size_t childCount;
    struct ast_node **childs;
    ast_getChilds(node, &childCount, &childs);
    
    char nodeId[MAX_NODE_ID];
    getNodeId(node, nodeId);
    
    dbuffer_t buffer;
    dbuffer_init(&buffer);
    dbuffer_pushStr(&buffer, "label=\"");
    ast_nodeInfoBuffer(node, &buffer, 0);
    dbuffer_pushChar(&buffer, '\"');
    dbuffer_pushChar(&buffer, 0);
    graph_setNodeProps(graph, nodeId, buffer.buffer);
    free(buffer.buffer);

    for (size_t ii = 0; ii < childCount; ii++) {
        char childId[MAX_NODE_ID];
        getNodeId(childs[ii], childId); 
        graph_addEdge(graph, nodeId, childId);
        _ast_dumpDot(childs[ii], graph);
    } 
}

char* ast_dumpDot(struct ast_node *node) {
    struct Graph graph;
    graph_init(&graph, "ast", 1);
    
    _ast_dumpDot(node, &graph); 

    char *r = graph_finalize(&graph);
    return r;
}

/// ---- Parser ----

// TODO: error formating.
#define parser_check(check, error_print) if (!(check)) { parser->error = format error ; return NULL; }
#define parser_check_silent(check) if(!(check)) return NULL;

struct token parser_peekToken(parser_t *parser) {
    if (!parser->hasPeek)
        token_next(&parser->reader, &parser->peek);
    parser->hasPeek = 1;
    return parser->peek;
}

struct token parser_next(parser_t *parser) {
    struct token token;
    if (parser->hasPeek) {
        token = parser->peek;
        parser->peek = (struct token){.type=TK_ERROR};
        parser->hasPeek = 0;
        return token;
    }
    token_next(&parser->reader, &token);
    return token;  
}

// discard the peeked token and get a fresh one.
struct token parser_fresh(parser_t *parser) {
    parser->peek = (struct token){.type=TK_ERROR};
    parser->hasPeek = 0;
   
    struct token token;
    token_next(&parser->reader, &token);
    return token;
}

void parser_init(parser_t *parser, range_t range) {
    // initialize the reader.
    *parser = (parser_t){.reader = (struct reader) { .range = range }};
}

struct ast_module* parser_parseModule(parser_t *parser) {
    struct ast_module *module = ast_module_new(); 
}

// Operator presedence.
// *, /
// +, -
// =
int getPresedence(enum token_type type) {
    switch(type) {
        case TK_PLUS:
        case TK_MINUS:
            return 4;
        case TK_MUL:
        case TK_DIV:
            return 5;
        default:
            return -1;
    }
}

// prefix expression, number, paranthesis expression. 
struct ast_node* parser_readAtomInternal(parser_t *parser, int hasPrefix) {
    struct token tok = parser_next(parser); 
    
    // Have we already parsed a prefix ?
    if (!hasPrefix && (tok.type == TK_MINUS || tok.type == TK_PLUS)) {
        struct ast_node *node = parser_readAtomInternal(parser, 1);
        if (tok.type == TK_PLUS)
            AST_AS_TYPE(node, number)->num *= -1; 
        return node;
    } else if (tok.type == TK_PARAN_OPEN) { 
        struct ast_node *node = parser_parseExpression(parser);
        parser_next(parser); // TODO: error handling.
        return node;
    } else if (tok.type == TK_NUMBER) {
        struct ast_number *number = ast_number_new(); 
        number->num = range_parseInt(tok.range);
        return &number->node;
    } else if (tok.type == TK_ID) {
        struct ast_variable *variable = ast_variable_new();
        variable->varName = tok.range;
        return &variable->node;
    }
}

struct ast_node* parser_parseBinary(parser_t *parser, struct ast_node *left);
 
struct ast_node* parser_readAtom(parser_t *parser) {
    return parser_readAtomInternal(parser, 0);
}

struct ast_node* parser_parseExpression(parser_t *parser) {
    struct ast_node *node = parser_readAtom(parser); 
    return parser_parseBinary(parser, node);    

}

struct ast_node* parser_parseBinary(parser_t *parser, struct ast_node *left) {
    struct token op = parser_peekToken(parser); 
    int opPrec = getPresedence(op.type);
    
    // if the token is not a operator, we are done.
    if (opPrec == -1)
        return left;
    parser_next(parser); // skip the operator. 

    struct ast_node *node = parser_readAtom(parser); 
    struct ast_binary_exp *exp = ast_binary_exp_new();
    exp->op = op.type;
    exp->left = left;
    exp->right = node;
    struct ast_node *result = parser_parseBinary(parser, &exp->node);

    // since left is a binary expression, the result must also be a binary expression.
    struct ast_binary_exp *result_exp = AST_AS_TYPE(result, binary_exp);
    // We fix the operator precedence with a right tree rotation.
    // Expression: a + b * c
    //
    //    *               +    
    //   /  c    --->   a   \
    //  +                    *    
    // a b                  b c
    //
    
    // Expression:  10 > a + b * c
    //
    //       +               >          
    //     /   \          10   \
    //    >     *   --->        +
    //  10 a   b c            a  \
    //                            *
    //                           b  c 
    
    // Nothing to rotate. 
    if (result_exp->left->type != BINARY_EXP)
        return result;

    int rootPrec = getPresedence(result_exp->op);
    struct ast_binary_exp *left_side = AST_AS_TYPE(result_exp->left, binary_exp); 
    
    //NOTE: if the precedences are equal, then the expression should be evaluated from left to right.
    // in that case it is already in the correct oriantation.
    
    // Rotation needed.
    if (rootPrec > opPrec) {
        //left node becomes the root.
        struct ast_node *z = left_side->right; 
        left_side->right = result;
        result = result_exp->left; 
        result_exp->left = z; 
    }

    return result;
}
