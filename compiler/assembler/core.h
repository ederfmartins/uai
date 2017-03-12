#ifndef _ASSEMBLER_CORE_H_
#define _ASSEMBLER_CORE_H_

#include "../grammar/ast_node.h"
#include "../collections/hash.h"

typedef struct
{
    char* mod_name;
    LLVMModuleRef mod;
    LLVMBuilderRef builder;
    ptr_hash_t globals;
    ptr_hash_t locals;
} Assembler_str;

typedef void* Assembler;

Assembler assembler_init(const char* module_name);
int assembler_dump_bytecode(Assembler assembler, const char* out_name);
void assembler_destroy(Assembler assembler);

void assembler_generate_functions(Assembler assembler, LinkedList* func_list);
void assembler_generate_function(Assembler assembler, FunctionNode* func_def);
void assembler_declare_function(Assembler assembler, FunctionNode* func_def);

void assembler_declare_builtin(Assembler assembler);
int assembler_is_global_defined(Assembler assembler, const char* identifier);
int assembler_is_defined(Assembler assembler, const char* identifier);

Node exec_op(Assembler a, Node n1, Node n2, char op);
Node assembler_produce_print(Assembler assembler, Node expr);
Node assembler_produce_store_variable(Assembler assembler, const char* varName, Node expr);
void assembler_produce_function(Assembler assembler, const char* func_name, Node statement_list, LinkedList param_list);
Node assembler_produce_ret_void(Assembler assembler);
Node assembler_produce_return(Assembler assembler, Node expr);

#endif
