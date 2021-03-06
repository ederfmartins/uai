#include "core.h"
#include "../collections/str_hash.h"

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Transforms/IPO.h>
#include <llvm-c/Transforms/Scalar.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yylineno;

// code generation private functions
LLVMValueRef _gen_expr(Assembler_str* ptr, AbstractSyntacticTree* expr);
LLVMValueRef _gen_binary_operation(Assembler ptr, BinaryOperator op,
    LLVMValueRef left, LLVMValueRef right);
void _gen_statement(Assembler_str* ptr, AbstractSyntacticTree* inst,
    LLVMValueRef cur_func, LLVMValueRef ret_var, LLVMBasicBlockRef end_block);
LLVMValueRef _gen_unary_expr(Assembler_str* ptr, AbstractSyntacticTree* expr);
void _assembler_declare_builtin(Assembler assembler);
void assembler_declare_printf(Assembler assembler);

// Private lookup functions
void* _assembler_lookup(Assembler assembler, const char* identifier);
int assembler_is_defined(Assembler assembler, const char* identifier);
int assembler_is_global_defined(Assembler assembler, const char* identifier);

char* _get_module_name(const char* fname);

// LLVM related private functions
LLVMValueRef _assembler_create_gconst_str(Assembler assembler, const char* name, const char* value);
LLVMTypeRef _get_function_ret_type(LLVMValueRef func);
int has_no_predecessors(Assembler assembler, LLVMBasicBlockRef entry,
    LLVMBasicBlockRef block);

void _configure_llvm(Assembler_str* assembler, const char* module_name)
{
    assembler->mod_name = _get_module_name(module_name);
    assembler->mod = LLVMModuleCreateWithName(assembler->mod_name);
    assembler->builder = LLVMCreateBuilder();
}

void _declare_builtin(Assembler ptr)
{
    hash_put(ptr->globals, (void*) "int", LLVMInt32Type());
    hash_put(ptr->globals, (void*) "boolean", LLVMInt1Type());
    hash_put(ptr->globals, (void*) "double", LLVMDoubleType());
    hash_put(ptr->globals, (void*) ".void", LLVMVoidType());
    hash_put(ptr->globals, (void*) "true", LLVMConstInt(LLVMInt1Type(), 1, 0));
    hash_put(ptr->globals, (void*) "false", LLVMConstInt(LLVMInt1Type(), 0, 0));
    assembler_declare_printf(ptr);
}

void _init_symbol_table(Assembler assembler)
{
    assembler->globals = hash_create(100, 0.75, (_COMPARE*) str_compare, (_HASH_CODE*) str_hash_code, NULL, NULL, NULL, NULL);
    assembler->locals = hash_create(100, 0.75, (_COMPARE*) str_compare, (_HASH_CODE*) str_hash_code, NULL, NULL, NULL, NULL);
    _declare_builtin(assembler);
}

void _validate_module(Assembler ptr)
{
    char *error = NULL;
    LLVMVerifyModule(ptr->mod, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);
}

Assembler assembler_init(const char* module_name)
{
    Assembler_str* assembler = (Assembler_str*) malloc(sizeof(Assembler_str));
    _configure_llvm(assembler, module_name);
    _init_symbol_table(assembler);
    _validate_module(assembler);
    return assembler;
}

void _run_optimizations(Assembler ptr)
{
    LLVMPassManagerRef  pm = LLVMCreatePassManager();
    LLVMAddConstantMergePass(pm);
    LLVMAddCFGSimplificationPass(pm);
    LLVMAddPromoteMemoryToRegisterPass(pm);
    //LLVMAddArgumentPromotionPass(pm);
    LLVMAddReassociatePass(pm);
    LLVMAddDeadStoreEliminationPass(pm);
    //LLVMAddTailCallEliminationPass(pm);
    //LLVMAddAggressiveDCEPass(pm);
    LLVMRunPassManager(pm, ptr->mod);
    LLVMDisposePassManager(pm);
}

int assembler_dump_bytecode(Assembler ptr, const char* out_name)
{
    _validate_module(ptr);
    _run_optimizations(ptr);
    _validate_module(ptr);
    return LLVMWriteBitcodeToFile(ptr->mod, out_name);
}

void assembler_destroy(Assembler assembler)
{
    Assembler_str* ptr = (Assembler_str*) assembler;
    hash_destroy(ptr->globals);
    hash_destroy(ptr->locals);
    LLVMDisposeBuilder(ptr->builder);
    LLVMDisposeModule(ptr->mod);
    free(ptr->mod_name);
    free(ptr);
}

void _declare_functions(Assembler assembler, LinkedList* func_list)
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
}

void _produce_functions_body(Assembler assembler, LinkedList* func_list)
{
    for (NodeList* node = ll_iter_begin(func_list);
        node != ll_iter_end(func_list);
        node = ll_iter_next(node))
    {
        AbstractSyntacticTree* func = nl_getValue(node);
        assembler_generate_function(assembler, &func->value.func_def);
        ast_destroy(func);
    }
}

void assembler_generate_functions(Assembler assembler, LinkedList* func_list)
{
    _declare_functions(assembler, func_list);
    _produce_functions_body(assembler, func_list);
    ll_destroy(func_list);
}

void _assert_type_is_declared(Assembler ptr, const char* type_name)
{
    if (! hash_contains_key(ptr->globals, (void*) type_name))
    {
        printf("%s was not declared\n", type_name);
        exit(-1);
    }
}

LLVMValueRef _function_ast_to_llvm(Assembler assembler, FunctionNode* func_def,
    LLVMTypeRef* args, int num_parans)
{
    LLVMTypeRef ret_type = hash_get(assembler->globals, (void*) func_def->ret_type);
    LLVMTypeRef ftype = LLVMFunctionType(ret_type, args, num_parans, func_def->is_vararg);
    LLVMValueRef func = LLVMAddFunction(assembler->mod, func_def->func_name, ftype);

    // if (ret_type == LLVMVoidType()) {
    //     LLVMAddFunctionAttr(func, LLVMNoUnwindAttribute);
    //     LLVMAddFunctionAttr(func, LLVMUWTable);
    // }
    return func;
}

LLVMTypeRef* _args_to_llvm(Assembler assembler, LinkedList* parameters, int num_parans)
{
    LLVMTypeRef* args = (LLVMTypeRef*) malloc(sizeof(LLVMTypeRef) * (num_parans + 1));
    num_parans = 0;

    for (NodeList* cur_node = ll_iter_begin(parameters);
        cur_node != ll_iter_end(parameters);
        cur_node = ll_iter_next(cur_node))
    {
        Parameter* param = nl_getValue(cur_node);
        _assert_type_is_declared(assembler, param->type);
        args[num_parans] = hash_get(assembler->globals, (void*) param->type);
        num_parans++;
    }

    return args;
}

void assembler_declare_function(Assembler ptr, FunctionNode* func_def)
{
    int num_parans = ll_size(&func_def->parameters);
    LLVMTypeRef* args = _args_to_llvm(ptr, &func_def->parameters, num_parans);
    _assert_type_is_declared(ptr, func_def->ret_type);
    LLVMValueRef func = _function_ast_to_llvm(ptr, func_def,  args, num_parans);
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
        LLVMSetValueName(args[num_parans], param->name);
        LLVMTypeRef type = LLVMTypeOf(args[num_parans]);
        LLVMValueRef var = LLVMBuildAlloca(ptr->builder, type, ".p");
        LLVMBuildStore(ptr->builder, args[num_parans], var);
        hash_put(ptr->locals, (void*) param->name, var);
        num_parans++;
    }

    free(args);
}

void _assert_function_exists(Assembler assembler, const char* func_name)
{
    if (! assembler_is_defined(assembler, func_name))
    {
        printf("At %d, function %s is not defined\n", yylineno, func_name);
        exit(-1);
    }
}

LLVMValueRef* _arguments_to_llvm(Assembler_str* ptr, LinkedList* exprs)
{
    LLVMValueRef* args = (LLVMValueRef*) malloc(sizeof(LLVMValueRef) * (ll_size(exprs)));
    NodeList* cur_node = ll_iter_begin(exprs);

    for (int i = 0; i < ll_size(exprs); i++)
    {
        AbstractSyntacticTree* tree = (AbstractSyntacticTree*) nl_getValue(cur_node);
        args[i] = _gen_expr(ptr, tree);
        cur_node = ll_iter_next(cur_node);
    }

    return args;
}

LLVMValueRef _gen_func_call(Assembler_str* ptr, AbstractSyntacticTree* func)
{
    _assert_function_exists(ptr, func->value.func_def.func_name);
    LLVMValueRef* args = _arguments_to_llvm(ptr, &func->value.func_def.parameters);
    int num_args = ll_size(&func->value.func_def.parameters);

    LLVMValueRef function = _assembler_lookup(ptr, func->value.func_def.func_name);
    LLVMValueRef call_instr;

    if (_get_function_ret_type(function) == LLVMVoidType())
    {
        call_instr = LLVMBuildCall(ptr->builder, function, args, num_args, "");
    } 
    else
    {
        call_instr = LLVMBuildCall(ptr->builder, function, args, num_args, ".ret");
    }

    free(args);
    return call_instr;
}

int _is_postfix_expr(AbstractSyntacticTree* expr)
{
    Production prod[] = {CONST_INT, CONST_REAL, CONST_BOOL, VAR_NAME, 
        FUNC_CALL, CONST_ARRAY, ARRAY_GET};
    for (int i = 0; i < 7; i++){
        if (expr->production == prod[i])
            return 1;
    }
    return 0;
}

int is_unary_expr(AbstractSyntacticTree* expr)
{
    if (expr->production == UN_EXPR) return 1;
    if (_is_postfix_expr(expr)) return 1;
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
        break;
        case CONST_BOOL:
            return LLVMConstInt(LLVMInt1Type(), expr->value.leaf.integer_constant, 0);
        break;
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

LLVMValueRef _gen_const_decl_array(Assembler_str* ptr, AbstractSyntacticTree* expr)
{
    LinkedList* elements = &expr->value.stm_list;
    int size = ll_size(elements);
    int i = 0;
    if (size < 1) {
        printf("Array size must be greater than 0\n");
        exit(-1);
    }
    LLVMValueRef* exprs = (LLVMValueRef*) malloc(sizeof(LLVMValueRef) * size);
    for (NodeList* node = ll_iter_begin(elements);
        node != ll_iter_end(elements);
        node = ll_iter_next(node))
    {
        AbstractSyntacticTree* ast = nl_getValue(node);
        LLVMValueRef val = _gen_expr(ptr, ast);
        exprs[i] = val;
        i++;
    }
    LLVMTypeRef array_type = LLVMArrayType(LLVMTypeOf(exprs[0]), size);
    LLVMValueRef array = LLVMBuildAlloca(ptr->builder, array_type, ".dcl_array");

    for (i = 0; i < size; i++)
    {
        LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, 0), LLVMConstInt(LLVMInt32Type(), i, 0)};
        LLVMValueRef var = LLVMBuildGEP(ptr->builder, array, indices, 2, ".tmp");
        LLVMBuildStore(ptr->builder, exprs[i], var);
    }
    free(exprs);
    return array;
}

LLVMValueRef _gen_get_array(Assembler_str* ptr, AbstractSyntacticTree* expr)
{
    LLVMValueRef idx = _gen_expr(ptr, expr->value.interior.right);
    LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, 0), idx};
    const char* var_name = expr->value.interior.left->value.leaf.str;

    if (!assembler_is_defined(ptr, var_name)) {
        printf("At line %d: Undefined variable %s\n", yylineno, expr->value.leaf.str);
        exit(-1);
    }
    LLVMValueRef array = (LLVMValueRef) _assembler_lookup(ptr, var_name);
    LLVMValueRef var = LLVMBuildGEP(ptr->builder, array, indices, 2, ".tmp");
    //return LLVMBuildLoad(ptr->builder, var, ".tmp");
    return var;
}

LLVMValueRef _gen_postfix_expr(Assembler_str* ptr, AbstractSyntacticTree* expr)
{
    switch(expr->production)
    {
        case FUNC_CALL:
            return _gen_func_call(ptr, expr);
        break;
        case CONST_ARRAY:
            return _gen_const_decl_array(ptr, expr);
        break;
        case ARRAY_GET:
            return _gen_get_array(ptr, expr);
        break;
        default:
            return _gen_primary_expr(ptr, expr);
    }
}

LLVMValueRef _gen_get_pointer(Assembler_str* ptr, AbstractSyntacticTree* expr)
{
    LLVMValueRef expr_val = _gen_unary_expr(ptr, expr->value.interior.left);
    LLVMValueRef indices[1];
    LLVMValueRef var = LLVMBuildGEP(ptr->builder, expr_val, indices, 0, ".tmp");
    return var;
}

LLVMValueRef _gen_unary_expr(Assembler_str* ptr, AbstractSyntacticTree* expr)
{
    if (_is_postfix_expr(expr))
        return _gen_postfix_expr(ptr, expr);
    switch (expr->operation)
    {
        case U_GET_ADRESS:
            return _gen_get_pointer(ptr, expr);
        break;
        default:
            printf("_gen_statement: Not implemented yet \n");
            exit(-1);
        break;
    }
}

LLVMValueRef _gen_expr(Assembler_str* ptr, AbstractSyntacticTree* expr)
{
    if (is_unary_expr(expr)) {
        LLVMValueRef val = _gen_unary_expr(ptr, expr);
        if (LLVMGetInstructionOpcode(val) == LLVMAlloca)
            return LLVMBuildLoad(ptr->builder, val, ".tmp");
        else return val;
    }

    if (expr->production != B_EXPR) {
        printf("Malformated ast! %d occur in a expression\n", expr->production);
        exit(-1);
    }

    LLVMValueRef left = _gen_expr(ptr, expr->value.interior.left);
    LLVMValueRef right = _gen_expr(ptr, expr->value.interior.right);
    return _gen_binary_operation(ptr, expr->operation, left, right);
}

LLVMValueRef _right_assign_to_llvm(Assembler_str* ptr, AbstractSyntacticTree* expr)
{
    return _gen_expr(ptr, expr);
}

LLVMValueRef _declare_variable_as_llvm(Assembler_str* ptr, const char* var_name, LLVMTypeRef type)
{
    LLVMValueRef var;
    if (assembler_is_defined(ptr, var_name)) {
        var = _assembler_lookup(ptr, var_name);
    } else {
        var = LLVMBuildAlloca(ptr->builder, type, var_name);
        hash_put(ptr->locals, (void*) var_name, var);
    }

    return var;
}

LLVMValueRef _left_assign_to_llvm(Assembler_str* ptr, LLVMTypeRef type, AbstractSyntacticTree* left)
{
    if (left->production == VAR_NAME){
        const char* var_name = left->value.leaf.str;
        return _declare_variable_as_llvm(ptr, var_name, type);
    }
    else {
        LLVMValueRef var = _gen_unary_expr(ptr, left);
        //LLVMDumpValue(var);
        return var;
    }
}

void _gen_assigment_expr(Assembler_str* ptr, AbstractSyntacticTree* inst)
{
    LLVMValueRef llvm = _right_assign_to_llvm(ptr, inst->value.interior.right);
    LLVMTypeRef expr_type = LLVMTypeOf(llvm);
    LLVMValueRef var = _left_assign_to_llvm(ptr, expr_type, inst->value.interior.left);
    LLVMBuildStore(ptr->builder, llvm, var);
}

LLVMValueRef _gen_print(Assembler ptr, AbstractSyntacticTree* print_stm)
{
    LLVMValueRef expr = _gen_expr(ptr, print_stm->value.interior.left);
    if (LLVMGetInstructionOpcode(expr) == LLVMGetElementPtr)
        expr = LLVMBuildLoad(ptr->builder, expr, ".tmp");

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

void _gen_return(Assembler_str* ptr, AbstractSyntacticTree* ret,
    LLVMValueRef ret_var, LLVMBasicBlockRef end_block)
{
    if (ret->value.interior.left != NULL)
    {
        LLVMValueRef ret_val = _gen_expr(ptr, ret->value.interior.left);
        LLVMBuildStore(ptr->builder, ret_val, ret_var);
    }

    LLVMBuildBr(ptr->builder, end_block);
}

void _gen_if(Assembler_str* ptr, AbstractSyntacticTree* inst,
    LLVMValueRef cur_func, LLVMValueRef ret_var, LLVMBasicBlockRef end_block)
{
    LLVMValueRef condition = _gen_expr(ptr, inst->value.tnode.cond);
    LLVMBasicBlockRef iftrue = LLVMAppendBasicBlock(cur_func, ".iftrue");
    LLVMBasicBlockRef iffalse = LLVMAppendBasicBlock(cur_func, ".iffalse");
    LLVMBasicBlockRef end = LLVMAppendBasicBlock(cur_func, ".endif");
    LLVMBuildCondBr(ptr->builder, condition, iftrue, iffalse);
    int contais_ret = 0;

    LLVMPositionBuilderAtEnd(ptr->builder, iftrue);
    for (NodeList* node = ll_iter_begin(&inst->value.tnode.left);
         node != ll_iter_end(&inst->value.tnode.left);
         node = ll_iter_next(node))
    {
        AbstractSyntacticTree* inst1 = nl_getValue(node);
        _gen_statement(ptr, inst1, cur_func, ret_var, end_block);
        if (inst1->production == RET_EXPR) contais_ret = 1;
    }
    if (! contais_ret)
        LLVMBuildBr(ptr->builder, end);

    contais_ret = 0;
    LLVMPositionBuilderAtEnd(ptr->builder, iffalse);
    for (NodeList* node = ll_iter_begin(&inst->value.tnode.right);
         node != ll_iter_end(&inst->value.tnode.right);
         node = ll_iter_next(node))
    {
        AbstractSyntacticTree* inst1 = nl_getValue(node);
        _gen_statement(ptr, inst1, cur_func, ret_var, end_block);
        if (inst1->production == RET_EXPR) contais_ret = 1;
    }
    if (! contais_ret)
        LLVMBuildBr(ptr->builder, end);

    LLVMPositionBuilderAtEnd(ptr->builder, end);
}

void _gen_for(Assembler_str* ptr, AbstractSyntacticTree* inst,
    LLVMValueRef cur_func, LLVMValueRef ret_var, LLVMBasicBlockRef end_block)
{
    LLVMBasicBlockRef test_cond = LLVMAppendBasicBlock(cur_func, ".ftest");
    LLVMBasicBlockRef fbody = LLVMAppendBasicBlock(cur_func, ".fbody");
    LLVMBasicBlockRef inc = LLVMAppendBasicBlock(cur_func, ".finc");
    LLVMBasicBlockRef end = LLVMAppendBasicBlock(cur_func, ".endfor");

    _gen_statement(ptr, inst->value.for_node.init, cur_func, ret_var, end_block);
    LLVMBuildBr(ptr->builder, test_cond);

    LLVMPositionBuilderAtEnd(ptr->builder, test_cond);
    LLVMValueRef condition = _gen_expr(ptr, inst->value.for_node.cond);
    LLVMBuildCondBr(ptr->builder, condition, fbody, end);

    int contais_ret = 0;
    LLVMPositionBuilderAtEnd(ptr->builder, fbody);
    for (NodeList* node = ll_iter_begin(&inst->value.for_node.body);
         node != ll_iter_end(&inst->value.for_node.body);
         node = ll_iter_next(node))
    {
        AbstractSyntacticTree* inst1 = nl_getValue(node);
        _gen_statement(ptr, inst1, cur_func, ret_var, end_block);
        if (inst1->production == RET_EXPR) contais_ret = 1;
    }
    if (! contais_ret)
        LLVMBuildBr(ptr->builder, inc);

    LLVMPositionBuilderAtEnd(ptr->builder, inc);
    _gen_statement(ptr, inst->value.for_node.inc, cur_func, ret_var, end_block);
    LLVMBuildBr(ptr->builder, test_cond);    

    LLVMPositionBuilderAtEnd(ptr->builder, end);
}

void _gen_statement(Assembler_str* ptr, AbstractSyntacticTree* inst,
    LLVMValueRef cur_func, LLVMValueRef ret_var, LLVMBasicBlockRef end_block)
{
    switch (inst->production)
    {
        case ASSIGN_EXPR:
            _gen_assigment_expr(ptr, inst);
        break;
        case RET_EXPR:
            _gen_return(ptr, inst, ret_var, end_block);
        break;
        case PRINT_STM:
            _gen_print(ptr, inst);
        break;
        case IF_STM:
            _gen_if(ptr, inst, cur_func, ret_var, end_block);
        break;
        case FOR_STM:
            _gen_for(ptr, inst, cur_func, ret_var, end_block);
        break;
        case B_EXPR:
        case CONST_INT:
        case CONST_REAL:
        case CONST_BOOL:
        case VAR_NAME:
        case FUNC_CALL:
            _gen_expr(ptr, inst);
        break;
        default:
            printf("_gen_statement: Not implemented yet \n");
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
        LLVMBasicBlockRef end = LLVMAppendBasicBlock(func, "end");
        LLVMPositionBuilderAtEnd(ptr->builder, entry);
        LLVMValueRef ret_val = 0;
        int contais_ret = 0;

        if (strcmp(".void", func_def->ret_type) != 0) {
            ret_val = LLVMBuildAlloca(ptr->builder,
                hash_get(assembler->globals, (void*) func_def->ret_type), ".ret");
            LLVMPositionBuilderAtEnd(ptr->builder, end);
            LLVMBuildRet(ptr->builder, LLVMBuildLoad(ptr->builder, ret_val, ".ret_reg"));
            LLVMPositionBuilderAtEnd(ptr->builder, entry);
        }
        else {
            LLVMPositionBuilderAtEnd(ptr->builder, end);
            LLVMBuildRetVoid(ptr->builder);
            LLVMPositionBuilderAtEnd(ptr->builder, entry);
        }
        
        _add_params_to_locals(ptr, func, &func_def->parameters);
        LinkedList instructions = func_def->body;

        for (NodeList* node = ll_iter_begin(&instructions);
            node != ll_iter_end(&instructions);
            node = ll_iter_next(node))
        {
            AbstractSyntacticTree* inst = nl_getValue(node);
            _gen_statement(ptr, inst, func, ret_val, end);
            if (inst->production == RET_EXPR) contais_ret = 1;
        }

        if (!contais_ret) {
            //LLVMGetNumSuccessors
            LLVMBuildBr(ptr->builder, end);
            //if (LLVMGetFirstInstruction(LLVMGetInsertBlock(ptr->builder)) != 0)
            //else LLVMDeleteBasicBlock(LLVMGetInsertBlock(ptr->builder));
        }

        LLVMBasicBlockRef cur_block = LLVMGetInsertBlock(ptr->builder);
        if (cur_block != entry && cur_block != end)
        {
            if (has_no_predecessors(ptr, entry, cur_block))
                LLVMDeleteBasicBlock(LLVMGetInsertBlock(ptr->builder));
        }

        if (LLVMGetLastBasicBlock(func) != end)
            LLVMMoveBasicBlockAfter(end, LLVMGetLastBasicBlock(func));
        // if (!has_return) {
        //     printf("%s has no return! Expected %s\n", func_def->func_name, func_def->ret_type);
        //     exit(-1);
        // }

//LLVMDumpValue(func);
        hash_destroy(ptr->locals);
        ptr->locals = hash_create(100, 0.75, (_COMPARE*) str_compare, (_HASH_CODE*) str_hash_code, NULL, NULL, NULL, NULL);
    }
}

int has_no_predecessors(Assembler assembler, LLVMBasicBlockRef entry,
    LLVMBasicBlockRef block)
{
    while (entry != 0)
    {
        LLVMValueRef term = LLVMGetBasicBlockTerminator(entry);
        for (unsigned int i = 0; i < LLVMGetNumSuccessors(term); i++)
        {
            if (block == LLVMGetSuccessor(term, i))
                return 0;
        }
        entry = LLVMGetNextBasicBlock(entry);
    }

    return 1;
}

LLVMValueRef _gen_binary_operation(Assembler ptr, BinaryOperator op,
    LLVMValueRef left, LLVMValueRef right)
{
    LLVMValueRef llvm;
    int is_double = (LLVMTypeOf(left) == LLVMDoubleType());

    if (LLVMGetInstructionOpcode(left) == LLVMAlloca)
        left = LLVMBuildLoad(ptr->builder, left, ".left");
    if (LLVMGetInstructionOpcode(right) == LLVMAlloca)
        right = LLVMBuildLoad(ptr->builder, right, ".right");

    if (LLVMGetInstructionOpcode(left) == LLVMGetElementPtr)
        left = LLVMBuildLoad(ptr->builder, left, ".tmp");
    if (LLVMGetInstructionOpcode(right) == LLVMGetElementPtr)
        right = LLVMBuildLoad(ptr->builder, right, ".tmp");

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
            if (is_double) llvm = LLVMBuildFRem(ptr->builder, left, right, ".tmp");
            else llvm = LLVMBuildSRem(ptr->builder, left, right, ".tmp");
            break;
        case GT:
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealOGT, left, right, ".tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntSGT, left, right, ".tmp");
            break;
        case LT:
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealOGT, left, right, ".tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntSLT, left, right, ".tmp");
            break;
        case GTE:
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealOGT, left, right, ".tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntSGE, left, right, ".tmp");
            break;
        case LTE:
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealOGT, left, right, ".tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntSLE, left, right, ".tmp");
            break;
        case EQ:
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealOGT, left, right, ".tmp");
            else llvm = LLVMBuildICmp (ptr->builder, LLVMIntEQ, left, right, ".tmp");
            break;
        case NEQ:
            if (is_double) llvm = LLVMBuildFCmp (ptr->builder, LLVMRealOGT, left, right, ".tmp");
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

void assembler_declare_printf(Assembler ptr)
{
    LLVMValueRef d = _assembler_create_gconst_str(ptr, ".print_arg_d", "%d\n");
    hash_put(ptr->globals, (void*) ".print_arg_d", d);
    LLVMValueRef f = _assembler_create_gconst_str(ptr, ".print_arg_f", "%lf\n");
    hash_put(ptr->globals, (void*) ".print_arg_f", f);

    LLVMTypeRef param_types[] = {LLVMPointerType(LLVMInt8Type(), 0)};
    LLVMTypeRef printf_type = LLVMFunctionType(LLVMInt32Type(), param_types, 1, 1);
    LLVMValueRef print_func = LLVMAddFunction(ptr->mod, "printf", printf_type);
    LLVMAddAttribute(LLVMGetFirstParam(print_func), LLVMNoAliasAttribute);
    hash_put(ptr->globals, (void*) "print", print_func);
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

LLVMTypeRef _get_function_ret_type(LLVMValueRef func)
{
    return LLVMGetReturnType(LLVMGetReturnType(LLVMTypeOf(func)));
}
