#ifndef _ASSEMBLER_CORE_H_
#define _ASSEMBLER_CORE_H_

#include "../grammar/ast_node.h"

typedef void* Assembler;

Assembler assembler_init(const char* module_name);
int assembler_dump_bytecode(Assembler assembler, const char* out_name);
void assembler_destroy(Assembler assembler);

void assembler_declare_printf(Assembler assembler);
int assembler_is_defined(Assembler assembler, const char* identifier);

Node exec_op(Assembler a, Node n1, Node n2, char op);
Node assembler_produce_print(Assembler assembler, Node expr);
Node assembler_produce_load_variable(Assembler assembler, const char* varName);
Node assembler_produce_store_variable(Assembler assembler, const char* varName, Node expr);
void assembler_produce_main(Assembler assembler, Node statement_list);
void assembler_produce_function(Assembler assembler, const char* func_name, Node statement_list);

Node assembler_const_bool(Assembler assembler, const char* value);
Node assembler_const_int(Assembler assembler, int value);
Node assembler_const_float(Assembler assembler, double value);


#endif
