#include "parser.h"
#include "utils.h"


int main() {
    struct ast_function function;
    function.node.type = FUNCTION;
    function.childCount = 0;

    struct ast_module module;     
    module.node.type = MODULE;
    module.childs = nnew(struct ast_node*);
    module.childs[0] = &function.node;
    module.childCount = 1; 
    ast_dump(&module.node, 0);


    return 0;
}
