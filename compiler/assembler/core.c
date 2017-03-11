#include "core.h"
#include "../grammar/ast_node.h"
#include "../collections/str_hash.h"

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// typedef struct
// {
//     char* mod_name;
//     LLVMModuleRef mod;
//     LLVMBuilderRef builder;
//     ptr_hash_t globals;
//     ptr_hash_t locals;
// } Assembler_str;

LLVMValueRef _assembler_create_constant(Assembler assembler, const char* name, const char* value)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    LLVMValueRef c1 = LLVMConstString(value, strlen(value) + 1, 1);
    LLVMTypeRef str_array = LLVMArrayType(LLVMInt8Type(), strlen(value) + 1);
    LLVMValueRef c = LLVMAddGlobal(ptr->mod, str_array, name);
    LLVMSetGlobalConstant(c, 1);
    LLVMSetInitializer(c, c1);
    LLVMSetLinkage(c, LLVMPrivateLinkage);
    LLVMSetUnnamedAddr(c, 1);
    return c;
}

char* _get_module_name(const char* fname)
{
    int l = strlen(fname);
    int i;
    for (i = l - 1; i >= 0; i--) {
        if (fname[i] == '/')
            break;
    }
    if (i < 0) i = 0;
    if (fname[i] == '/') i++;
    return strdup(&fname[i]);
}

void* _assembler_lookup(Assembler assembler, const char* identifier)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    if (hash_contains_key(ptr->locals, (void*) identifier))
        return hash_get(ptr->locals, (void*) identifier);
    else return hash_get(ptr->globals, (void*) identifier);
}

Assembler assembler_init(const char* module_name)
{
    Assembler_str* assembler = (Assembler_str*) malloc(sizeof(Assembler_str));
    assembler->mod_name = _get_module_name(module_name);

    assembler->mod = LLVMModuleCreateWithName(assembler->mod_name);
    assembler->builder = LLVMCreateBuilder();
    
    assembler->globals = hash_create(100, 0.75, (_COMPARE*) str_compare, (_HASH_CODE*) str_hash_code, NULL, NULL, NULL, NULL);
    assembler->locals = hash_create(100, 0.75, (_COMPARE*) str_compare, (_HASH_CODE*) str_hash_code, NULL, NULL, NULL, NULL);

    return (Assembler) assembler;
}

void assembler_destroy(Assembler assembler)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    hash_destroy(ptr->globals);
    hash_destroy(ptr->locals);
    LLVMDisposeBuilder(ptr->builder);
    free(ptr->mod_name);
    free(ptr);
}

void assembler_declare_printf(Assembler assembler)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    LLVMValueRef d = _assembler_create_constant(assembler, ".print_arg_d", "%d\n");
    hash_put(ptr->globals, (void*)".print_arg_d", d);
    LLVMValueRef f = _assembler_create_constant(assembler, ".print_arg_f", "%f\n");
    hash_put(ptr->globals, (void*)".print_arg_f", f);

    LLVMTypeRef param_types[] = {LLVMPointerType(LLVMInt8Type(), 0)};
    LLVMTypeRef printf_type = LLVMFunctionType(LLVMInt32Type(), param_types, 1, 1);
    LLVMValueRef print_func = LLVMAddFunction(ptr->mod, "printf", printf_type);
    LLVMAddAttribute(LLVMGetFirstParam(print_func), LLVMNoAliasAttribute);
    hash_put(ptr->globals, (void*)"print", print_func);
}

int assembler_dump_bytecode(Assembler assembler, const char* out_name)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    char *error = NULL;
    LLVMVerifyModule(ptr->mod, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);
    return LLVMWriteBitcodeToFile(ptr->mod, out_name);
}

Node exec_op(Assembler a, Node n1, Node n2, char op)
{
    Assembler_str* ptr = (Assembler_str*) a;
    Node ret;
    LLVMValueRef llvm;
    int is_double = LLVMTypeOf(n1.llvm) == LLVMDoubleType();

    switch (op) {
        case '+':
            if (is_double) llvm = LLVMBuildFAdd(ptr->builder, n1.llvm, n2.llvm, "tmp");
            else llvm = LLVMBuildAdd(ptr->builder, n1.llvm, n2.llvm, "tmp");
            break;
        case '-':
            if (is_double) llvm = LLVMBuildFSub(ptr->builder, n1.llvm, n2.llvm, "tmp");
            else llvm = LLVMBuildSub(ptr->builder, n1.llvm, n2.llvm, "tmp");
            break;
        case '*':
            if (is_double) llvm = LLVMBuildFMul(ptr->builder, n1.llvm, n2.llvm, "tmp");
            else llvm = LLVMBuildMul(ptr->builder, n1.llvm, n2.llvm, "tmp");
            break;
        case '/':
            if (is_double) llvm = LLVMBuildFDiv(ptr->builder, n1.llvm, n2.llvm, "tmp");
            else llvm = LLVMBuildUDiv(ptr->builder, n1.llvm, n2.llvm, "tmp");
            break;
        case '%':
            llvm = LLVMBuildSDiv(ptr->builder, n1.llvm, n2.llvm, "tmp");
            break;
        case '>':
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, n1.llvm, n2.llvm, "tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntUGT, n1.llvm, n2.llvm, "tmp");
            break;
        case '<':
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, n1.llvm, n2.llvm, "tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntSLT, n1.llvm, n2.llvm, "tmp");
            break;
        case 'g':
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, n1.llvm, n2.llvm, "tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntUGE, n1.llvm, n2.llvm, "tmp");
            break;
        case 'l':
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, n1.llvm, n2.llvm, "tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntSLE, n1.llvm, n2.llvm, "tmp");
            break;
        case '=':
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, n1.llvm, n2.llvm, "tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntEQ, n1.llvm, n2.llvm, "tmp");
            break;
        case '!':
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, n1.llvm, n2.llvm, "tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntNE, n1.llvm, n2.llvm, "tmp");
            break;
    }

    ret = node_merge(n1, n2);
    node_add_instruction(&ret, llvm);
    return ret;
}

Node assembler_produce_print(Assembler assembler, Node expr)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    LLVMValueRef n = expr.llvm;
    LLVMValueRef arg;

    if (LLVMTypeOf(expr.llvm) == LLVMDoubleType()) {
        arg = hash_get(ptr->globals, (void*) ".print_arg_f");
    }
    else {
        arg = hash_get(ptr->globals, (void*) ".print_arg_d");
    }

    arg = LLVMConstPointerCast(arg, LLVMPointerType(LLVMInt8Type(), 0));
    LLVMValueRef args[] = {arg, n};
    LLVMValueRef print = hash_get(ptr->globals, (void*) "print");
    LLVMValueRef call_func = LLVMBuildCall(ptr->builder, print, args, 2, "print");
    node_add_instruction(&expr, call_func);

    return expr;
}

Node assembler_const_bool(Assembler assembler, const char* value)
{
    Node node = node_init();
    int v = 0;
    if (strcmp(value, "true")) v = 1;
    node_add_instruction(&node, LLVMConstInt(LLVMInt1Type(), v, 0));
    return node;
}

Node assembler_const_int(Assembler assembler, int value)
{
    Node node = node_init();
    node_add_instruction(&node, LLVMConstInt(LLVMInt32Type(), value, 0));
    return node;
}

Node assembler_const_float(Assembler assembler, double value)
{
    Node node = node_init();
    node_add_instruction(&node, LLVMConstReal(LLVMDoubleType(), value));
    return node;
}

int assembler_is_global_defined(Assembler assembler, const char* identifier)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    return hash_contains_key(ptr->globals, (void*) identifier);
}

int assembler_is_defined(Assembler assembler, const char* identifier)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    return hash_contains_key(ptr->globals, (void*) identifier) || hash_contains_key(ptr->locals, (void*) identifier);
}

Node assembler_produce_load_variable(Assembler assembler, const char* varName)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    Node node = node_init();
    LLVMValueRef val = (LLVMValueRef) _assembler_lookup(ptr, varName);
    node_add_instruction(&node, LLVMBuildLoad(ptr->builder, val, "tmp"));
    return node;
}

Node assembler_produce_store_variable(Assembler assembler, const char* varName, Node expr)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    if (! assembler_is_defined(assembler, varName)) {
        LLVMValueRef var = LLVMBuildAlloca(ptr->builder, LLVMTypeOf(expr.llvm), varName);
        hash_put(ptr->locals, (void*) varName, var);
        ll_insert(&(expr.instructions_list), var);
    }
    LLVMValueRef var = _assembler_lookup(assembler, varName);
    ll_insert(&expr.instructions_list, LLVMBuildStore(ptr->builder, expr.llvm, var));
    return expr;
}

Node assembler_produce_ret_void(Assembler assembler)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    Node node = node_init();
    LLVMValueRef ret_type = LLVMBuildRetVoid(ptr->builder);
    node_add_instruction(&node, ret_type);
    node.return_type = LLVMVoidType();
    node.return_type_valid = 1;
    return node;
}

Node assembler_produce_return(Assembler assembler, Node expr)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    expr.return_type_valid = 1;
    expr.return_type = LLVMTypeOf(expr.llvm);
    LLVMValueRef ret_inst = LLVMBuildRet(ptr->builder, expr.llvm);
    node_add_instruction(&expr, ret_inst);
    return expr;
}

void assembler_produce_function(Assembler assembler, const char* func_name, Node statement_list, LinkedList param_list)
{
    Assembler_str* ptr = (Assembler_str*) assembler;

    if (!statement_list.return_type_valid) {
        node_add_instruction(&statement_list, LLVMBuildRetVoid(ptr->builder));
        statement_list.return_type = LLVMVoidType();
    }

    LLVMTypeRef param_types[200];

    NodeList* cur_node = ll_iter_begin(&param_list);
    for (int i = 0; i < ll_size(&param_list); i++)
    {
        param_types[i] = ((Parameter*) nl_getValue(cur_node))->type;
        cur_node = ll_iter_next(cur_node);
    }

    LLVMTypeRef ret_type = LLVMFunctionType(statement_list.return_type, param_types, ll_size(&param_list), 0);
    LLVMValueRef func = LLVMAddFunction(ptr->mod, func_name, ret_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
    LLVMBuilderRef curr_builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(curr_builder, entry);

    LinkedList instructions = statement_list.instructions_list;
    // printf("%s instructions %d\n", func_name, ll_size(&instructions));
    for (NodeList* node = ll_iter_begin(&instructions);
        node != ll_iter_end(&instructions);
        node = ll_iter_next(node))
    {
        LLVMValueRef inst = nl_getValue(node);
        //printf("...%s\n", LLVMPrintValueToString(inst));
        if (LLVMIsConstant(inst))
            continue;
        LLVMInsertIntoBuilder(curr_builder, inst);
    }

    node_destroy(&statement_list);
    LLVMDisposeBuilder(curr_builder);
    hash_put(ptr->globals, (void*) func_name, func);

    // Clear local variables
    hash_destroy(ptr->locals);
    ptr->locals = hash_create(100, 0.75, (_COMPARE*) str_compare, (_HASH_CODE*) str_hash_code, NULL, NULL, NULL, NULL);
}
