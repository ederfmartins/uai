#ifndef _AST_NODE_H
#define _AST_NODE_H

#include "../collections/linkedList.h"

#include <llvm-c/Core.h>

typedef struct
{
    LLVMValueRef llvm;
    LLVMTypeRef return_type;
    int return_type_valid;
    LinkedList instructions_list;
} Node;

Node node_init();
void node_destroy(Node* node);
Node node_merge(Node n1, Node n2);
void node_add_instruction(Node* node, LLVMValueRef inst);

typedef struct
{
    LLVMTypeRef type;
    const char* name;
} Parameter;

#endif
