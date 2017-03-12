#include "core.h"
#include "../collections/str_hash.h"

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yylineno;

LLVMValueRef _assembler_create_gconst_str(Assembler assembler, const char* name, const char* value)
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

void assembler_declare_printf(Assembler assembler)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    LLVMValueRef d = _assembler_create_gconst_str(assembler, ".print_arg_d", "%d\n");
    hash_put(ptr->globals, (void*)".print_arg_d", d);
    LLVMValueRef f = _assembler_create_gconst_str(assembler, ".print_arg_f", "%f\n");
    hash_put(ptr->globals, (void*)".print_arg_f", f);

    LLVMTypeRef param_types[] = {LLVMPointerType(LLVMInt8Type(), 0)};
    LLVMTypeRef printf_type = LLVMFunctionType(LLVMInt32Type(), param_types, 1, 1);
    LLVMValueRef print_func = LLVMAddFunction(ptr->mod, "printf", printf_type);
    LLVMAddAttribute(LLVMGetFirstParam(print_func), LLVMNoAliasAttribute);
    hash_put(ptr->globals, (void*)"print", print_func);
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

void assembler_declare_builtin(Assembler assembler)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    assembler_declare_printf(assembler);
    hash_put(ptr->globals, (void*) "int", LLVMInt32Type());
    hash_put(ptr->globals, (void*) "boolean", LLVMInt1Type());
    hash_put(ptr->globals, (void*) "double", LLVMDoubleType());
    hash_put(ptr->globals, (void*) ".void", LLVMVoidType());
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

void assembler_generate_functions(Assembler assembler, LinkedList* func_list)
{
    for (NodeList* node = ll_iter_begin(func_list);
        node != ll_iter_end(func_list);
        node = ll_iter_next(node))
    {
        AbstractSyntacticTree* func = nl_getValue(node);
        if (func->operation != FUNC_DEF) {
            printf("Invalid parameter assembler_generate_functions called with a ast of type %d\n", func->operation);
            exit(-1);
        }
        assembler_declare_function(assembler, &func->value.func_def);
    }

    for (NodeList* node = ll_iter_begin(func_list);
        node != ll_iter_end(func_list);
        node = ll_iter_next(node))
    {
        AbstractSyntacticTree* func = nl_getValue(node);
        assembler_generate_function(assembler, &func->value.func_def);
        function_node_destroy(&func->value.func_def);
    }

    ll_destroy(func_list);
}

void assembler_declare_function(Assembler assembler, FunctionNode* func_def)
{
    Assembler_str* ptr = (Assembler_str*) assembler;

    int num_parans = ll_size(&func_def->parameters) + 1;
    LLVMTypeRef* args = (LLVMTypeRef*) malloc(sizeof(LLVMTypeRef) * num_parans);
    num_parans = 0;

    for (NodeList* cur_node = ll_iter_begin(&func_def->parameters);
        cur_node != ll_iter_end(&func_def->parameters);
        cur_node = ll_iter_next(cur_node))
    {
        Parameter* param = nl_getValue(cur_node);

        if (! hash_contains_key(ptr->globals, (void*) param->type))
        {
            printf("%s was not declared\n", param->type);
            exit(-1);
        }

        args[num_parans] = hash_get(ptr->globals, (void*) param->type);
        num_parans++;
    }

    if (! hash_contains_key(ptr->globals, (void*) func_def->ret_type))
    {
        printf("%s was not declared\n", func_def->ret_type);
        exit(-1);
    }

    LLVMTypeRef ret_type = hash_get(ptr->globals, (void*) func_def->ret_type);
    LLVMTypeRef ftype = LLVMFunctionType(ret_type, args, num_parans, func_def->is_vararg);
    LLVMValueRef func = LLVMAddFunction(ptr->mod, func_def->func_name, ftype);
    hash_put(ptr->globals, (void*) func_def->func_name, func);

    free(args);
}

void _add_params_to_locals(Assembler_str* ptr, LLVMValueRef func, LinkedList* params)
{
    int num_parans = ll_size(params) + 1;
    LLVMValueRef* args = (LLVMValueRef*) malloc(sizeof(LLVMValueRef) * num_parans);
    num_parans = 0;
    LLVMGetParams(func, args);

    for (NodeList* node = ll_iter_begin(params);
        node != ll_iter_end(params);
        node = ll_iter_next(node))
    {
        Parameter* param = nl_getValue(node);
        LLVMTypeRef type = LLVMTypeOf(args[num_parans]);
        LLVMValueRef var = LLVMBuildAlloca(ptr->builder, type, ".param");
        LLVMBuildStore(ptr->builder, args[num_parans], var);
        hash_put(ptr->locals, (void*) param->name, var);
        num_parans++;
    }

    free(args);
}

LLVMValueRef _generate_primary_expr(Assembler_str* ptr, AbstractSyntacticTree* expr)
{
    switch (expr->operation)
    {
        case CONST_INT:
            return LLVMConstInt(LLVMInt32Type(), expr->value.leaf.integer_constant, 0);
        break;
        case CONST_REAL:
            return LLVMConstReal(LLVMDoubleType(), expr->value.leaf.double_constant);
        case CONST_BOOL:
            return LLVMConstInt(LLVMInt1Type(), expr->value.leaf.integer_constant, 0);
        case VAR_NAME:
            if (!assembler_is_defined(ptr, expr->value.leaf.str)) {
                printf("At line %d: Undefined variable %s\n", yylineno, expr->value.leaf.str);
                exit(-1);
            }
            LLVMValueRef val = (LLVMValueRef) _assembler_lookup(ptr, expr->value.leaf.str);
            return LLVMBuildLoad(ptr->builder, val, ".tmp");
        break;
        default:
            printf("Not implemented yet\n");
            exit(-1);
            return NULL;
        break;
    }
}

LLVMValueRef _generate_expr(Assembler_str* ptr, AbstractSyntacticTree* expr)
{
    return _generate_primary_expr(ptr, expr);
}

void _generate_assigment_expr(Assembler_str* ptr, AbstractSyntacticTree* inst)
{
    const char* var_name = inst->value.interior.left->value.leaf.str;
    AbstractSyntacticTree* expr = inst->value.interior.right;
    LLVMValueRef var;
    LLVMValueRef llvm = _generate_expr(ptr, expr);
    LLVMTypeRef expr_type = LLVMTypeOf(llvm);
    if (assembler_is_defined(ptr, var_name)) {
        var = _assembler_lookup(ptr, var_name);
        /*if (var_type != expr_type) {
            printf("Incompatible types! %s (%s) = %s\n", var_name,
                LLVMPrintTypeToString(var_type),
                LLVMPrintTypeToString(expr_type));
            exit(-1);
        }*/
    } else {
        var = LLVMBuildAlloca(ptr->builder, expr_type, var_name);
        hash_put(ptr->locals, (void*) var_name, var);
    }

    LLVMBuildStore(ptr->builder, llvm, var);
}

void _generate_statement(Assembler_str* ptr, AbstractSyntacticTree* inst)
{
    switch (inst->operation)
    {
        case ASSIGN_EXPR:
            _generate_assigment_expr(ptr, inst);
        break;
        default:
        break;
    }
}

void assembler_generate_function(Assembler assembler, FunctionNode* func_def)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    if (ll_size(&func_def->body) > 0) {
        LLVMValueRef func = hash_get(ptr->globals, (void*) func_def->func_name);
        LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
        
        LLVMPositionBuilderAtEnd(ptr->builder, entry);
        _add_params_to_locals(ptr, func, &func_def->parameters);
        LinkedList instructions = func_def->body;
        int has_return = 0;

        for (NodeList* node = ll_iter_begin(&instructions);
            node != ll_iter_end(&instructions);
            node = ll_iter_next(node))
        {
            AbstractSyntacticTree* inst = nl_getValue(node);
            if (inst->operation == RET_EXPR) has_return = 1;
            _generate_statement(ptr, inst);
            free(inst);
        }

        if (! has_return) {
            if (strcmp(".void", func_def->ret_type) == 0)
                LLVMBuildRetVoid(ptr->builder);
            else {
                printf("%s has no return! Expected %s\n", func_def->func_name, func_def->ret_type);
                exit(-1);
            }
        }

        hash_destroy(ptr->locals);
        ptr->locals = hash_create(100, 0.75, (_COMPARE*) str_compare, (_HASH_CODE*) str_hash_code, NULL, NULL, NULL, NULL);
    }

    ll_destroy(&func_def->body);
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
            if (is_double) llvm = LLVMBuildFAdd(ptr->builder, n1.llvm, n2.llvm, ".tmp");
            else llvm = LLVMBuildAdd(ptr->builder, n1.llvm, n2.llvm, ".tmp");
            break;
        case '-':
            if (is_double) llvm = LLVMBuildFSub(ptr->builder, n1.llvm, n2.llvm, ".tmp");
            else llvm = LLVMBuildSub(ptr->builder, n1.llvm, n2.llvm, ".tmp");
            break;
        case '*':
            if (is_double) llvm = LLVMBuildFMul(ptr->builder, n1.llvm, n2.llvm, ".tmp");
            else llvm = LLVMBuildMul(ptr->builder, n1.llvm, n2.llvm, ".tmp");
            break;
        case '/':
            if (is_double) llvm = LLVMBuildFDiv(ptr->builder, n1.llvm, n2.llvm, ".tmp");
            else llvm = LLVMBuildUDiv(ptr->builder, n1.llvm, n2.llvm, ".tmp");
            break;
        case '%':
            llvm = LLVMBuildSDiv(ptr->builder, n1.llvm, n2.llvm, ".tmp");
            break;
        case '>':
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, n1.llvm, n2.llvm, ".tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntUGT, n1.llvm, n2.llvm, ".tmp");
            break;
        case '<':
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, n1.llvm, n2.llvm, ".tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntSLT, n1.llvm, n2.llvm, ".tmp");
            break;
        case 'g':
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, n1.llvm, n2.llvm, ".tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntUGE, n1.llvm, n2.llvm, ".tmp");
            break;
        case 'l':
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, n1.llvm, n2.llvm, ".tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntSLE, n1.llvm, n2.llvm, ".tmp");
            break;
        case '=':
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, n1.llvm, n2.llvm, ".tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntEQ, n1.llvm, n2.llvm, ".tmp");
            break;
        case '!':
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, n1.llvm, n2.llvm, ".tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntNE, n1.llvm, n2.llvm, ".tmp");
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
        //param_types[i] = ((Parameter*) nl_getValue(cur_node))->type;
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
