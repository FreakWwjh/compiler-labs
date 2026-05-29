%{
#include <stdio.h>
#include <stdlib.h>

void yyerror(const char *s);
int yylex(void);
%}

%union {
    int num;
}

%token <num> NUMBER
%token PLUS MINUS MUL DIV LPAREN RPAREN
%type <num> expr term factor

%left PLUS MINUS
%left MUL DIV

%%

input:
    /* empty */
    | input line
    ;

line:
    expr '\n'           { printf("= %d\n", $1); }
    | '\n'              { /* empty line */ }
    | error '\n'        { yyerrok; printf("Error: invalid expression\n"); }
    ;

expr:
    expr PLUS term      { $$ = $1 + $3; }
    | expr MINUS term   { $$ = $1 - $3; }
    | term              { $$ = $1; }
    ;

term:
    term MUL factor     { $$ = $1 * $3; }
    | term DIV factor   { $$ = $1 / $3; }
    | factor            { $$ = $1; }
    ;

factor:
    NUMBER              { $$ = $1; }
    | LPAREN expr RPAREN { $$ = $2; }
    ;

%%

void yyerror(const char *s) {
    /* error handling is done in grammar rules */ 
}

int main(int argc, char **argv) {
    printf("===== Flex/Bison Calculator Demo =====\n");
    printf("Enter arithmetic expressions (Ctrl+D to exit):\n");
    yyparse();
    printf("Done.\n");
    return 0;
}
