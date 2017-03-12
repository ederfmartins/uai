#include "../grammar/ast_node.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

AbstractSyntacticTree* ast_init()
{
   AbstractSyntacticTree* t = (AbstractSyntacticTree*) malloc (sizeof(AbstractSyntacticTree));
   t->operation = UNDEFINED;
   t->mode = 0;
   // this can be dangerous :(
   // t->value.interior.left = (AbstractSyntacticTree*) NULL;
   // t->value.interior.right = (AbstractSyntacticTree*) NULL;
   return t;
}

void function_node_destroy(FunctionNode* func)
{
    /*
    TODO: verify the ownership off there variables
    const char* func_name;
    const char* ret_type;
    LinkedList* parameters;
    struct tree_node* body;
    */
}

AbstractSyntacticTree* ast_function_definition(const char* func_name,
    const char* ret_type, LinkedList* parameters, LinkedList* body)
{
    AbstractSyntacticTree* ast = ast_init();
    ast->operation = FUNC_DEF;
    ast->value.func_def.func_name = func_name;
    ast->value.func_def.ret_type = ret_type;
    ast->value.func_def.is_vararg = 0;

    if (parameters) ast->value.func_def.parameters = *parameters;
    else ll_init(&ast->value.func_def.parameters);
    
    if (body) ast->value.func_def.body = *body;
    else ll_init(&ast->value.func_def.body);
    return ast;
}

AbstractSyntacticTree* ast_const_bool(const char* value)
{
    AbstractSyntacticTree* ast = ast_init();
    ast->operation = CONST_BOOL;
    if (strcmp(value, "true")) ast->value.leaf.integer_constant = 1;
    else ast->value.leaf.integer_constant = 0;
    return ast;
}

AbstractSyntacticTree* ast_const_int(int value)
{
    AbstractSyntacticTree* ast = ast_init();
    ast->operation = CONST_INT;
    ast->value.leaf.integer_constant = value;
    return ast;
}

AbstractSyntacticTree* ast_const_float(double value)
{
    AbstractSyntacticTree* ast = ast_init();
    ast->operation = CONST_REAL;
    ast->value.leaf.integer_constant = value;
    return ast;
}

AbstractSyntacticTree* ast_var_name(const char* var_name)
{
    AbstractSyntacticTree* ast = ast_init();
    ast->operation = VAR_NAME;
    ast->value.leaf.str = var_name;
    return ast;
}

AbstractSyntacticTree* ast_assignment_expr(
    const char* identifier, AbstractSyntacticTree* right)
{
    AbstractSyntacticTree* ast = ast_init();
    ast->operation = ASSIGN_EXPR;
    ast->value.interior.left = ast_init();
    ast->value.interior.left->operation = UNDEFINED;
    ast->value.interior.left->value.leaf.str = identifier;
    ast->value.interior.right = right;
    return ast;
}
