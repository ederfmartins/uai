#ifndef _AST_NODE_H
#define _AST_NODE_H

#include "../collections/linkedList.h"

#include <llvm-c/Core.h>

typedef enum
{
    UNDEFINED = 0,
    FUNC_DEF,
    FUNC_CALL,
    CONST_INT, // 3
    CONST_REAL,
    CONST_BOOL, //5
    CONST_ARRAY,
    ARRAY_GET,
    ASSIGN_EXPR,
    PRINT_STM,
    IF_STM, // 10
    FOR_STM,
    VAR_NAME,
    RET_EXPR,
    B_EXPR, // 14
    UN_EXPR
} Production;

typedef enum
{
    PLUS = 1,
    MINUS,
    MUL,
    DIV,
    MOD,
    LT,
    GT,
    LTE,
    GTE,
    EQ,
    NEQ
} BinaryOperator;

typedef enum
{
    U_GET_ADRESS
} UnaryOperator;

struct tree_node;

typedef struct
{
    const char* func_name;
    const char* ret_type;
    LinkedList parameters;
    LinkedList body;
    int is_vararg;
} FunctionNode;

typedef struct tree_node
{
    Production production;
    int operation;
    union
    {
        struct
        {
            struct tree_node* left;
            struct tree_node* right;  /* NULL if unary operator */
        } interior;

        struct
        {
            struct tree_node* cond;
            LinkedList left;
            LinkedList right;
        } tnode;

        struct
        {
            struct tree_node* init;
            struct tree_node* cond;
            struct tree_node* inc;
            LinkedList body;
        } for_node;

        struct {
            const char* str;
            int integer_constant;
            double double_constant;
        } leaf;

        FunctionNode func_def;
        LinkedList stm_list;

    } value;
} AbstractSyntacticTree;

typedef struct
{
    const char* type;
    const char* name;
} Parameter;


AbstractSyntacticTree* ast_init();
void ast_destroy(AbstractSyntacticTree* ast);

// Top productions
AbstractSyntacticTree* binary_expr(BinaryOperator op,
    AbstractSyntacticTree* left, AbstractSyntacticTree* right);
AbstractSyntacticTree* ast_function_definition(const char* func_name,
    const char* ret_type, LinkedList* parameters, LinkedList* body);
AbstractSyntacticTree* ast_function_call(
    const char* func_name, LinkedList* parameters);

// Statements
AbstractSyntacticTree* ast_return(AbstractSyntacticTree* expr);
AbstractSyntacticTree* ast_print(AbstractSyntacticTree* expr);
AbstractSyntacticTree* ast_decl_assign_expr(
    const char* identifier, AbstractSyntacticTree* right);
AbstractSyntacticTree* ast_assign_expr(
    AbstractSyntacticTree* left, AbstractSyntacticTree* right);
AbstractSyntacticTree* ast_if(AbstractSyntacticTree* cond,
    LinkedList then, LinkedList elsee);
AbstractSyntacticTree* ast_for(AbstractSyntacticTree* init,
    AbstractSyntacticTree* cond, AbstractSyntacticTree* inc, LinkedList body);
AbstractSyntacticTree* ast_array_get(const char* var_name, AbstractSyntacticTree* idx);
AbstractSyntacticTree* ast_get_pointer(AbstractSyntacticTree* expr);

// Leaf nodes
AbstractSyntacticTree* ast_const_bool(const char* value);
AbstractSyntacticTree* ast_const_int(int value);
AbstractSyntacticTree* ast_const_float(double value);
AbstractSyntacticTree* ast_var_name(const char* var_name);
AbstractSyntacticTree* ast_const_array(LinkedList* elements);

#endif
