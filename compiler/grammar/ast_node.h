#ifndef _AST_NODE_H
#define _AST_NODE_H

#include "../collections/linkedList.h"

#include <llvm-c/Core.h>

typedef enum
{
    UNDEFINED = 0,
    FUNC_DEF,
    CONST_INT,
    CONST_REAL,
    CONST_BOOL,
    ASSIGN_EXPR,
    VAR_NAME,
    RET_EXPR,
    PLUS,
    MINUS,
    MUL,
    DIV,
} Operator;

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

struct tree_node;

typedef struct
{
    const char* type;
    const char* name;
} Parameter;


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
    Operator operation;
    int mode;
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

AbstractSyntacticTree* ast_init();

AbstractSyntacticTree* ast_function_definition(const char* func_name,
    const char* ret_type, LinkedList* parameters, LinkedList* body);
void function_node_destroy(FunctionNode* func);

AbstractSyntacticTree* ast_const_bool(const char* value);
AbstractSyntacticTree* ast_const_int(int value);
AbstractSyntacticTree* ast_const_float(double value);
AbstractSyntacticTree* ast_var_name(const char* var_name);

AbstractSyntacticTree* ast_assignment_expr(
    const char* identifier, AbstractSyntacticTree* right);

#endif
