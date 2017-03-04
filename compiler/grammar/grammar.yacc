%{
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
%}

%define parse.error verbose

%union { int i; char* s; double d; Node node;}

%token<s> IDENTIFIER
%token<i> INT
%token<d> FLOAT
%token<s> BOOLEAN

%token<s> PRINT
%token<s> DEF
%token<s> OR_OP
%token<s> AND_OP
%token<s> LE_OP
%token<s> GE_OP
%token<s> EQ_OP
%token<s> NEQ_OP
%token<s> NE_OP

%type<node> additive_expression multiplicative_expression const_expression expr
%type<node> assignment_expression statement statement_list
%type<node> relational_expression equality_expression
%type<node> logical_and_expression logical_or_expression conditional_expression
%type<node> print_stm function_definition

%start prog

%%

prog: function_definition | prog function_definition;

statement_list
    : statement
    | statement_list statement
    {
        ll_transfer(&$1.instructions_list, &$2.instructions_list);
        ll_destroy(&$2.instructions_list);
        $$ = $1;
    }
    ;

statement: expr '\n' | assignment_expression | print_stm;

assignment_expression: IDENTIFIER '=' logical_or_expression '\n'
    {
        $$ = assembler_produce_store_variable(assembler, $1, $3);
    }
    ;

expr: logical_or_expression;

const_expression
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
    ;

multiplicative_expression
    : const_expression
    | multiplicative_expression '*' const_expression
    {
        $$ = exec_op(assembler, $1, $3, '*');
    }
    | multiplicative_expression '/' const_expression
    {
        $$ = exec_op(assembler, $1, $3, '/');
    }
    | multiplicative_expression '%' const_expression
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
        assembler_produce_function(assembler, $2, $8);
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
