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

typedef struct
{
    char* mod_name;
    LLVMModuleRef mod;
    LLVMBuilderRef builder;
    ptr_hash_t globals;
    ptr_hash_t locals;
} Assembler_str;

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
    Node ret;
    Assembler_str* ptr = (Assembler_str*) a;
    ret.instructions_list = n1.instructions_list;
    ll_transfer(&ret.instructions_list, &n2.instructions_list);
    ll_destroy(&n2.instructions_list);
    int is_double = LLVMTypeOf(n1.llvm) == LLVMDoubleType();
    switch (op) {
        case '+':
            if (is_double) ret.llvm = LLVMBuildFAdd(ptr->builder, n1.llvm, n2.llvm, "tmp");
            else ret.llvm = LLVMBuildAdd(ptr->builder, n1.llvm, n2.llvm, "tmp");
            break;
        case '-':
            if (is_double) ret.llvm = LLVMBuildFSub(ptr->builder, n1.llvm, n2.llvm, "tmp");
            else ret.llvm = LLVMBuildSub(ptr->builder, n1.llvm, n2.llvm, "tmp");
            break;
        case '*':
            if (is_double) ret.llvm = LLVMBuildFMul(ptr->builder, n1.llvm, n2.llvm, "tmp");
            else ret.llvm = LLVMBuildMul(ptr->builder, n1.llvm, n2.llvm, "tmp");
            break;
        case '/':
            if (is_double) ret.llvm = LLVMBuildFDiv(ptr->builder, n1.llvm, n2.llvm, "tmp");
            else ret.llvm = LLVMBuildUDiv(ptr->builder, n1.llvm, n2.llvm, "tmp");
            break;
        case '%':
            ret.llvm = LLVMBuildSDiv(ptr->builder, n1.llvm, n2.llvm, "tmp");
            break;
        case '>':
            if (is_double) ret.llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, n1.llvm, n2.llvm, "tmp");
            else ret.llvm = LLVMBuildICmp (ptr->builder, LLVMIntUGT, n1.llvm, n2.llvm, "tmp");
            break;
        case '<':
            if (is_double) ret.llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, n1.llvm, n2.llvm, "tmp");
            else ret.llvm = LLVMBuildICmp (ptr->builder, LLVMIntSLT, n1.llvm, n2.llvm, "tmp");
            break;
        case 'g':
            if (is_double) ret.llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, n1.llvm, n2.llvm, "tmp");
            else ret.llvm = LLVMBuildICmp (ptr->builder, LLVMIntUGE, n1.llvm, n2.llvm, "tmp");
            break;
        case 'l':
            if (is_double) ret.llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, n1.llvm, n2.llvm, "tmp");
            else ret.llvm = LLVMBuildICmp (ptr->builder, LLVMIntSLE, n1.llvm, n2.llvm, "tmp");
            break;
        case '=':
            if (is_double) ret.llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, n1.llvm, n2.llvm, "tmp");
            else ret.llvm = LLVMBuildICmp (ptr->builder, LLVMIntEQ, n1.llvm, n2.llvm, "tmp");
            break;
        case '!':
            if (is_double) ret.llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, n1.llvm, n2.llvm, "tmp");
            else ret.llvm = LLVMBuildICmp (ptr->builder, LLVMIntNE, n1.llvm, n2.llvm, "tmp");
            break;
    }
    ll_insert(&ret.instructions_list, ret.llvm);
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
    LLVMValueRef call_fac = LLVMBuildCall(ptr->builder, print, args, 2, "print");
    expr.llvm = call_fac;
    ll_insert(&expr.instructions_list, expr.llvm);
    return expr;
}

Node assembler_const_bool(Assembler assembler, const char* value)
{
    Node node;
    int v = 0;
    if (strcmp(value, "true")) v = 1;
    node.llvm = LLVMConstInt(LLVMInt1Type(), v, 0);
    ll_init(&node.instructions_list);
    return node;
}

Node assembler_const_int(Assembler assembler, int value)
{
    Node node;
    node.llvm = LLVMConstInt(LLVMInt32Type(), value, 0);
    ll_init(&node.instructions_list);
    return node;
}

Node assembler_const_float(Assembler assembler, double value)
{
    Node node;
    node.llvm = LLVMConstReal(LLVMDoubleType(), value);
    ll_init(&node.instructions_list);
    return node;
}

int assembler_is_defined(Assembler assembler, const char* identifier)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    return hash_get(ptr->globals, (void*) identifier) != NULL;
}

Node assembler_produce_load_variable(Assembler assembler, const char* varName)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    Node node;
    LLVMValueRef val = (LLVMValueRef) hash_get(ptr->globals, (void*) varName);
    node.llvm = LLVMBuildLoad(ptr->builder, val, varName);
    ll_init(&node.instructions_list);
    ll_insert(&node.instructions_list, node.llvm);
    return node;
}

Node assembler_produce_store_variable(Assembler assembler, const char* varName, Node expr)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    if (! assembler_is_defined(assembler, varName)) {
        LLVMValueRef var = LLVMBuildAlloca(ptr->builder, LLVMTypeOf(expr.llvm), varName);
        hash_put(ptr->globals, (void*) varName, var);
        ll_insert(&(expr.instructions_list), var);
    }
    LLVMValueRef var = hash_get(ptr->globals, (void*) varName);
    ll_insert(&expr.instructions_list, LLVMBuildStore(ptr->builder, expr.llvm, var));
    return expr;
}

void assembler_produce_main(Assembler assembler, Node statement_list)
{
    Assembler_str* ptr = (Assembler_str*) assembler;

    LLVMTypeRef param_types[1];
    LLVMTypeRef ret_type = LLVMFunctionType(LLVMVoidType(), param_types, 0, 0);
    LLVMValueRef main_func = LLVMAddFunction(ptr->mod, "main", ret_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main_func, "entry");
    LLVMBuilderRef curr_builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(curr_builder, entry);

    LinkedList instructions = statement_list.instructions_list;
    for (NodeList* node = ll_iter_begin(&instructions);
        node != ll_iter_end(&instructions);
        node = ll_iter_next(node))
    {
        LLVMValueRef inst = nl_getValue(node);
        if (LLVMIsConstant(inst))
            continue;
        //printf("...%s\n", LLVMPrintValueToString(inst));
        LLVMInsertIntoBuilder(curr_builder, inst);
    }
    LLVMBuildRetVoid(curr_builder);
    ll_destroy(&instructions);
}

void assembler_produce_function(Assembler assembler, const char* func_name, Node statement_list)
{
    Assembler_str* ptr = (Assembler_str*) assembler;

    LLVMTypeRef param_types[1];
    LLVMTypeRef ret_type = LLVMFunctionType(LLVMVoidType(), param_types, 0, 0);
    LLVMValueRef main_func = LLVMAddFunction(ptr->mod, func_name, ret_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main_func, "entry");
    LLVMBuilderRef curr_builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(curr_builder, entry);

    LinkedList instructions = statement_list.instructions_list;
    for (NodeList* node = ll_iter_begin(&instructions);
        node != ll_iter_end(&instructions);
        node = ll_iter_next(node))
    {
        LLVMValueRef inst = nl_getValue(node);
        if (LLVMIsConstant(inst))
            continue;
        //printf("...%s\n", LLVMPrintValueToString(inst));
        LLVMInsertIntoBuilder(curr_builder, inst);
    }
    LLVMBuildRetVoid(curr_builder);
    ll_destroy(&instructions);
}
