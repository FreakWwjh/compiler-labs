#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>

// Simple JSON parser subset for DFA files
class SimpleJson {
public:
    std::map<std::string, std::string> strings;
    std::map<std::string, std::vector<std::string>> arrays;
    std::map<std::string, std::map<std::string, std::string>> objects;
    std::map<std::string, std::map<std::string, std::map<std::string, std::string>>> nested_objects;

    bool parse(const std::string& text) {
        size_t pos = 0;
        skipWhitespace(text, pos);
        if (pos >= text.size() || text[pos] != '{') return false;
        pos++;
        while (true) {
            skipWhitespace(text, pos);
            if (pos >= text.size()) return false;
            if (text[pos] == '}') { pos++; break; }
            std::string key = parseString(text, pos);
            if (key.empty()) return false;
            skipWhitespace(text, pos);
            if (pos >= text.size() || text[pos] != ':') return false;
            pos++;
            skipWhitespace(text, pos);
            if (pos >= text.size()) return false;

            if (text[pos] == '"') {
                std::string val = parseString(text, pos);
                strings[key] = val;
            } else if (text[pos] == '[') {
                auto arr = parseArray(text, pos);
                arrays[key] = arr;
            } else if (text[pos] == '{') {
                // Try nested object first (transitions), then flat object (accept_states)
                size_t tryPos = pos;
                auto nested = parseNestedObject(text, tryPos);
                if (!nested.empty()) {
                    nested_objects[key] = nested;
                    pos = tryPos;
                } else {
                    auto obj = parseFlatObject(text, pos);
                    objects[key] = obj;
                }
            } else {
                // number or literal
                size_t start = pos;
                while (pos < text.size() && !isspace(text[pos]) && text[pos] != ',' && text[pos] != '}') pos++;
                strings[key] = text.substr(start, pos - start);
            }

            skipWhitespace(text, pos);
            if (pos < text.size() && text[pos] == ',') { pos++; continue; }
            else if (pos < text.size() && text[pos] == '}') { pos++; break; }
            else return false;
        }
        return true;
    }

private:
    void skipWhitespace(const std::string& s, size_t& pos) {
        while (pos < s.size() && isspace(s[pos])) pos++;
    }

    std::string parseString(const std::string& s, size_t& pos) {
        if (s[pos] != '"') return "";
        pos++;
        std::string res;
        while (pos < s.size() && s[pos] != '"') {
            if (s[pos] == '\\' && pos + 1 < s.size()) {
                pos++;
                switch (s[pos]) {
                    case 'n': res.push_back('\n'); break;
                    case 't': res.push_back('\t'); break;
                    case 'r': res.push_back('\r'); break;
                    default: res.push_back(s[pos]); break;
                }
            } else {
                res.push_back(s[pos]);
            }
            pos++;
        }
        if (pos < s.size() && s[pos] == '"') pos++;
        return res;
    }

    std::vector<std::string> parseArray(const std::string& s, size_t& pos) {
        std::vector<std::string> res;
        if (s[pos] != '[') return res;
        pos++;
        while (true) {
            skipWhitespace(s, pos);
            if (pos >= s.size()) break;
            if (s[pos] == ']') { pos++; break; }
            if (s[pos] == '"') {
                res.push_back(parseString(s, pos));
            } else {
                size_t start = pos;
                while (pos < s.size() && s[pos] != ',' && s[pos] != ']' && !isspace(s[pos])) pos++;
                res.push_back(s.substr(start, pos - start));
            }
            skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == ',') { pos++; continue; }
            else if (pos < s.size() && s[pos] == ']') { pos++; break; }
            else break;
        }
        return res;
    }

    std::map<std::string, std::string> parseFlatObject(const std::string& s, size_t& pos) {
        std::map<std::string, std::string> res;
        if (s[pos] != '{') return res;
        pos++;
        while (true) {
            skipWhitespace(s, pos);
            if (pos >= s.size()) break;
            if (s[pos] == '}') { pos++; break; }
            std::string key = parseString(s, pos);
            skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == ':') pos++;
            skipWhitespace(s, pos);
            std::string val;
            if (s[pos] == '"') {
                val = parseString(s, pos);
            } else {
                size_t start = pos;
                while (pos < s.size() && s[pos] != ',' && s[pos] != '}' && !isspace(s[pos])) pos++;
                val = s.substr(start, pos - start);
            }
            res[key] = val;
            skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == ',') { pos++; continue; }
            else if (pos < s.size() && s[pos] == '}') { pos++; break; }
            else break;
        }
        return res;
    }

    std::map<std::string, std::map<std::string, std::string>> parseNestedObject(const std::string& s, size_t& pos) {
        std::map<std::string, std::map<std::string, std::string>> res;
        if (s[pos] != '{') return res;
        size_t saved = pos;
        pos++;
        while (true) {
            skipWhitespace(s, pos);
            if (pos >= s.size()) { pos = saved; return {}; }
            if (s[pos] == '}') { pos++; break; }
            std::string key = parseString(s, pos);
            skipWhitespace(s, pos);
            if (pos >= s.size() || s[pos] != ':') { pos = saved; return {}; }
            pos++;
            skipWhitespace(s, pos);
            if (pos >= s.size() || s[pos] != '{') { pos = saved; return {}; }
            auto inner = parseFlatObject(s, pos);
            res[key] = inner;
            skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == ',') { pos++; continue; }
            else if (pos < s.size() && s[pos] == '}') { pos++; break; }
            else { pos = saved; return {}; }
        }
        return res;
    }
};

struct DFA {
    std::vector<std::string> alphabet;
    std::vector<std::string> states;
    std::string startState;
    std::map<std::string, std::string> acceptStates; // state -> token type
    std::map<std::string, std::map<std::string, std::string>> transitions; // state -> (sym -> next)
};

bool loadDFA(const std::string& filename, DFA& dfa) {
    std::ifstream fin(filename);
    if (!fin) {
        std::cerr << "Error: Cannot open DFA file " << filename << std::endl;
        return false;
    }
    std::stringstream buffer;
    buffer << fin.rdbuf();
    std::string text = buffer.str();
    fin.close();

    SimpleJson json;
    if (!json.parse(text)) {
        std::cerr << "Error: Failed to parse DFA JSON.\n";
        return false;
    }

    if (json.arrays.count("alphabet")) dfa.alphabet = json.arrays["alphabet"];
    if (json.arrays.count("states")) dfa.states = json.arrays["states"];
    if (json.strings.count("start_state")) dfa.startState = json.strings["start_state"];

    // accept_states may be array or object
    if (json.objects.count("accept_states")) {
        dfa.acceptStates = json.objects["accept_states"];
    } else if (json.arrays.count("accept_states")) {
        for (const auto& s : json.arrays["accept_states"]) {
            dfa.acceptStates[s] = "ACCEPT";
        }
    }

    if (json.nested_objects.count("transitions")) {
        dfa.transitions = json.nested_objects["transitions"];
    }

    return !dfa.states.empty() && !dfa.startState.empty();
}

struct Token {
    std::string type;
    std::string value;
    int line;
    int col;
};

class DFAScanner {
public:
    DFAScanner(const DFA& d, const std::string& src) : dfa(d), source(src), pos(0), line(1), col(1) {}

    std::vector<Token> scanAll() {
        std::vector<Token> tokens;
        while (pos < source.size()) {
            skipWhitespace();
            if (pos >= source.size()) break;

            int startLine = line;
            int startCol = col;

            std::string state = dfa.startState;
            size_t lastAcceptPos = std::string::npos;
            std::string lastAcceptType;
            size_t lastAcceptLen = 0;

            size_t i = pos;
            int curLine = line;
            int curCol = col;

            while (i < source.size()) {
                // Check if current state is accept
                if (dfa.acceptStates.count(state)) {
                    lastAcceptPos = i;
                    lastAcceptType = dfa.acceptStates.at(state);
                    lastAcceptLen = i - pos;
                }

                std::string ch = source.substr(i, 1);
                auto itState = dfa.transitions.find(state);
                if (itState == dfa.transitions.end()) break;
                auto itNext = itState->second.find(ch);
                if (itNext == itState->second.end()) break;

                state = itNext->second;
                i++;
                if (ch[0] == '\n') {
                    curLine++;
                    curCol = 1;
                } else {
                    curCol++;
                }
            }

            // Check accept at the very end of run
            if (i <= source.size() && dfa.acceptStates.count(state)) {
                lastAcceptPos = i;
                lastAcceptType = dfa.acceptStates.at(state);
                lastAcceptLen = i - pos;
            }

            if (lastAcceptPos == std::string::npos) {
                // No accept state found
                std::string bad = source.substr(pos, 1);
                tokens.push_back({"UNKNOWN", bad, startLine, startCol});
                advancePos();
            } else {
                std::string val = source.substr(pos, lastAcceptLen);
                tokens.push_back({lastAcceptType, val, startLine, startCol});
                // Advance to lastAcceptPos
                while (pos < lastAcceptPos) advancePos();
            }
        }
        return tokens;
    }

private:
    const DFA& dfa;
    const std::string& source;
    size_t pos;
    int line;
    int col;

    void skipWhitespace() {
        while (pos < source.size() && isspace(static_cast<unsigned char>(source[pos]))) {
            advancePos();
        }
    }

    void advancePos() {
        if (pos >= source.size()) return;
        if (source[pos] == '\n') {
            line++;
            col = 1;
        } else {
            col++;
        }
        pos++;
    }
};

const char* keywordCheck(const std::string& type, const std::string& value) {
    static const std::map<std::string, std::string> kw = {
        {"int", "INT"}, {"float", "FLOAT"}, {"void", "VOID"}, {"if", "IF"},
        {"else", "ELSE"}, {"while", "WHILE"}, {"return", "RETURN"},
        {"input", "INPUT"}, {"print", "PRINT"}
    };
    if (type == "ID" && kw.count(value)) return kw.at(value).c_str();
    return type.c_str();
}

void processFile(const std::string& dfaFile, const std::string& srcFile) {
    DFA dfa;
    if (!loadDFA(dfaFile, dfa)) {
        std::cerr << "Failed to load DFA.\n";
        return;
    }

    std::ifstream fin(srcFile);
    if (!fin) {
        std::cerr << "Error: Cannot open source file " << srcFile << std::endl;
        return;
    }
    std::stringstream buffer;
    buffer << fin.rdbuf();
    std::string source = buffer.str();
    fin.close();

    DFAScanner scanner(dfa, source);
    auto tokens = scanner.scanAll();
    for (const auto& tok : tokens) {
        const char* t = keywordCheck(tok.type, tok.value);
        std::cout << "(" << t << ", " << tok.value << ")\n";
    }
}

void interactive(const std::string& dfaFile) {
    DFA dfa;
    if (!loadDFA(dfaFile, dfa)) {
        std::cerr << "Failed to load DFA.\n";
        return;
    }

    std::cout << "DFA loaded: " << dfa.states.size() << " states, " << dfa.alphabet.size() << " symbols.\n";
    std::cout << "Enter source code (EOF to finish):\n";
    std::stringstream buffer;
    std::string line;
    while (std::getline(std::cin, line)) {
        buffer << line << '\n';
    }
    std::string source = buffer.str();

    DFAScanner scanner(dfa, source);
    auto tokens = scanner.scanAll();
    for (const auto& tok : tokens) {
        const char* t = keywordCheck(tok.type, tok.value);
        std::cout << t << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage:\n"
                  << "  " << argv[0] << " <dfa.json> [<source.src>]   # File mode\n"
                  << "  " << argv[0] << " <dfa.json>                 # Interactive mode\n";
        return 1;
    }

    std::string dfaFile = argv[1];
    if (argc >= 3) {
        processFile(dfaFile, argv[2]);
    } else {
        interactive(dfaFile);
    }
    return 0;
}
