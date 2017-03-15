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
    int integer; \
    char* str; \
    double real; \
    Parameter param; \
    LinkedList list; \
    AbstractSyntacticTree* ast; \
}

%token<integer> INT
%token<real> FLOAT
%token<str> BOOLEAN
%token<str> RETURN
%token<str> PRINT
%token<str> DEF
%token<str> IF
%token<str> ELSE

%token<str> OR_OP
%token<str> AND_OP
%token<str> NOT_OP
%token<str> LE_OP
%token<str> GE_OP
%token<str> EQ_OP
%token<str> NE_OP
%token<str> IDENTIFIER

%type<ast> additive_expression multiplicative_expression expr
%type<ast> relational_expression equality_expression
%type<ast> logical_and_expression logical_or_expression conditional_expression
%type<ast> print_stm return_statement

%type<ast> postfix_expression
%type<ast> function_definition statement if_statement
%type<ast> assignment_expression primary_expression
%type<list> parameter_list argument_expression_list function_list
%type<list> statement_list function_body compound_statement
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

statement
    : expr '\n' dt
    | assignment_expression dt
    | print_stm dt
    | return_statement dt
    | if_statement dt
    ;

assignment_expression: IDENTIFIER '=' expr '\n'
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
        $$ = $2;
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
        $$ = ast_function_call($1, NULL);
    }
    | IDENTIFIER '('argument_expression_list')'
    {
        $$ = ast_function_call($1, &$3);
    }
    ;

argument_expression_list
    : expr
    {
        ll_init(&$$);
        ll_insert(&$$, $1);
    }
    | argument_expression_list ',' dt expr
    {
        ll_insert(&$1, $4);
        $$ = $1;
    }
    ;

multiplicative_expression
    : postfix_expression
    | multiplicative_expression '*' dt postfix_expression
    {
        $$ = binary_expr(MUL, $1, $4);
    }
    | multiplicative_expression '/' dt postfix_expression
    {
        $$ = binary_expr(DIV, $1, $4);
    }
    | multiplicative_expression '%' dt postfix_expression
    {
        $$ = binary_expr(MOD, $1, $4);
    }
    ;

additive_expression
    : multiplicative_expression
    | additive_expression '+' dt multiplicative_expression
    {
        $$ = binary_expr(PLUS, $1, $4);
    }
    | additive_expression '-' dt multiplicative_expression
    {
        $$ = binary_expr(MINUS, $1, $4);
    }
    ;

relational_expression
    : additive_expression
    | relational_expression '<' dt additive_expression {$$ = binary_expr(LT, $1, $4);}
    | relational_expression '>' dt additive_expression {$$ = binary_expr(GT, $1, $4);}
    | relational_expression LE_OP dt additive_expression {$$ = binary_expr(LTE, $1, $4);}
    | relational_expression GE_OP dt additive_expression {$$ = binary_expr(GTE, $1, $4);}
    ;

equality_expression
    : relational_expression
    | equality_expression EQ_OP relational_expression {$$ = binary_expr(EQ, $1, $3);}
    | equality_expression NE_OP relational_expression {$$ = binary_expr(NEQ, $1, $3);}
    ;

logical_and_expression
    : equality_expression
    | logical_and_expression AND_OP equality_expression {$$ = binary_expr(NEQ, $1, $3);}
    ;

logical_or_expression
    : logical_and_expression
    | logical_or_expression OR_OP logical_and_expression {$$ = binary_expr(NEQ, $1, $3);}
    ;

print_stm: PRINT expr '\n'
    {
        $$ = ast_print($2);
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
    | DEF IDENTIFIER IDENTIFIER '(' ')' function_body
    {
        LinkedList l;
        ll_init(&l);
        $$ = ast_function_definition($3, $2, &l, &$6);
    }
    | DEF IDENTIFIER IDENTIFIER '(' parameter_list ')' function_body
    {
        $$ = ast_function_definition($3, $2, &$5, &$7);
    }
    ;

function_body: dt '{' dt statement_list dt '}' dt
    {
        $$ = $4;
    }
    | dt '{' dt '}' dt
    {
        ll_init(&$$);
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
        $$ = ast_return(NULL);
    }
    | RETURN expr '\n'
    {
        $$ = ast_return($2);
    }
    ;

compound_statement
    : '{' dt statement_list '}' dt
    {
        $$ = $3;
    }
    | statement
    {
        ll_init(&$$);
        ll_insert(&$$, $1);
    }

if_statement
    : IF expr dt compound_statement
    {
        LinkedList l;
        ll_init(&l);
        $$ = ast_if($2, $4, l);
    }
    | IF expr dt compound_statement ELSE dt compound_statement
    {
        $$ = ast_if($2, $4, $7);
    }

dt: discardable | epsilon;

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
