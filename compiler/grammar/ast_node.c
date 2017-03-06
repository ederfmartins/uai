#include "../grammar/ast_node.h"

#include <stdlib.h>

Node node_init()
{
    Node node;
    node.llvm = NULL;
    node.return_type_valid = 0;
    ll_init(&node.instructions_list);
    return node;
}

void node_destroy(Node* node)
{
    node->llvm = NULL;
    node->return_type_valid = 0;
    ll_destroy(&node->instructions_list);
}

Node node_merge(Node n1, Node n2)
{
    if (n2.return_type_valid)
    {
        n1.return_type_valid = n2.return_type_valid;
        n1.return_type = n2.return_type;
    }

    ll_transfer(&n1.instructions_list, &n2.instructions_list);
    node_destroy(&n2);
    return n1;
}

void node_add_instruction(Node* node, LLVMValueRef inst)
{
    node->llvm = inst;
    ll_insert(&node->instructions_list, node->llvm);
}
