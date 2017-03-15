#include "../grammar/ast_node.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ast_if_destroy(AbstractSyntacticTree* ast, int free_ast);

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

void function_node_destroy(FunctionNode* func, int free_params)
{
    /*
    TODO: verify the ownership off there variables
    const char* func_name;
    const char* ret_type;
    */
    //printf("Destroy %s %d\n", func->func_name, ll_size(&func->body));
    for (NodeList* node = ll_iter_begin(&func->body);
        node != ll_iter_end(&func->body);
        node = ll_iter_next(node))
    {
        ast_destroy((AbstractSyntacticTree*) nl_getValue(node));
    }
    if (free_params)
    {
        for (NodeList* node = ll_iter_begin(&func->parameters);
        node != ll_iter_end(&func->parameters);
        node = ll_iter_next(node))
        {
            ast_destroy((AbstractSyntacticTree*) nl_getValue(node));
        }
    }

    ll_destroy(&func->parameters);
    ll_destroy(&func->body);
}

void ast_destroy(AbstractSyntacticTree* ast)
{
    if (ast == NULL) return;
    //printf("Destroy... %d\n", ast->production);
    switch (ast->production)
    {
        case FUNC_DEF:
            function_node_destroy(&ast->value.func_def, 0);
        break;
        case FUNC_CALL:
            function_node_destroy(&ast->value.func_def, 1);
        break;
        case ASSIGN_EXPR:
            free(ast->value.interior.left);
            ast_destroy(ast->value.interior.right);
            break;
        case IF_STM:
            ast_if_destroy(ast, 0);
        break;
        case PRINT_STM:
        case RET_EXPR:
        case B_EXPR:
            ast_destroy(ast->value.interior.right);
            ast_destroy(ast->value.interior.left);
        break;
        case CONST_INT:
        case CONST_REAL:
        case CONST_BOOL:
        case VAR_NAME:
        break;
        default:
            printf("ast_destroy: Not implemented yet!\n");
            exit(-1);
    }

    free(ast);
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
    ll_init(&ast->value.func_def.body);
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

AbstractSyntacticTree* ast_const_float(double value)
{
    AbstractSyntacticTree* ast = ast_init();
    ast->production = CONST_REAL;
    ast->value.leaf.double_constant = value;
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

AbstractSyntacticTree* ast_if(AbstractSyntacticTree* cond,
    LinkedList then, LinkedList elsee)
{
    AbstractSyntacticTree* ast = ast_init();
    ast->production = IF_STM;
    ast->value.tnode.cond = cond;
    ast->value.tnode.left = then;
    ast->value.tnode.right = elsee;
    return ast;
}

void ast_if_destroy(AbstractSyntacticTree* ast, int free_ast)
{
    for (NodeList* node = ll_iter_begin(&ast->value.tnode.left);
        node != ll_iter_end(&ast->value.tnode.left);
        node = ll_iter_next(node))
    {
        ast_destroy((AbstractSyntacticTree*) nl_getValue(node));
    }

    for (NodeList* node = ll_iter_begin(&ast->value.tnode.right);
        node != ll_iter_end(&ast->value.tnode.right);
        node = ll_iter_next(node))
    {
        ast_destroy((AbstractSyntacticTree*) nl_getValue(node));
    }

    ll_destroy(&ast->value.tnode.right);
    ll_destroy(&ast->value.tnode.right);
    ast_destroy(ast->value.tnode.cond);

    if (free_ast) free(ast);
}
