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

// Private functions
void* _assembler_lookup(Assembler assembler, const char* identifier);
int assembler_is_defined(Assembler assembler, const char* identifier);
int assembler_is_global_defined(Assembler assembler, const char* identifier);
LLVMValueRef _assembler_create_gconst_str(Assembler assembler, const char* name, const char* value);
char* _get_module_name(const char* fname);
void _assembler_declare_builtin(Assembler assembler);
void assembler_declare_printf(Assembler assembler);

// code gen privete functions
LLVMValueRef _gen_expr(Assembler_str* ptr, AbstractSyntacticTree* expr);
LLVMValueRef _gen_binary_operation(Assembler ptr, BinaryOperator op,
    LLVMValueRef left, LLVMValueRef right);

Assembler assembler_init(const char* module_name)
{
    Assembler_str* assembler = (Assembler_str*) malloc(sizeof(Assembler_str));
    assembler->mod_name = _get_module_name(module_name);

    assembler->mod = LLVMModuleCreateWithName(assembler->mod_name);
    assembler->builder = LLVMCreateBuilder();
    
    assembler->globals = hash_create(100, 0.75, (_COMPARE*) str_compare, (_HASH_CODE*) str_hash_code, NULL, NULL, NULL, NULL);
    assembler->locals = hash_create(100, 0.75, (_COMPARE*) str_compare, (_HASH_CODE*) str_hash_code, NULL, NULL, NULL, NULL);

    _assembler_declare_builtin(assembler);
    return (Assembler) assembler;
}

void _assembler_declare_builtin(Assembler assembler)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    assembler_declare_printf(assembler);
    hash_put(ptr->globals, (void*) "int", LLVMInt32Type());
    hash_put(ptr->globals, (void*) "boolean", LLVMInt1Type());
    hash_put(ptr->globals, (void*) "double", LLVMDoubleType());
    hash_put(ptr->globals, (void*) ".void", LLVMVoidType());
    hash_put(ptr->globals, (void*) "true", LLVMConstInt(LLVMInt1Type(), 1, 0));
    hash_put(ptr->globals, (void*) "false", LLVMConstInt(LLVMInt1Type(), 0, 0));
}

int assembler_dump_bytecode(Assembler assembler, const char* out_name)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    char *error = NULL;
    LLVMVerifyModule(ptr->mod, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);
    return LLVMWriteBitcodeToFile(ptr->mod, out_name);
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
        if (func->production != FUNC_DEF) {
            printf("Invalid parameter assembler_generate_functions called with a ast of type %d\n", func->production);
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

int _is_postfix_expr(AbstractSyntacticTree* expr)
{
    Production prod[] = {CONST_INT, CONST_REAL, CONST_BOOL, VAR_NAME, FUNC_CALL};
    for (int i = 0; i < 5; i++)
        if (expr->production == prod[i])
            return 1;
    return 0;
}

LLVMValueRef _gen_primary_expr(Assembler_str* ptr, AbstractSyntacticTree* expr)
{
    switch (expr->production)
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
            if (LLVMIsConstant (val)) return val;
            else return LLVMBuildLoad(ptr->builder, val, ".tmp");
        break;
        default:
            return _gen_expr(ptr, expr);
        break;
    }
}

LLVMValueRef _gen_func_call(Assembler_str* ptr, AbstractSyntacticTree* func)
{
    if (! assembler_is_defined(ptr, func->value.func_def.func_name))
    {
        printf("At %d, function %s is not defined\n", yylineno, func->value.func_def.func_name);
        exit(-1);
    }

    LinkedList* exprs = &func->value.func_def.parameters;
    LLVMValueRef* args = (LLVMValueRef*) malloc(sizeof(LLVMValueRef) * (ll_size(exprs)));
    NodeList* cur_node = ll_iter_begin(exprs);

    for (int i = 0; i < ll_size(exprs); i++)
    {
        AbstractSyntacticTree* tree = (AbstractSyntacticTree*) nl_getValue(cur_node);
        args[i] = _gen_expr(ptr, tree);
        cur_node = ll_iter_next(cur_node);
    }

    LLVMValueRef f = _assembler_lookup(ptr, func->value.func_def.func_name);
    LLVMValueRef call_func = LLVMBuildCall(ptr->builder, f, args, ll_size(exprs), ".ret");
    free(args);
    return call_func;
}

LLVMValueRef _gen_postfix_expr(Assembler_str* ptr, AbstractSyntacticTree* expr)
{
    if (expr->production == FUNC_CALL) return _gen_func_call(ptr, expr);
    else return _gen_primary_expr(ptr, expr);
}

LLVMValueRef _gen_expr(Assembler_str* ptr, AbstractSyntacticTree* expr)
{
    if (_is_postfix_expr(expr))
        return _gen_postfix_expr(ptr, expr);

    if (!(expr->production == B_EXPR)) {
        printf("Malformated ast! %d occur in a expression\n", expr->production);
        exit(-1);
    }

    LLVMValueRef left = _gen_expr(ptr, expr->value.interior.left);
    LLVMValueRef right = _gen_expr(ptr, expr->value.interior.right);
    return _gen_binary_operation(ptr, expr->operation, left, right);
}

void _gen_assigment_expr(Assembler_str* ptr, AbstractSyntacticTree* inst)
{
    const char* var_name = inst->value.interior.left->value.leaf.str;
    AbstractSyntacticTree* expr = inst->value.interior.right;
    LLVMValueRef var;
    LLVMValueRef llvm = _gen_expr(ptr, expr);
    LLVMTypeRef expr_type = LLVMTypeOf(llvm);
    if (assembler_is_defined(ptr, var_name)) {
        var = _assembler_lookup(ptr, var_name);
    } else {
        var = LLVMBuildAlloca(ptr->builder, expr_type, var_name);
        hash_put(ptr->locals, (void*) var_name, var);
    }

    LLVMBuildStore(ptr->builder, llvm, var);
}

LLVMValueRef _gen_return(Assembler_str* ptr, AbstractSyntacticTree* ret)
{
    if (ret->value.interior.left == NULL)
        return LLVMBuildRetVoid(ptr->builder);
    LLVMValueRef expr = _gen_expr(ptr, ret->value.interior.left);
    return LLVMBuildRet(ptr->builder, expr);
}

LLVMValueRef _gen_print(Assembler ptr, AbstractSyntacticTree* print_stm)
{
    LLVMValueRef expr = _gen_expr(ptr, print_stm->value.interior.left);
    LLVMValueRef arg;

    if (LLVMTypeOf(expr) == LLVMDoubleType()) {
        arg = hash_get(ptr->globals, (void*) ".print_arg_f");
    }
    else {
        arg = hash_get(ptr->globals, (void*) ".print_arg_d");
    }

    arg = LLVMConstPointerCast(arg, LLVMPointerType(LLVMInt8Type(), 0));
    LLVMValueRef args[] = {arg, expr};
    LLVMValueRef print = hash_get(ptr->globals, (void*) "print");
    return LLVMBuildCall(ptr->builder, print, args, 2, ".print");
}

void _gen_statement(Assembler_str* ptr, AbstractSyntacticTree* inst)
{
    switch (inst->production)
    {
        case ASSIGN_EXPR:
            _gen_assigment_expr(ptr, inst);
        break;
        case RET_EXPR:
            _gen_return(ptr, inst);
        break;
        case PRINT_STM:
            _gen_print(ptr, inst);
        break;
        default:
            printf("Not implemented yet\n");
            exit(-1);
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
            if (inst->production == RET_EXPR) has_return = 1;
            _gen_statement(ptr, inst);
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

LLVMValueRef _gen_binary_operation(Assembler ptr, BinaryOperator op,
    LLVMValueRef left, LLVMValueRef right)
{
    LLVMValueRef llvm;
    int is_double = (LLVMTypeOf(left) == LLVMDoubleType());

    switch (op) {
        case PLUS:
            if (is_double) llvm = LLVMBuildFAdd(ptr->builder, left, right, ".tmp");
            else llvm = LLVMBuildAdd(ptr->builder, left, right, ".tmp");
            break;
        case MINUS:
            if (is_double) llvm = LLVMBuildFSub(ptr->builder, left, right, ".tmp");
            else llvm = LLVMBuildSub(ptr->builder, left, right, ".tmp");
            break;
        case MUL:
            if (is_double) llvm = LLVMBuildFMul(ptr->builder, left, right, ".tmp");
            else llvm = LLVMBuildMul(ptr->builder, left, right, ".tmp");
            break;
        case DIV:
            if (is_double) llvm = LLVMBuildFDiv(ptr->builder, left, right, ".tmp");
            else llvm = LLVMBuildSDiv(ptr->builder, left, right, ".tmp");
            break;
        case MOD:
            llvm = LLVMBuildSRem(ptr->builder, left, right, ".tmp");
            break;
        case GT:
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, left, right, ".tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntUGT, left, right, ".tmp");
            break;
        case LT:
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, left, right, ".tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntSLT, left, right, ".tmp");
            break;
        case GTE:
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, left, right, ".tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntUGE, left, right, ".tmp");
            break;
        case LTE:
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, left, right, ".tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntSLE, left, right, ".tmp");
            break;
        case EQ:
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, left, right, ".tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntEQ, left, right, ".tmp");
            break;
        case NEQ:
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealUGT, left, right, ".tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntNE, left, right, ".tmp");
            break;
        default:
            printf("Operation is not defined in the grammar!\n");
            exit(-1);
        break;
    }
    return llvm;
}

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
