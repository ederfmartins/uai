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

typedef Assembler_str* Assembler;

// Control functions
Assembler assembler_init(const char* module_name);
int assembler_dump_bytecode(Assembler assembler, const char* out_name);
void assembler_destroy(Assembler assembler);

// Code generation
void assembler_generate_functions(Assembler assembler, LinkedList* func_list);
void assembler_generate_function(Assembler assembler, FunctionNode* func_def);
void assembler_declare_function(Assembler assembler, FunctionNode* func_def);

#endif
