#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

typedef enum {
    TOK_EOF = 0,
    // Keywords
    TOK_INT, TOK_FLOAT, TOK_VOID, TOK_IF, TOK_ELSE,
    TOK_WHILE, TOK_RETURN, TOK_INPUT, TOK_PRINT,
    // Identifiers / Literals
    TOK_ID, TOK_NUM, TOK_FLT,
    // Operators
    TOK_ADD, TOK_SUB, TOK_MUL, TOK_DIV,
    TOK_LT, TOK_LE, TOK_EQ, TOK_GT, TOK_GE, TOK_NE,
    TOK_ASG,
    // Delimiters
    TOK_SEMI, TOK_LPAR, TOK_RPAR, TOK_LBR, TOK_RBR,
    TOK_LBK, TOK_RBK, TOK_CMA,
    TOK_UNKNOWN
} TokenType;

const char* token_name(TokenType t) {
    switch (t) {
        case TOK_INT: return "INT";
        case TOK_FLOAT: return "FLOAT";
        case TOK_VOID: return "VOID";
        case TOK_IF: return "IF";
        case TOK_ELSE: return "ELSE";
        case TOK_WHILE: return "WHILE";
        case TOK_RETURN: return "RETURN";
        case TOK_INPUT: return "INPUT";
        case TOK_PRINT: return "PRINT";
        case TOK_ID: return "ID";
        case TOK_NUM: return "NUM";
        case TOK_FLT: return "FLOAT";
        case TOK_ADD: return "ADD";
        case TOK_SUB: return "SUB";
        case TOK_MUL: return "MUL";
        case TOK_DIV: return "DIV";
        case TOK_LT: return "LT";
        case TOK_LE: return "LE";
        case TOK_EQ: return "EQ";
        case TOK_GT: return "GT";
        case TOK_GE: return "GE";
        case TOK_NE: return "NE";
        case TOK_ASG: return "ASG";
        case TOK_SEMI: return "SEMI";
        case TOK_LPAR: return "LPAR";
        case TOK_RPAR: return "RPAR";
        case TOK_LBR: return "LBR";
        case TOK_RBR: return "RBR";
        case TOK_LBK: return "LBK";
        case TOK_RBK: return "RBK";
        case TOK_CMA: return "CMA";
        case TOK_EOF: return "EOF";
        default: return "UNKNOWN";
    }
}

typedef struct {
    TokenType type;
    char value[256];
    int line;
    int col;
} Token;

typedef struct {
    FILE* fp;
    int line;
    int col;
    int current;
    int unget;
} Scanner;

void scanner_init(Scanner* s, FILE* fp) {
    s->fp = fp;
    s->line = 1;
    s->col = 1;
    s->current = fgetc(fp);
    s->unget = 0;
}

static void next_char(Scanner* s) {
    s->current = fgetc(s->fp);
    if (s->current == '\n') {
        s->line++;
        s->col = 1;
    } else if (s->current != EOF) {
        s->col++;
    }
}

static int is_keyword(const char* s, TokenType* out) {
    if (strcmp(s, "int") == 0) { *out = TOK_INT; return 1; }
    if (strcmp(s, "float") == 0) { *out = TOK_FLOAT; return 1; }
    if (strcmp(s, "void") == 0) { *out = TOK_VOID; return 1; }
    if (strcmp(s, "if") == 0) { *out = TOK_IF; return 1; }
    if (strcmp(s, "else") == 0) { *out = TOK_ELSE; return 1; }
    if (strcmp(s, "while") == 0) { *out = TOK_WHILE; return 1; }
    if (strcmp(s, "return") == 0) { *out = TOK_RETURN; return 1; }
    if (strcmp(s, "input") == 0) { *out = TOK_INPUT; return 1; }
    if (strcmp(s, "print") == 0) { *out = TOK_PRINT; return 1; }
    return 0;
}

Token next_token(Scanner* s) {
    Token tok = {TOK_UNKNOWN, "", s->line, s->col};

    // skip whitespace
    while (s->current == ' ' || s->current == '\t' || s->current == '\n' || s->current == '\r') {
        next_char(s);
    }

    if (s->current == EOF) {
        tok.type = TOK_EOF;
        return tok;
    }

    int start_line = s->line;
    int start_col = s->col;
    tok.line = start_line;
    tok.col = start_col;

    // identifier or keyword
    if (isalpha(s->current) || s->current == '_') {
        int i = 0;
        while (isalnum(s->current) || s->current == '_') {
            if (i < 255) tok.value[i++] = (char)s->current;
            next_char(s);
        }
        tok.value[i] = '\0';
        if (!is_keyword(tok.value, &tok.type)) {
            tok.type = TOK_ID;
        }
        return tok;
    }

    // number
    if (isdigit(s->current) || s->current == '.') {
        int i = 0;
        int has_digits_before = 0, has_dot = 0, has_digits_after = 0, has_exp = 0;

        while (isdigit(s->current)) {
            if (i < 255) tok.value[i++] = (char)s->current;
            has_digits_before = 1;
            next_char(s);
        }

        if (s->current == '.') {
            has_dot = 1;
            if (i < 255) tok.value[i++] = (char)s->current;
            next_char(s);
            while (isdigit(s->current)) {
                if (i < 255) tok.value[i++] = (char)s->current;
                has_digits_after = 1;
                next_char(s);
            }
        }

        if (s->current == 'e' || s->current == 'E') {
            has_exp = 1;
            if (i < 255) tok.value[i++] = (char)s->current;
            next_char(s);
            if (s->current == '+' || s->current == '-') {
                if (i < 255) tok.value[i++] = (char)s->current;
                next_char(s);
            }
            int exp_digits = 0;
            while (isdigit(s->current)) {
                if (i < 255) tok.value[i++] = (char)s->current;
                exp_digits = 1;
                next_char(s);
            }
            if (!exp_digits) {
                tok.value[i] = '\0';
                tok.type = TOK_UNKNOWN;
                return tok;
            }
        }

        tok.value[i] = '\0';
        if (has_dot || has_exp) {
            if (has_digits_before || has_digits_after || has_exp) {
                tok.type = TOK_FLT;
            } else {
                tok.type = TOK_UNKNOWN;
            }
        } else {
            tok.type = TOK_NUM;
        }
        return tok;
    }

    // single char tokens
    char c = (char)s->current;
    next_char(s);

    switch (c) {
        case '+': tok.type = TOK_ADD; strcpy(tok.value, "+"); break;
        case '-': tok.type = TOK_SUB; strcpy(tok.value, "-"); break;
        case '*': tok.type = TOK_MUL; strcpy(tok.value, "*"); break;
        case '/': tok.type = TOK_DIV; strcpy(tok.value, "/"); break;
        case ';': tok.type = TOK_SEMI; strcpy(tok.value, ";"); break;
        case '(': tok.type = TOK_LPAR; strcpy(tok.value, "("); break;
        case ')': tok.type = TOK_RPAR; strcpy(tok.value, ")"); break;
        case '{': tok.type = TOK_LBR; strcpy(tok.value, "{"); break;
        case '}': tok.type = TOK_RBR; strcpy(tok.value, "}"); break;
        case '[': tok.type = TOK_LBK; strcpy(tok.value, "["); break;
        case ']': tok.type = TOK_RBK; strcpy(tok.value, "]"); break;
        case ',': tok.type = TOK_CMA; strcpy(tok.value, ","); break;
        case '=':
            if (s->current == '=') {
                next_char(s);
                tok.type = TOK_EQ; strcpy(tok.value, "==");
            } else {
                tok.type = TOK_ASG; strcpy(tok.value, "=");
            }
            break;
        case '<':
            if (s->current == '=') {
                next_char(s);
                tok.type = TOK_LE; strcpy(tok.value, "<=");
            } else {
                tok.type = TOK_LT; strcpy(tok.value, "<");
            }
            break;
        case '>':
            if (s->current == '=') {
                next_char(s);
                tok.type = TOK_GE; strcpy(tok.value, ">=");
            } else {
                tok.type = TOK_GT; strcpy(tok.value, ">");
            }
            break;
        case '!':
            if (s->current == '=') {
                next_char(s);
                tok.type = TOK_NE; strcpy(tok.value, "!=");
            } else {
                tok.type = TOK_UNKNOWN;
                tok.value[0] = '!'; tok.value[1] = '\0';
            }
            break;
        default:
            tok.type = TOK_UNKNOWN;
            tok.value[0] = c; tok.value[1] = '\0';
            break;
    }
    return tok;
}

void print_simple(const Token* tok) {
    printf("%s\n", token_name(tok->type));
}

void print_detailed(const Token* tok) {
    printf("(%s, %s)\n", token_name(tok->type), tok->value);
}

void mode1() {
    int n;
    if (scanf("%d", &n) != 1) return;
    char buf[1024];
    for (int i = 0; i < n; i++) {
        if (scanf("%255s", buf) != 1) break;
        FILE* mem = tmpfile();
        if (!mem) { printf("UNKNOWN\n"); continue; }
        fprintf(mem, "%s\n", buf);
        rewind(mem);
        Scanner sc;
        scanner_init(&sc, mem);
        Token tok = next_token(&sc);
        fclose(mem);
        if (tok.type == TOK_EOF || tok.type == TOK_UNKNOWN) {
            printf("UNKNOWN\n");
        } else {
            print_simple(&tok);
        }
    }
}

void mode2() {
    char line[4096];
    getchar(); // consume newline after mode number
    if (!fgets(line, sizeof(line), stdin)) return;
    FILE* mem = tmpfile();
    if (!mem) return;
    fprintf(mem, "%s", line);
    rewind(mem);
    Scanner sc;
    scanner_init(&sc, mem);
    Token tok;
    do {
        tok = next_token(&sc);
        if (tok.type != TOK_EOF) print_simple(&tok);
    } while (tok.type != TOK_EOF);
    fclose(mem);
}

void process_file(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        exit(1);
    }
    Scanner sc;
    scanner_init(&sc, fp);
    Token tok;
    do {
        tok = next_token(&sc);
        if (tok.type != TOK_EOF) print_detailed(&tok);
    } while (tok.type != TOK_EOF);
    fclose(fp);
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        fprintf(stderr, "Usage:\n  %s              Interactive mode\n  %s <filename>  Scan source file\n", argv[0], argv[0]);
        return 1;
    }

    if (argc == 2) {
        process_file(argv[1]);
        return 0;
    }

    printf("Scanner (C version) - Lexical Analyzer\n");
    printf("Select mode:\n");
    printf("  1 - Tokenize n individual words\n");
    printf("  2 - Tokenize a statement line\n");
    printf("Mode: ");
    int mode;
    if (scanf("%d", &mode) != 1) return 0;

    if (mode == 1) {
        mode1();
    } else if (mode == 2) {
        mode2();
    } else {
        fprintf(stderr, "Invalid mode.\n");
        return 1;
    }
    return 0;
}
