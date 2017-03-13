#ifndef _AST_NODE_H
#define _AST_NODE_H

#include "../collections/linkedList.h"

#include <llvm-c/Core.h>

typedef enum
{
    UNDEFINED = 0,
    FUNC_DEF,
    FUNC_CALL,
    CONST_INT,
    CONST_REAL,
    CONST_BOOL,
    ASSIGN_EXPR,
    PRINT_STM,
    VAR_NAME,
    RET_EXPR,
    B_EXPR
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
    BinaryOperator operation;
    union
    {
        struct
        {
            struct tree_node* left;
            struct tree_node* right;  /* NULL if unary operator */
        } interior;

        struct {
            const char* str;
            int integer_constant;
            double double_constant;
            LLVMValueRef value_ref;
        } leaf;

        FunctionNode func_def;

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
AbstractSyntacticTree* ast_assignment_expr(
    const char* identifier, AbstractSyntacticTree* right);

// Leaf nodes
AbstractSyntacticTree* ast_const_bool(const char* value);
AbstractSyntacticTree* ast_const_int(int value);
AbstractSyntacticTree* ast_const_float(double value);
AbstractSyntacticTree* ast_var_name(const char* var_name);

#endif
