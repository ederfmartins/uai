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


    //statement: expr '\n' | assignment_expression | print_stm | return_statement;
    //assignment_expression: IDENTIFIER '=' logical_or_expression '\n'
%}

%define parse.error verbose

%union { \
    int integer; \
    char* str; \
    double real; \
    Node node; \
    Parameter param; \
    LinkedList list; \
    AbstractSyntacticTree* ast; \
}

%token<str> IDENTIFIER
%token<integer> INT
%token<real> FLOAT
%token<str> BOOLEAN
%token<str> RETURN

%token<str> PRINT
%token<str> DEF
%token<str> OR_OP
%token<str> AND_OP
%token<str> LE_OP
%token<str> GE_OP
%token<str> EQ_OP
%token<str> NEQ_OP
%token<str> NE_OP

%type<node> postfix_expression
%type<node> additive_expression multiplicative_expression expr
%type<node> relational_expression equality_expression
%type<node> logical_and_expression logical_or_expression conditional_expression
%type<node> print_stm return_statement

%type<ast> function_definition statement
%type<ast> assignment_expression primary_expression
%type<list> parameter_list argument_expression_list function_list
%type<list> statement_list function_body
%type<param> parameter_declaration

%start prog

%%

prog: function_list
    {
        assembler_generate_functions(assembler, &$1);
    };

function_list:
    function_definition
    {
        ll_init(&$$);
        ll_insert(&$$, $1);
    }
    | function_list function_definition
    {
        $$ = $1;
        ll_insert(&$$, $2);
    };

statement_list
    : statement
    {
        ll_init(&$$);
        ll_insert(&$$, $1);
    }
    | statement_list statement
    {
        $$ = $1;
        ll_insert(&$$, $2);
    }
    ;

statement: assignment_expression;

assignment_expression: IDENTIFIER '=' postfix_expression '\n'
    {
        $$ = ast_assignment_expr($1, $3);
    }
    ;

expr: logical_or_expression;

primary_expression
    : INT
    {
        $$ = ast_const_int($1);
    }
    | FLOAT
    {
        $$ = ast_const_float($1);
    }
    | BOOLEAN
    {
        $$ = ast_const_bool($1);
    }
    | '(' expr ')'
    {
        //$$ = $2;
        $$ = NULL;
    }
    | IDENTIFIER
    {
        $$ = ast_var_name($1);
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

function_definition: DEF IDENTIFIER '(' parameter_list ')' function_body
    {
        $$ = ast_function_definition($2, ".void", &$4, &$6);
    }
    | DEF IDENTIFIER '(' ')' function_body
    {
        LinkedList l;
        ll_init(&l);
        $$ = ast_function_definition($2, ".void", &l, &$5);
    }
    ;

function_body: discardable_tokens '{' discardable_tokens statement_list discardable_tokens '}' discardable_tokens
    {
        $$ = $4;
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
        $$.type = $1;
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
            assembler_declare_builtin(assembler);
            
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
