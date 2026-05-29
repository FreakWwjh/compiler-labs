%{
#include "calc.tab.h"
#include <stdlib.h>
%}

%option noyywrap

%%

[0-9]+      { yylval.num = atoi(yytext); return NUMBER; }
[ \t]+      { /* ignore whitespace */ }
\n          { return '\n'; }
"+"         { return PLUS; }
"-"         { return MINUS; }
"*"         { return MUL; }
"/"         { return DIV; }
"("         { return LPAREN; }
")"         { return RPAREN; }
.           { return yytext[0]; }

%%
