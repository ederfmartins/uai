D           [0-9]
L           [a-zA-Z_]
H           [a-fA-F0-9]
E           [Ee][+-]?{D}+

%{
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
    #include "../grammar/ast_node.h"
    #include "../grammar/grammar.h"
%}

%option yylineno

%%

"<="                  { return LE_OP; }
">="                  { return GE_OP; }
"=="                  { return EQ_OP; }
"!="                  { return NEQ_OP; }
\+                  { return '+'; }
\-                  { return '-'; }
\*                  { return '*'; }
\/                  { return '/'; }
\^                  { return '^'; }
\%                  { return '%'; }
\(                  { return '('; }
\)                  { return ')'; }
\{                  { return '{'; }
\}                  { return '}'; }
"?"                  { return '?'; }
":"                  { return ':'; }
"="                   { return '='; }
">"                   { return '>'; }
"<"                   { return '<'; }
or                  { return OR_OP; }
and                  { return AND_OP; }

print                  { return PRINT; }
return                  { return RETURN; }
def                  { return DEF; }

0[xX]{H}+           { yylval.i = strtol(yytext, NULL, 16); return INT; }
0[1-7]+             { yylval.i = strtol(yytext, NULL, 8); return INT; }
{D}+                { yylval.i = atoi(yytext); return INT; }

{D}+{E}             { yylval.d = atof(yytext); return FLOAT; }
{D}*"."{D}+({E})?   { yylval.d = atof(yytext); return FLOAT; }
{D}+"."{D}*({E})?   { yylval.d = atof(yytext); return FLOAT; }

{L}({L}|{D})*           { yylval.s = strdup(yytext); return IDENTIFIER; }

\n                  return '\n';
[ \t]+              ;/* ignore whitespace */;
%%

int yywrap()
{
    return 1;
}
