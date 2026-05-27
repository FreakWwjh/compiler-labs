#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <unordered_map>
#include <iomanip>

enum class TokenType {
    // Keywords
    KEY_INT, KEY_FLOAT, KEY_VOID, KEY_IF, KEY_ELSE,
    KEY_WHILE, KEY_RETURN, KEY_INPUT, KEY_PRINT,
    // Identifiers & Literals
    ID, NUM, FLOAT,
    // Operators
    ADD, SUB, MUL, DIV,
    LT, LE, EQ, GT, GE, NE,
    ASG,
    // Delimiters
    SEMI,     // ;
    LPAR,     // (
    RPAR,     // )
    LBR,      // {
    RBR,      // }
    LBK,      // [
    RBK,      // ]
    CMA,      // ,
    // Special
    END,      // end of input
    UNKNOWN   // unrecognized
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int col;
};

std::string tokenTypeToString(TokenType type, bool simple = false) {
    switch (type) {
        case TokenType::KEY_INT:    return simple ? "INT"    : "KEY_INT";
        case TokenType::KEY_FLOAT:  return simple ? "FLOAT"  : "KEY_FLOAT";
        case TokenType::KEY_VOID:   return simple ? "VOID"   : "KEY_VOID";
        case TokenType::KEY_IF:     return simple ? "IF"     : "KEY_IF";
        case TokenType::KEY_ELSE:   return simple ? "ELSE"   : "KEY_ELSE";
        case TokenType::KEY_WHILE:  return simple ? "WHILE"  : "KEY_WHILE";
        case TokenType::KEY_RETURN: return simple ? "RETURN" : "KEY_RETURN";
        case TokenType::KEY_INPUT:  return simple ? "INPUT"  : "KEY_INPUT";
        case TokenType::KEY_PRINT:  return simple ? "PRINT"  : "KEY_PRINT";
        case TokenType::ID:         return "ID";
        case TokenType::NUM:        return "NUM";
        case TokenType::FLOAT:      return "FLOAT";
        case TokenType::ADD:        return "ADD";
        case TokenType::SUB:        return "SUB";
        case TokenType::MUL:        return "MUL";
        case TokenType::DIV:        return "DIV";
        case TokenType::LT:         return "LT";
        case TokenType::LE:         return "LE";
        case TokenType::EQ:         return "EQ";
        case TokenType::GT:         return "GT";
        case TokenType::GE:         return "GE";
        case TokenType::NE:         return "NE";
        case TokenType::ASG:        return "ASG";
        case TokenType::SEMI:       return simple ? "SEMI"  : "SEMI";
        case TokenType::LPAR:       return simple ? "LPAR"  : "LPAR";
        case TokenType::RPAR:       return simple ? "RPAR"  : "RPAR";
        case TokenType::LBR:        return simple ? "LBR"   : "LBR";
        case TokenType::RBR:        return simple ? "RBR"   : "RBR";
        case TokenType::LBK:        return simple ? "LBK"   : "LBK";
        case TokenType::RBK:        return simple ? "RBK"   : "RBK";
        case TokenType::CMA:        return simple ? "CMA"   : "CMA";
        case TokenType::END:        return "END";
        case TokenType::UNKNOWN:    return "UNKNOWN";
    }
    return "UNKNOWN";
}

class Scanner {
public:
    explicit Scanner(std::istream& in) : input(in), line(1), col(1), current(0) {
        nextChar();
    }

    Token nextToken() {
        skipWhitespace();
        if (current == EOF || current == '\0') {
            return makeToken(TokenType::END, "");
        }

        int startLine = line;
        int startCol = col;

        // Identifier or keyword
        if (std::isalpha(static_cast<unsigned char>(current)) || current == '_') {
            std::string lexeme;
            while (std::isalnum(static_cast<unsigned char>(current)) || current == '_') {
                lexeme.push_back(static_cast<char>(current));
                nextChar();
            }
            TokenType type = lookupKeyword(lexeme);
            return Token{type, lexeme, startLine, startCol};
        }

        // Number (integer or float)
        if (std::isdigit(static_cast<unsigned char>(current))) {
            return readNumber(startLine, startCol);
        }

        // Dot leading float like .66
        if (current == '.') {
            return readNumber(startLine, startCol);
        }

        // Operators and delimiters
        char c = current;
        nextChar();

        switch (c) {
            case '+': return Token{TokenType::ADD, "+", startLine, startCol};
            case '-': return Token{TokenType::SUB, "-", startLine, startCol};
            case '*': return Token{TokenType::MUL, "*", startLine, startCol};
            case '/': return Token{TokenType::DIV, "/", startLine, startCol};
            case ';': return Token{TokenType::SEMI, ";", startLine, startCol};
            case '(': return Token{TokenType::LPAR, "(", startLine, startCol};
            case ')': return Token{TokenType::RPAR, ")", startLine, startCol};
            case '{': return Token{TokenType::LBR, "{", startLine, startCol};
            case '}': return Token{TokenType::RBR, "}", startLine, startCol};
            case '[': return Token{TokenType::LBK, "[", startLine, startCol};
            case ']': return Token{TokenType::RBK, "]", startLine, startCol};
            case ',': return Token{TokenType::CMA, ",", startLine, startCol};
            case '=':
                if (current == '=') {
                    nextChar();
                    return Token{TokenType::EQ, "==", startLine, startCol};
                }
                return Token{TokenType::ASG, "=", startLine, startCol};
            case '<':
                if (current == '=') {
                    nextChar();
                    return Token{TokenType::LE, "<=", startLine, startCol};
                }
                return Token{TokenType::LT, "<", startLine, startCol};
            case '>':
                if (current == '=') {
                    nextChar();
                    return Token{TokenType::GE, ">=", startLine, startCol};
                }
                return Token{TokenType::GT, ">", startLine, startCol};
            case '!':
                if (current == '=') {
                    nextChar();
                    return Token{TokenType::NE, "!=", startLine, startCol};
                }
                // standalone '!' not supported by grammar, treat as unknown
                return Token{TokenType::UNKNOWN, "!", startLine, startCol};
            default:
                return Token{TokenType::UNKNOWN, std::string(1, c), startLine, startCol};
        }
    }

    std::vector<Token> scanAll() {
        std::vector<Token> tokens;
        Token tok;
        do {
            tok = nextToken();
            tokens.push_back(tok);
        } while (tok.type != TokenType::END);
        return tokens;
    }

private:
    std::istream& input;
    int line;
    int col;
    int current;

    void nextChar() {
        current = input.get();
        if (current == '\n') {
            line++;
            col = 1;
        } else if (current != EOF) {
            col++;
        }
    }

    void skipWhitespace() {
        while (current == ' ' || current == '\t' || current == '\n' || current == '\r') {
            nextChar();
        }
    }

    Token makeToken(TokenType type, const std::string& value) {
        return Token{type, value, line, col};
    }

    TokenType lookupKeyword(const std::string& s) {
        static const std::unordered_map<std::string, TokenType> keywords = {
            {"int", TokenType::KEY_INT},
            {"float", TokenType::KEY_FLOAT},
            {"void", TokenType::KEY_VOID},
            {"if", TokenType::KEY_IF},
            {"else", TokenType::KEY_ELSE},
            {"while", TokenType::KEY_WHILE},
            {"return", TokenType::KEY_RETURN},
            {"input", TokenType::KEY_INPUT},
            {"print", TokenType::KEY_PRINT}
        };
        auto it = keywords.find(s);
        if (it != keywords.end()) return it->second;
        return TokenType::ID;
    }

    Token readNumber(int startLine, int startCol) {
        std::string lexeme;
        bool isFloat = false;
        bool hasDigitsBeforeDot = false;
        bool hasDigitsAfterDot = false;
        bool hasExponent = false;

        // digits before dot
        while (std::isdigit(static_cast<unsigned char>(current))) {
            lexeme.push_back(static_cast<char>(current));
            hasDigitsBeforeDot = true;
            nextChar();
        }

        // fractional part
        if (current == '.') {
            isFloat = true;
            lexeme.push_back('.');
            nextChar();
            while (std::isdigit(static_cast<unsigned char>(current))) {
                lexeme.push_back(static_cast<char>(current));
                hasDigitsAfterDot = true;
                nextChar();
            }
        }

        // exponent part
        if (current == 'e' || current == 'E') {
            isFloat = true;
            lexeme.push_back(static_cast<char>(current));
            nextChar();
            if (current == '+' || current == '-') {
                lexeme.push_back(static_cast<char>(current));
                nextChar();
            }
            bool expDigits = false;
            while (std::isdigit(static_cast<unsigned char>(current))) {
                lexeme.push_back(static_cast<char>(current));
                expDigits = true;
                nextChar();
            }
            if (!expDigits) {
                // malformed exponent, but still return as float/unknown
                // For simplicity, return what we have
                return Token{TokenType::UNKNOWN, lexeme, startLine, startCol};
            }
            hasExponent = true;
        }

        if (isFloat) {
            // valid float: digits.digits, .digits, digits. , or any with exponent
            if (hasDigitsBeforeDot || hasDigitsAfterDot || hasExponent) {
                return Token{TokenType::FLOAT, lexeme, startLine, startCol};
            }
            return Token{TokenType::UNKNOWN, lexeme, startLine, startCol};
        }

        if (hasDigitsBeforeDot) {
            return Token{TokenType::NUM, lexeme, startLine, startCol};
        }

        // standalone dot
        return Token{TokenType::UNKNOWN, lexeme, startLine, startCol};
    }
};

void printTokenSimple(const Token& tok) {
    std::cout << tokenTypeToString(tok.type, true) << "\n";
}

void printTokenDetailed(const Token& tok) {
    std::cout << "(" << tokenTypeToString(tok.type, false) << ", " << tok.value << ")\n";
}

void printTokenWithPos(const Token& tok) {
    std::cout << "<" << tokenTypeToString(tok.type, false) << ", \"" << tok.value << "\", "
              << tok.line << ":" << tok.col << ">\n";
}

void mode1() {
    int n;
    if (!(std::cin >> n)) return;
    std::vector<std::string> words;
    words.reserve(n);
    for (int i = 0; i < n; ++i) {
        std::string w;
        std::cin >> w;
        words.push_back(w);
    }

    for (const auto& w : words) {
        std::istringstream iss(w + "\n");
        Scanner scanner(iss);
        Token tok = scanner.nextToken();
        if (tok.type == TokenType::END || tok.type == TokenType::UNKNOWN) {
            std::cout << "UNKNOWN\n";
        } else {
            printTokenSimple(tok);
        }
    }
}

void mode2() {
    std::string line;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::getline(std::cin, line);
    std::istringstream iss(line + "\n");
    Scanner scanner(iss);
    auto tokens = scanner.scanAll();
    for (const auto& tok : tokens) {
        if (tok.type == TokenType::END) continue;
        printTokenSimple(tok);
    }
}

void processFile(const std::string& filename) {
    std::ifstream fin(filename);
    if (!fin) {
        std::cerr << "Error: Cannot open file " << filename << "\n";
        std::exit(1);
    }
    Scanner scanner(fin);
    auto tokens = scanner.scanAll();
    for (const auto& tok : tokens) {
        if (tok.type == TokenType::END) continue;
        printTokenDetailed(tok);
    }
}

void printUsage(const char* prog) {
    std::cerr << "Usage:\n"
              << "  " << prog << "               Interactive mode (choose 1 or 2)\n"
              << "  " << prog << " <filename>   Scan source file and output tokens\n";
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        printUsage(argv[0]);
        return 1;
    }

    if (argc == 2) {
        processFile(argv[1]);
        return 0;
    }

    std::cout << "Scanner - Lexical Analyzer\n";
    std::cout << "Select mode:\n";
    std::cout << "  1 - Tokenize n individual words\n";
    std::cout << "  2 - Tokenize a statement line\n";
    std::cout << "Mode: ";
    int mode;
    if (!(std::cin >> mode)) {
        return 0;
    }

    if (mode == 1) {
        mode1();
    } else if (mode == 2) {
        mode2();
    } else {
        std::cerr << "Invalid mode.\n";
        return 1;
    }

    return 0;
}
