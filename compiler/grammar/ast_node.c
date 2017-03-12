#include "../grammar/ast_node.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AbstractSyntacticTree* ast_init()
{
   AbstractSyntacticTree* t = (AbstractSyntacticTree*) malloc (sizeof(AbstractSyntacticTree));
   t->production = UNDEFINED;
   t->operation = 0;
   // this can be dangerous :(
   // t->value.interior.left = (AbstractSyntacticTree*) NULL;
   // t->value.interior.right = (AbstractSyntacticTree*) NULL;
   return t;
}

AbstractSyntacticTree* ast_interior(Production op,
    AbstractSyntacticTree* left, AbstractSyntacticTree* right)
{
    AbstractSyntacticTree* ast = ast_init();
    ast->production = op;
    ast->value.interior.left = left;
    ast->value.interior.right = right;
    return ast;
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

AbstractSyntacticTree* binary_expr(BinaryOperator op,
    AbstractSyntacticTree* left, AbstractSyntacticTree* right)
{
    AbstractSyntacticTree* ast = ast_interior(B_EXPR, left, right);
    ast->operation = op;
    return ast;
}

AbstractSyntacticTree* ast_function_definition(const char* func_name,
    const char* ret_type, LinkedList* parameters, LinkedList* body)
{
    AbstractSyntacticTree* ast = ast_init();
    ast->production = FUNC_DEF;
    ast->value.func_def.func_name = func_name;
    ast->value.func_def.ret_type = ret_type;
    ast->value.func_def.is_vararg = 0;

    if (parameters) ast->value.func_def.parameters = *parameters;
    else ll_init(&ast->value.func_def.parameters);
    
    if (body) ast->value.func_def.body = *body;
    else ll_init(&ast->value.func_def.body);
    return ast;
}

AbstractSyntacticTree* ast_function_call(
    const char* func_name, LinkedList* parameters)
{
    AbstractSyntacticTree* ast = ast_init();
    ast->production = FUNC_CALL;
    ast->value.func_def.func_name = func_name;
    ast->value.func_def.ret_type = NULL;
    if (parameters) ast->value.func_def.parameters = *parameters;
    else ll_init(&ast->value.func_def.parameters);
    return ast;
}

AbstractSyntacticTree* ast_const_bool(const char* value)
{
    AbstractSyntacticTree* ast = ast_init();
    ast->production = CONST_BOOL;
    if (strcmp(value, "true")) ast->value.leaf.integer_constant = 1;
    else ast->value.leaf.integer_constant = 0;
    return ast;
}

AbstractSyntacticTree* ast_const_int(int value)
{
    AbstractSyntacticTree* ast = ast_init();
    ast->production = CONST_INT;
    ast->value.leaf.integer_constant = value;
    return ast;
}

AbstractSyntacticTree* ast_return(AbstractSyntacticTree* expr)
{
    return ast_interior(RET_EXPR, expr, NULL);
}

AbstractSyntacticTree* ast_print(AbstractSyntacticTree* expr)
{
    return ast_interior(PRINT_STM, expr, NULL);
}

AbstractSyntacticTree* ast_const_float(double value)
{
    AbstractSyntacticTree* ast = ast_init();
    ast->production = CONST_REAL;
    ast->value.leaf.integer_constant = value;
    return ast;
}

AbstractSyntacticTree* ast_var_name(const char* var_name)
{
    AbstractSyntacticTree* ast = ast_init();
    ast->production = VAR_NAME;
    ast->value.leaf.str = var_name;
    return ast;
}

AbstractSyntacticTree* ast_assignment_expr(
    const char* identifier, AbstractSyntacticTree* right)
{
    AbstractSyntacticTree* left = ast_init();
    left->production = UNDEFINED;
    left->value.leaf.str = identifier;
    return ast_interior(ASSIGN_EXPR, left, right);
}
