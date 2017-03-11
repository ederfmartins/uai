%{
    //#define YYDEBUG 1
    #include "../grammar/ast_node.h"
    #include "../assembler/core.h"

    #include <llvm-c/Core.h>
    #include <llvm-c/ExecutionEngine.h>
    #include <llvm-c/Target.h>
    #include <llvm-c/Analysis.h>
    #include <llvm-c/BitWriter.h>
    
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
    
    extern int yylex();
    extern FILE* yyin;
    extern int yylineno;
    int yyparse();
    void yyerror(const char *str);

    Assembler assembler;
    //int yydebug = 1;
%}

%define parse.error verbose

%union { \
    int i; \
    char* s; \
    double d; \
    Node node; \
    Parameter param; \
    LinkedList list; \
}

%token<s> IDENTIFIER
%token<i> INT
%token<d> FLOAT
%token<s> BOOLEAN
%token<s> RETURN

%token<s> PRINT
%token<s> DEF
%token<s> OR_OP
%token<s> AND_OP
%token<s> LE_OP
%token<s> GE_OP
%token<s> EQ_OP
%token<s> NEQ_OP
%token<s> NE_OP

%type<node> postfix_expression primary_expression
%type<node> additive_expression multiplicative_expression expr
%type<node> assignment_expression statement statement_list
%type<node> relational_expression equality_expression
%type<node> logical_and_expression logical_or_expression conditional_expression
%type<node> print_stm function_definition return_statement
%type<list> parameter_list argument_expression_list
%type<param> parameter_declaration

%start prog

%%

prog: function_definition | prog function_definition;

statement_list
    : statement
    | statement_list statement
    {
        $$ = node_merge($1, $2);
    }
    ;

statement: expr '\n' | assignment_expression | print_stm | return_statement;

assignment_expression: IDENTIFIER '=' logical_or_expression '\n'
    {
        $$ = assembler_produce_store_variable(assembler, $1, $3);
    }
    ;

expr: logical_or_expression;

primary_expression
    : INT
    {
        $$ = assembler_const_int(assembler, $1);
    }
    | FLOAT
    {
        $$ = assembler_const_float(assembler, $1);
    }
    | BOOLEAN
    {
        $$ = assembler_const_bool(assembler, $1);
    }
    | '(' expr ')'
    {
        $$ = $2;
    }
    | IDENTIFIER
    {
        if (!assembler_is_defined(assembler, $1)) {
            printf("Error at line %d: Undefined variable %s\n", yylineno, $1);
            exit(-1);
        }
        $$ = assembler_produce_load_variable(assembler, $1);
    }
    | 
    ;

postfix_expression
    : primary_expression
    | IDENTIFIER '(' ')'
    {
        // corrigir os testes de integração
        Assembler_str* ptr = (Assembler_str*) assembler;
        $$ = node_init();
        LLVMValueRef args[1];
        LLVMValueRef f = hash_get(ptr->globals, (void*) $1);
        LLVMValueRef call_func = LLVMBuildCall(ptr->builder, f, args, 0, "tmp");
        node_add_instruction(&$$, call_func);
    }
    | IDENTIFIER '('argument_expression_list')'
    {
        $$ = node_init();
        Assembler_str* ptr = (Assembler_str*) assembler;
        LinkedList list = $3;
        NodeList* cur_node = ll_iter_begin(&list);
        LLVMValueRef args[200];

        if (ll_size(&list) > 200) {
            printf("Calling a function with %d arguments! Please refactore this cheet code, so.\n", ll_size(&list));
            exit(-1);
        }

        for (int i = 0; i < ll_size(&list); i++)
        {
            args[i] = ((Node*) nl_getValue(cur_node))->llvm;
            $$ = node_merge($$, * ((Node*) nl_getValue(cur_node)));
            cur_node = ll_iter_next(cur_node);
        }

        LLVMValueRef f = hash_get(ptr->globals, (void*) $1);
        LLVMValueRef call_func = LLVMBuildCall(ptr->builder, f, args, ll_size(&list), "tmp");
        node_add_instruction(&$$, call_func);
    }
    ;

argument_expression_list
    : expr
    {
        ll_init(&$$);
        Node* n = (Node*) malloc(sizeof(Node));
        *n = $1;
        ll_insert(&$$, n);
    }
    | argument_expression_list ',' expr
    {
        Node* n = (Node*) malloc(sizeof(Node));
        *n = $3;
        ll_insert(&$1, n);
    }
    ;

multiplicative_expression
    : postfix_expression
    | multiplicative_expression '*' postfix_expression
    {
        $$ = exec_op(assembler, $1, $3, '*');
    }
    | multiplicative_expression '/' postfix_expression
    {
        $$ = exec_op(assembler, $1, $3, '/');
    }
    | multiplicative_expression '%' postfix_expression
    {
        $$ = exec_op(assembler, $1, $3, '%');
    }
    ;

additive_expression
    : multiplicative_expression
    | additive_expression '+' multiplicative_expression
    {
        $$ = exec_op(assembler, $1, $3, '+');
    }
    | additive_expression '-' multiplicative_expression
    {
        $$ = exec_op(assembler, $1, $3, '-');
    }
    ;

relational_expression
    : additive_expression
    | relational_expression '<' additive_expression {$$ = exec_op(assembler, $1, $3, '<');}
    | relational_expression '>' additive_expression {$$ = exec_op(assembler, $1, $3, '>');}
    | relational_expression LE_OP additive_expression {$$ = exec_op(assembler, $1, $3, 'l');}
    | relational_expression GE_OP additive_expression {$$ = exec_op(assembler, $1, $3, 'g');}
    ;

equality_expression
    : relational_expression
    | equality_expression EQ_OP relational_expression {$$ = exec_op(assembler, $1, $3, '=');}
    | equality_expression NE_OP relational_expression {$$ = exec_op(assembler, $1, $3, '!');}
    ;

logical_and_expression
    : equality_expression
    | logical_and_expression AND_OP equality_expression
    ;

logical_or_expression
    : logical_and_expression
    | logical_or_expression OR_OP logical_and_expression
    ;

print_stm: PRINT expr '\n'
    {
        $$ = assembler_produce_print(assembler, $2);
    }
    ;

function_definition: DEF IDENTIFIER '(' ')' discardable_tokens '{' discardable_tokens statement_list discardable_tokens '}' discardable_tokens
    {
        if (assembler_is_defined(assembler, $2)) {
            printf("Error at line %d: identifier %s was previos defined!\n", yylineno, $2);
            exit(-1);
        }
        LinkedList l;
        l.cnt = 0;
        assembler_produce_function(assembler, $2, $8, l);
    }
    | DEF IDENTIFIER '(' parameter_list ')' discardable_tokens '{' discardable_tokens statement_list discardable_tokens '}' discardable_tokens
    {
        if (assembler_is_defined(assembler, $2)) {
            printf("Error at line %d: identifier %s was previos defined!\n", yylineno, $2);
            exit(-1);
        }

        if (ll_size(&$4) > 200) {
            printf("Declaring a function with %d arguments! Please refactore this cheet code, so.\n", ll_size(&$4));
            exit(-1);
        }

        assembler_produce_function(assembler, $2, $9, $4);
    }
    ;

parameter_list
    : parameter_declaration
    {
        ll_init(&$$);
        Parameter *p = (Parameter*) malloc(sizeof(Parameter));
        *p = $1;
        ll_insert(&$$, p);
    }
    | parameter_list ',' parameter_declaration
    {
        $$ = $1;
        Parameter *p = (Parameter*) malloc(sizeof(Parameter));
        *p = $3;
        ll_insert(&$$, p);
    }
    ;

parameter_declaration: IDENTIFIER IDENTIFIER
    {
        $$.type = LLVMInt32Type();
        $$.name = $2;
    }
    ;

return_statement
    : RETURN '\n'
    {
        $$ = assembler_produce_ret_void(assembler);
    }
    | RETURN expr '\n'
    {
        $$ = assembler_produce_return(assembler, $2);
    }
    ;

discardable_tokens: discardable | epsilon;

discardable: discardable  discardable_token | discardable_token;

discardable_token: '\n'; 

epsilon: ; 

%%

    void yyerror(const char *str)
    {
        fprintf(stderr,"Error at line %d: %s\n", yylineno, str);
    }

    int main(int argc, char* argv[])
    {
        if (argc > 3 || argc < 2) {
            printf("Usage: ./uai input.uai [output.bc]\n");
            return -1;
        }

        yyin = fopen(argv[1], "rb");
        
        if (yyin) {
            assembler = assembler_init(argv[1]);
            assembler_declare_printf(assembler);
            
            yyparse();
            fclose(yyin);
            char* out_name;

            if (argc < 3) {
                out_name = (char *) malloc(strlen(argv[1]) + 5);
                sprintf(out_name, "%s.bc", argv[1]);
            } else {
                out_name = argv[2];
            }

            if (assembler_dump_bytecode(assembler, out_name) != 0) {
                fprintf(stderr, "error writing bitcode to file, skipping\n");
            }
        }

        return 0;
    }
