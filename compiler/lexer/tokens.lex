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

    void multiple_newlines();
    void remove_comments();
    void count_col();
    int column = 0;
%}

%option yylineno

%%
or                    { count_col(); return OR_OP; }
and                   { count_col(); return AND_OP; }
print                 { count_col(); return PRINT; }
return                { count_col(); return RETURN; }
def                   { count_col(); return DEF; }
if                    { count_col(); return IF; }
else                  { count_col(); return ELSE; }
for                   { count_col(); return FOR; }

0[xX]{H}+             { count_col(); yylval.integer = strtol(yytext, NULL, 16); return INT; }
0[1-7]+               { count_col(); yylval.integer = strtol(yytext, NULL, 8); return INT; }
-?{D}+                { count_col(); yylval.integer = atoi(yytext); return INT; }

{D}+{E}               { count_col(); yylval.real = atof(yytext); return FLOAT; }
{D}*"."{D}+({E})?     { count_col(); yylval.real = atof(yytext); return FLOAT; }
{D}+"."{D}*({E})?     { count_col(); yylval.real = atof(yytext); return FLOAT; }

{L}({L}|{D})*         { count_col(); yylval.str = strdup(yytext); return IDENTIFIER; }

"/*"                  { remove_comments(); }
"<="                  { count_col(); return LE_OP; }
">="                  { count_col(); return GE_OP; }
"=="                  { count_col(); return EQ_OP; }
"!="                  { count_col(); return NE_OP; }
\+                    { count_col(); return '+'; }
\-                    { count_col(); return '-'; }
\*                    { count_col(); return '*'; }
\/                    { count_col(); return '/'; }
\^                    { count_col(); return '^'; }
\%                    { count_col(); return '%'; }
\(                    { count_col(); return '('; }
\)                    { count_col(); return ')'; }
\{                    { count_col(); return '{'; }
\}                    { count_col(); return '}'; }
"?"                   { count_col(); return '?'; }
":"                   { count_col(); return ':'; }
"="                   { count_col(); return '='; }
">"                   { count_col(); return '>'; }
"<"                   { count_col(); return '<'; }
","                   { count_col(); return ','; }
";"                   { count_col(); return ';'; }

\n                    { count_col(); multiple_newlines(); return '\n';}
[ \t]+              ;/* ignore whitespace */;
%%

int yywrap()
{
    return 1;
}

void multiple_newlines()
{
    char new_line;
    do new_line = input();
    while (new_line == ' ' || new_line == '\n' || new_line == '\t');
    if (new_line > 0)
        unput(new_line);
}

void remove_comments()
{
    char c, c1;

loop:
    while ((c = input()) != '*' && c != 0);

    if ((c1 = input()) != '/' && c != 0)
    {
        unput(c1);
        goto loop;
    }
}

void count_col()
{
    int i;

    for (i = 0; yytext[i] != '\0'; i++)
        if (yytext[i] == '\n')
            column = 0;
        else if (yytext[i] == '\t')
            column += 8 - (column % 8);
        else
            column++;
}
