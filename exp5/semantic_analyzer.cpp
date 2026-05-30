#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <stack>
#include <algorithm>
#include <string>
#include <iomanip>
#include <memory>

using namespace std;

// ============================================================
//  Part 1: 复用实验四核心 — 文法与 SLR(1) 分析表生成（精简版）
// ============================================================

struct Production {
    int id;
    string lhs;
    vector<string> rhs;
};

struct LR0Item {
    int prod_id;
    int dot_pos;
    bool operator<(const LR0Item& o) const {
        if (prod_id != o.prod_id) return prod_id < o.prod_id;
        return dot_pos < o.dot_pos;
    }
    bool operator==(const LR0Item& o) const {
        return prod_id == o.prod_id && dot_pos == o.dot_pos;
    }
};

using ItemSet = set<LR0Item>;

struct Transition { int from; string symbol; int to; };

vector<Production> productions;
set<string> non_terminals;
set<string> terminals;
set<string> all_symbols;
string start_symbol;
int augmented_start_prod_id = -1;

map<string, set<string>> first_sets;
map<string, set<string>> follow_sets;

vector<ItemSet> canonical_collection;
vector<Transition> transitions;

vector<map<string, string>> action_table;
vector<map<string, int>> goto_table;

bool isTerminal(const string& sym) {
    return non_terminals.find(sym) == non_terminals.end();
}

bool parseGrammar(const string& filename) {
    ifstream in(filename);
    if (!in.is_open()) { cerr << "Cannot open grammar file: " << filename << endl; return false; }
    string line;
    int prod_counter = 0;
    bool first_prod = true;
    while (getline(in, line)) {
        size_t s = line.find_first_not_of(" \t\r\n");
        if (s == string::npos) continue;
        size_t e = line.find_last_not_of(" \t\r\n");
        line = line.substr(s, e - s + 1);
        if (line.empty() || line[0] == '#') continue;
        size_t ap1 = line.find("->"), ap2 = line.find("→"), ap; int al;
        if (ap1 != string::npos) { ap = ap1; al = 2; }
        else if (ap2 != string::npos) { ap = ap2; al = 3; }
        else continue;
        string lhs = line.substr(0, ap);
        size_t le = lhs.find_last_not_of(" \t");
        if (le != string::npos) lhs = lhs.substr(0, le + 1);
        non_terminals.insert(lhs);
        string rhs_str = line.substr(ap + al);
        size_t rs = rhs_str.find_first_not_of(" \t");
        if (rs != string::npos) rhs_str = rhs_str.substr(rs);
        vector<string> alts;
        size_t pos = 0;
        while (true) {
            size_t bar = rhs_str.find('|', pos);
            string part = (bar == string::npos) ? rhs_str.substr(pos) : rhs_str.substr(pos, bar - pos);
            size_t ps = part.find_first_not_of(" \t");
            size_t pe = part.find_last_not_of(" \t");
            if (ps != string::npos) alts.push_back(part.substr(ps, pe - ps + 1));
            if (bar == string::npos) break;
            pos = bar + 1;
        }
        for (const string& alt : alts) {
            Production p; p.id = prod_counter++; p.lhs = lhs;
            stringstream ss(alt); string sym;
            while (ss >> sym) p.rhs.push_back(sym);
            if (first_prod) { start_symbol = lhs; first_prod = false; }
            productions.push_back(p);
        }
    }
    in.close();
    for (const auto& p : productions)
        for (const auto& s : p.rhs)
            if (non_terminals.find(s) == non_terminals.end()) terminals.insert(s);
    if (productions.empty()) return false;
    string ns = start_symbol + "'";
    while (non_terminals.count(ns)) ns += "'";
    Production aug; aug.id = 0; aug.lhs = ns; aug.rhs.push_back(start_symbol);
    for (auto& p : productions) p.id += 1;
    productions.insert(productions.begin(), aug);
    augmented_start_prod_id = 0;
    non_terminals.insert(ns);
    for (size_t i = 0; i < productions.size(); ++i) productions[i].id = (int)i;
    all_symbols = terminals; all_symbols.insert(non_terminals.begin(), non_terminals.end());
    return true;
}

ItemSet closure(const ItemSet& items) {
    ItemSet res = items;
    bool changed = true;
    while (changed) {
        changed = false;
        ItemSet cur = res;
        for (const auto& item : cur) {
            const Production& p = productions[item.prod_id];
            if (item.dot_pos >= (int)p.rhs.size()) continue;
            string B = p.rhs[item.dot_pos];
            if (isTerminal(B)) continue;
            for (const auto& prod : productions) {
                if (prod.lhs == B) {
                    LR0Item ni{prod.id, 0};
                    if (res.find(ni) == res.end()) { res.insert(ni); changed = true; }
                }
            }
        }
    }
    return res;
}

ItemSet goTo(const ItemSet& items, const string& X) {
    ItemSet J;
    for (const auto& item : items) {
        const Production& p = productions[item.prod_id];
        if (item.dot_pos < (int)p.rhs.size() && p.rhs[item.dot_pos] == X)
            J.insert({item.prod_id, item.dot_pos + 1});
    }
    return closure(J);
}

int findItemSet(const vector<ItemSet>& col, const ItemSet& items) {
    for (size_t i = 0; i < col.size(); ++i) if (col[i] == items) return (int)i;
    return -1;
}

set<string> getSymbols(const ItemSet& items) {
    set<string> syms;
    for (const auto& item : items) {
        const Production& p = productions[item.prod_id];
        if (item.dot_pos < (int)p.rhs.size()) syms.insert(p.rhs[item.dot_pos]);
    }
    return syms;
}

vector<ItemSet> buildCanonicalCollection(vector<Transition>& trans) {
    vector<ItemSet> col; trans.clear();
    ItemSet I0; I0.insert({augmented_start_prod_id, 0}); I0 = closure(I0);
    col.push_back(I0); queue<int> q; q.push(0);
    while (!q.empty()) {
        int idx = q.front(); q.pop();
        set<string> syms = getSymbols(col[idx]);
        for (const string& X : syms) {
            ItemSet J = goTo(col[idx], X);
            if (J.empty()) continue;
            int jdx = findItemSet(col, J);
            if (jdx == -1) { jdx = (int)col.size(); col.push_back(J); q.push(jdx); }
            trans.push_back({idx, X, jdx});
        }
    }
    return col;
}

set<string> firstOfSeq(const vector<string>& seq) {
    set<string> res;
    for (size_t i = 0; i < seq.size(); ++i) {
        if (isTerminal(seq[i])) { res.insert(seq[i]); return res; }
        const auto& f = first_sets[seq[i]];
        for (const string& s : f) if (s != "ε") res.insert(s);
        if (f.find("ε") == f.end()) return res;
    }
    res.insert("ε"); return res;
}

void computeFirst() {
    first_sets.clear();
    for (const string& t : terminals) first_sets[t].insert(t);
    first_sets["ε"].insert("ε");
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& p : productions) {
            const string& A = p.lhs;
            if (p.rhs.empty()) { if (first_sets[A].insert("ε").second) changed = true; continue; }
            for (size_t i = 0; i < p.rhs.size(); ++i) {
                const string& Xi = p.rhs[i];
                if (isTerminal(Xi)) { if (first_sets[A].insert(Xi).second) changed = true; break; }
                const auto& xf = first_sets[Xi];
                for (const string& s : xf) if (s != "ε") if (first_sets[A].insert(s).second) changed = true;
                if (xf.find("ε") == xf.end()) break;
                if (i == p.rhs.size() - 1) if (first_sets[A].insert("ε").second) changed = true;
            }
        }
    }
}

void computeFollow() {
    follow_sets.clear();
    string aug_start = productions[augmented_start_prod_id].lhs;
    follow_sets[aug_start].insert("$");
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& p : productions) {
            const string& A = p.lhs;
            for (size_t i = 0; i < p.rhs.size(); ++i) {
                const string& B = p.rhs[i];
                if (isTerminal(B)) continue;
                if (i + 1 < p.rhs.size()) {
                    vector<string> beta(p.rhs.begin() + i + 1, p.rhs.end());
                    set<string> fb = firstOfSeq(beta);
                    for (const string& s : fb) if (s != "ε") if (follow_sets[B].insert(s).second) changed = true;
                    if (fb.count("ε")) for (const string& s : follow_sets[A]) if (follow_sets[B].insert(s).second) changed = true;
                } else {
                    for (const string& s : follow_sets[A]) if (follow_sets[B].insert(s).second) changed = true;
                }
            }
        }
    }
}

void buildSLRTable() {
    int n = (int)canonical_collection.size();
    action_table.assign(n, map<string, string>());
    goto_table.assign(n, map<string, int>());
    map<pair<int, string>, int> tmap;
    for (const auto& t : transitions) tmap[{t.from, t.symbol}] = t.to;
    for (int i = 0; i < n; ++i) {
        for (const auto& item : canonical_collection[i]) {
            const Production& p = productions[item.prod_id];
            if (item.dot_pos == (int)p.rhs.size()) {
                if (item.prod_id == augmented_start_prod_id) {
                    action_table[i]["$"] = "acc";
                } else {
                    string act = "r" + to_string(item.prod_id);
                    for (const string& a : follow_sets[p.lhs]) action_table[i][a] = act;
                }
            } else {
                string ns = p.rhs[item.dot_pos];
                if (isTerminal(ns)) {
                    auto key = make_pair(i, ns);
                    if (tmap.count(key)) action_table[i][ns] = "s" + to_string(tmap[key]);
                }
            }
        }
    }
    for (const auto& t : transitions) if (!isTerminal(t.symbol)) goto_table[t.from][t.symbol] = t.to;
}

// 从实验四导出的文件加载 SLR 分析表（替代现场生成）
bool loadSLRTable(const string& filename) {
    ifstream in(filename);
    if (!in.is_open()) { cerr << "Cannot open SLR table file: " << filename << endl; return false; }
    action_table.clear(); goto_table.clear();
    string line;
    int max_state = 0;
    while (getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        stringstream ss(line);
        string tag; ss >> tag;
        if (tag == "AUG_START" || tag == "PROD_COUNT" || tag == "STATES") continue;
        if (tag == "ACTION") {
            int state; string sym, act;
            ss >> state >> sym >> act;
            if ((int)action_table.size() <= state) action_table.resize(state + 1);
            action_table[state][sym] = act;
            max_state = max(max_state, state);
        } else if (tag == "GOTO") {
            int state, next; string sym;
            ss >> state >> sym >> next;
            if ((int)goto_table.size() <= state) goto_table.resize(state + 1);
            goto_table[state][sym] = next;
            max_state = max(max_state, state);
        }
    }
    if (action_table.empty() || goto_table.empty()) return false;
    // 统一大小
    if ((int)action_table.size() <= max_state) action_table.resize(max_state + 1);
    if ((int)goto_table.size() <= max_state) goto_table.resize(max_state + 1);
    return true;
}

// ============================================================
//  Part 2: 语义分析基础数据结构
// ============================================================

// ---------- Token ----------
struct Token {
    string type;   // 种属
    string value;  // 个体值
};

vector<Token> loadTokens(const string& filename) {
    vector<Token> tokens;
    ifstream in(filename);
    if (!in.is_open()) { cerr << "Cannot open token file: " << filename << endl; return tokens; }
    string line;
    while (getline(in, line)) {
        size_t s = line.find_first_not_of(" \t\r\n");
        if (s == string::npos) continue;
        size_t e = line.find_last_not_of(" \t\r\n");
        line = line.substr(s, e - s + 1);
        if (line.empty() || line[0] == '#') continue;

        Token tok;
        // 尝试解析实验二输出格式: (TYPE, value) 或 (TYPE,value)
        if (!line.empty() && line.front() == '(' && line.back() == ')') {
            string inner = line.substr(1, line.size() - 2);
            size_t comma = inner.find(',');
            if (comma != string::npos) {
                tok.type = inner.substr(0, comma);
                tok.value = inner.substr(comma + 1);
                // 去除空格
                size_t ts = tok.type.find_first_not_of(" \t");
                size_t te = tok.type.find_last_not_of(" \t");
                if (ts != string::npos) tok.type = tok.type.substr(ts, te - ts + 1);
                size_t vs = tok.value.find_first_not_of(" \t");
                size_t ve = tok.value.find_last_not_of(" \t");
                if (vs != string::npos) tok.value = tok.value.substr(vs, ve - vs + 1);
            } else {
                tok.type = tok.value = inner;
            }
        } else {
            // 原生格式: TYPE value
            stringstream ss(line);
            if (ss >> tok.type >> tok.value) {
                if (tok.value.empty()) tok.value = tok.type;
            } else if (ss >> tok.type) {
                tok.value = tok.type;
            }
        }

        // 实验二到实验五的 token 类型映射
        if (tok.type == "NUM") {
            // 根据数值判断是整数还是浮点数
            bool is_float = false;
            for (char c : tok.value) {
                if (c == '.' || c == 'e' || c == 'E') { is_float = true; break; }
            }
            tok.type = is_float ? "FLOAT_NUM" : "INT_NUM";
        } else if (tok.type == "FLOAT" && tok.value != "float") {
            // 实验二把浮点数字面量也标为 FLOAT（如 (FLOAT, 3.5)）
            tok.type = "FLOAT_NUM";
        } else if (tok.type == "INT" && tok.value != "int") {
            // 同理，若实验二把整数字面量标为 INT（如 (INT, 5)）
            tok.type = "INT_NUM";
        } else if (tok.type == "KEY_INT") {
            tok.type = "INT";
        } else if (tok.type == "KEY_FLOAT") {
            tok.type = "FLOAT";
        } else if (tok.type == "KEY_VOID") {
            tok.type = "VOID";
        } else if (tok.type == "KEY_PRINT") {
            tok.type = "PRINT";
        }
        // 其他类型如 ID, ADD, MUL, ASG, SEMI, LPAR, RPAR 等无需映射

        if (!tok.type.empty()) tokens.push_back(tok);
    }
    // 追加结束标记
    tokens.push_back({"$", "$"});
    return tokens;
}

// ---------- AST ----------
enum class ASTNodeType {
    PROGRAM, DECL_LIST, VAR_DECL, ASSIGN_STMT, PRINT_STMT,
    BINARY_OP, IDENTIFIER, LITERAL
};

struct ASTNode {
    ASTNodeType type;
    string label;
    string value;
    string dtype; // 数据类型（int/float）
    vector<ASTNode*> children;
    ASTNode(ASTNodeType t, const string& l = "", const string& v = "", const string& d = "")
        : type(t), label(l), value(v), dtype(d) {}
    ~ASTNode() { for (auto c : children) delete c; }
};

const char* astTypeName(ASTNodeType t) {
    switch (t) {
        case ASTNodeType::PROGRAM: return "Program";
        case ASTNodeType::DECL_LIST: return "DeclList";
        case ASTNodeType::VAR_DECL: return "VarDecl";
        case ASTNodeType::ASSIGN_STMT: return "Assign";
        case ASTNodeType::PRINT_STMT: return "Print";
        case ASTNodeType::BINARY_OP: return "BinOp";
        case ASTNodeType::IDENTIFIER: return "Id";
        case ASTNodeType::LITERAL: return "Literal";
    }
    return "Unknown";
}

void printAST(ASTNode* node, int indent = 0) {
    if (!node) return;
    for (int i = 0; i < indent; ++i) cout << "  ";
    cout << astTypeName(node->type);
    if (!node->label.empty()) cout << " [" << node->label << "]";
    if (!node->value.empty()) cout << " = " << node->value;
    if (!node->dtype.empty()) cout << " <" << node->dtype << ">";
    cout << endl;
    for (auto c : node->children) printAST(c, indent + 1);
}

// ---------- 符号表 ----------
struct SymbolEntry {
    string name;
    string type;
    int scope_level;
};

class SymbolTable {
    vector<map<string, SymbolEntry>> scopes;
public:
    SymbolTable() { scopes.emplace_back(); } // 全局作用域
    void enterScope() { scopes.emplace_back(); }
    void exitScope() { if (scopes.size() > 1) scopes.pop_back(); }
    bool insert(const string& name, const string& type) {
        if (scopes.empty()) return false;
        auto& cur = scopes.back();
        if (cur.count(name)) return false; // 重复声明
        SymbolEntry se{name, type, (int)scopes.size() - 1};
        cur[name] = se;
        return true;
    }
    SymbolEntry* lookup(const string& name) {
        for (int i = (int)scopes.size() - 1; i >= 0; --i) {
            auto it = scopes[i].find(name);
            if (it != scopes[i].end()) return &(it->second);
        }
        return nullptr;
    }
    SymbolEntry* lookupCurrent(const string& name) {
        if (scopes.empty()) return nullptr;
        auto it = scopes.back().find(name);
        if (it != scopes.back().end()) return &(it->second);
        return nullptr;
    }
    void print() const {
        cout << "\n========== 符号表 (Symbol Table) ==========\n";
        for (size_t i = 0; i < scopes.size(); ++i) {
            cout << "Scope " << i << ":\n";
            if (scopes[i].empty()) cout << "  (empty)\n";
            for (const auto& kv : scopes[i]) {
                cout << "  " << kv.second.name << " : " << kv.second.type << endl;
            }
        }
        cout << "==========================================\n";
    }
};

// ---------- 语义值与属性栈 ----------
enum class ValueType { INT, FLOAT, VOID, ERROR };

struct SemanticValue {
    string sym;           // 文法符号（终结符/非终结符类型）
    string token_val;     // 终结符的原始值（如变量名、数字）
    string addr;          // 地址/临时变量名
    ValueType vtype;      // 值类型
    ASTNode* ast;         // AST子树
    string code;          // 生成的三地址码片段
    SemanticValue() : vtype(ValueType::VOID), ast(nullptr) {}
    ~SemanticValue() { /* AST所有权由父节点或根节点管理 */ }
};

string valueTypeStr(ValueType vt) {
    switch (vt) {
        case ValueType::INT: return "int";
        case ValueType::FLOAT: return "float";
        case ValueType::VOID: return "void";
        case ValueType::ERROR: return "error";
    }
    return "unknown";
}

ValueType strToValueType(const string& s) {
    if (s == "int" || s == "INT") return ValueType::INT;
    if (s == "float" || s == "FLOAT") return ValueType::FLOAT;
    return ValueType::VOID;
}

string newTemp() {
    static int cnt = 0;
    return "t" + to_string(++cnt);
}

// 语义错误记录
struct SemanticError {
    int line; // 简化：用token索引代替
    string msg;
};
vector<SemanticError> sem_errors;

void semanticError(int pos, const string& msg) {
    sem_errors.push_back({pos, msg});
    cerr << "[Semantic Error] at token #" << pos << ": " << msg << endl;
}

// ============================================================
//  Part 3: 语义动作
// ============================================================
// 规约时调用：rhs_vals[0] 是产生式右部最左符号的语义值
SemanticValue executeAction(int prod_id, const vector<SemanticValue>& rhs, int token_pos, SymbolTable& symtab) {
    SemanticValue res;
    const Production& p = productions[prod_id];

    // 辅助lambda：创建二元运算AST
    auto makeBinOp = [&](const string& op) {
        ASTNode* node = new ASTNode(ASTNodeType::BINARY_OP, op);
        node->children.push_back(rhs[0].ast);
        node->children.push_back(rhs[2].ast);
        return node;
    };

    // 辅助：类型检查与推导
    auto checkArith = [&]() -> ValueType {
        ValueType t1 = rhs[0].vtype;
        ValueType t2 = rhs[2].vtype;
        if (t1 == ValueType::ERROR || t2 == ValueType::ERROR) return ValueType::ERROR;
        if (t1 != t2) {
            semanticError(token_pos, "Type mismatch in arithmetic expression: " + valueTypeStr(t1) + " vs " + valueTypeStr(t2));
            return ValueType::ERROR;
        }
        return t1;
    };

    switch (prod_id) {
        case 0: // S' -> Prog
            res.ast = rhs[0].ast;
            res.code = rhs[0].code;
            break;
        case 1: // Prog -> DeclList
            res.ast = new ASTNode(ASTNodeType::PROGRAM, "Prog");
            res.ast->children.push_back(rhs[0].ast);
            res.code = rhs[0].code;
            break;
        case 2: { // DeclList -> DeclList Decl
            res.ast = rhs[0].ast; // 复用已有DeclList节点
            res.ast->children.push_back(rhs[1].ast);
            res.code = rhs[0].code + rhs[1].code;
            break;
        }
        case 3: { // DeclList -> Decl
            res.ast = new ASTNode(ASTNodeType::DECL_LIST, "DeclList");
            res.ast->children.push_back(rhs[0].ast);
            res.code = rhs[0].code;
            break;
        }
        case 4: // Decl -> VarDecl
        case 5: // Decl -> Assign
        case 6: // Decl -> Print
            res = rhs[0];
            break;
        case 7: { // VarDecl -> Type ID SEMI
            string vtype = rhs[0].token_val; // "int" or "float"
            string vname = rhs[1].token_val;
            if (!symtab.insert(vname, vtype)) {
                semanticError(token_pos, "Variable '" + vname + "' redeclared");
            }
            res.ast = new ASTNode(ASTNodeType::VAR_DECL, vtype, vname, vtype);
            res.code = "";
            break;
        }
        case 8: // Type -> INT
            res.token_val = "int";
            res.vtype = ValueType::INT;
            break;
        case 9: // Type -> FLOAT
            res.token_val = "float";
            res.vtype = ValueType::FLOAT;
            break;
        case 10: { // Assign -> ID ASG Expr SEMI
            string vname = rhs[0].token_val;
            SymbolEntry* ent = symtab.lookup(vname);
            if (!ent) {
                semanticError(token_pos, "Variable '" + vname + "' not declared");
                res.vtype = ValueType::ERROR;
            } else {
                res.vtype = strToValueType(ent->type);
                if (res.vtype != rhs[2].vtype && rhs[2].vtype != ValueType::ERROR) {
                    semanticError(token_pos, "Type mismatch in assignment to '" + vname + "': expected " + valueTypeStr(res.vtype) + ", got " + valueTypeStr(rhs[2].vtype));
                }
            }
            res.ast = new ASTNode(ASTNodeType::ASSIGN_STMT, "=", vname);
            res.ast->children.push_back(rhs[2].ast);
            res.addr = rhs[2].addr.empty() ? rhs[2].token_val : rhs[2].addr;
            if (ent) {
                res.code = rhs[2].code + ent->name + " = " + res.addr + "\n";
            } else {
                res.code = rhs[2].code;
            }
            break;
        }
        case 11: { // Print -> PRINT Expr SEMI
            res.ast = new ASTNode(ASTNodeType::PRINT_STMT, "print");
            res.ast->children.push_back(rhs[1].ast);
            res.code = rhs[1].code + "print " + rhs[1].addr + "\n";
            break;
        }
        case 12: { // Expr -> Expr ADD Term
            res.vtype = checkArith();
            res.addr = newTemp();
            res.ast = makeBinOp("+");
            res.code = rhs[0].code + rhs[2].code + res.addr + " = " + rhs[0].addr + " + " + rhs[2].addr + "\n";
            break;
        }
        case 13: // Expr -> Term
            res = rhs[0];
            break;
        case 14: { // Term -> Term MUL Fact
            res.vtype = checkArith();
            res.addr = newTemp();
            res.ast = makeBinOp("*");
            res.code = rhs[0].code + rhs[2].code + res.addr + " = " + rhs[0].addr + " * " + rhs[2].addr + "\n";
            break;
        }
        case 15: // Term -> Fact
            res = rhs[0];
            break;
        case 16: { // Fact -> ID
            string vname = rhs[0].token_val;
            SymbolEntry* ent = symtab.lookup(vname);
            if (!ent) {
                semanticError(token_pos, "Variable '" + vname + "' not declared");
                res.vtype = ValueType::ERROR;
            } else {
                res.vtype = strToValueType(ent->type);
            }
            res.addr = vname;
            res.ast = new ASTNode(ASTNodeType::IDENTIFIER, vname, vname, ent ? ent->type : "error");
            break;
        }
        case 17: // Fact -> INT_NUM
            res.addr = rhs[0].token_val;
            res.vtype = ValueType::INT;
            res.ast = new ASTNode(ASTNodeType::LITERAL, rhs[0].token_val, rhs[0].token_val, "int");
            break;
        case 18: // Fact -> FLOAT_NUM
            res.addr = rhs[0].token_val;
            res.vtype = ValueType::FLOAT;
            res.ast = new ASTNode(ASTNodeType::LITERAL, rhs[0].token_val, rhs[0].token_val, "float");
            break;
        case 19: // Fact -> LPAR Expr RPAR
            res = rhs[1];
            break;
        default:
            if (!rhs.empty()) res = rhs[0];
            break;
    }
    return res;
}

// ============================================================
//  Part 4: SLR(1) 驱动 + 语义分析
// ============================================================

struct ParseStep {
    int step;
    string stack_state;
    string stack_sym;
    string input;
    string action;
};

bool parseAndAnalyze(const vector<Token>& tokens, ASTNode*& root, SymbolTable& symtab, vector<ParseStep>& steps, SemanticValue& final_val) {
    stack<int> state_st;
    stack<string> sym_st;
    stack<SemanticValue> attr_st;

    state_st.push(0);
    sym_st.push("$");

    SemanticValue init_sv;
    init_sv.sym = "$";
    attr_st.push(init_sv);

    int pos = 0;
    int step_cnt = 0;

    while (pos < (int)tokens.size()) {
        int state = state_st.top();
        const Token& tok = tokens[pos];
        string lookahead = tok.type;

        auto ait = action_table[state].find(lookahead);
        if (ait == action_table[state].end()) {
            cerr << "[Syntax Error] at token #" << pos << " (" << tok.type << "," << tok.value << "): unexpected token in state " << state << endl;
            return false;
        }

        string action = ait->second;

        // 记录步骤（可选）
        {
            ParseStep ps;
            ps.step = ++step_cnt;
            // 状态栈快照
            stack<int> tmp_st = state_st;
            vector<int> stv;
            while (!tmp_st.empty()) { stv.push_back(tmp_st.top()); tmp_st.pop(); }
            reverse(stv.begin(), stv.end());
            for (int s : stv) ps.stack_state += to_string(s) + " ";
            ps.input = lookahead + "(" + tok.value + ") ...";
            ps.action = action;
            steps.push_back(ps);
        }

        if (action[0] == 's') {
            int next_state = stoi(action.substr(1));
            // 移进
            state_st.push(next_state);
            sym_st.push(lookahead);

            SemanticValue sv;
            sv.sym = lookahead;
            sv.token_val = tok.value;
            sv.addr = tok.value;
            if (lookahead == "INT_NUM") sv.vtype = ValueType::INT;
            else if (lookahead == "FLOAT_NUM") sv.vtype = ValueType::FLOAT;
            else if (lookahead == "ID") sv.vtype = ValueType::VOID; // 待查找
            sv.ast = nullptr; // 终结符一般不在此建AST，由产生式处理（或在此建简单节点）
            attr_st.push(sv);
            ++pos;
        } else if (action[0] == 'r') {
            int prod_id = stoi(action.substr(1));
            const Production& p = productions[prod_id];
            int rhs_len = (int)p.rhs.size();

            vector<SemanticValue> rhs_vals(rhs_len);
            for (int i = rhs_len - 1; i >= 0; --i) {
                rhs_vals[i] = attr_st.top(); attr_st.pop();
                sym_st.pop();
                state_st.pop();
            }

            // 执行语义动作
            SemanticValue res = executeAction(prod_id, rhs_vals, pos, symtab);
            res.sym = p.lhs;

            int top_state = state_st.top();
            auto git = goto_table[top_state].find(p.lhs);
            if (git == goto_table[top_state].end()) {
                cerr << "[Syntax Error] GOTO missing for non-terminal " << p.lhs << " from state " << top_state << endl;
                return false;
            }
            int next_state = git->second;
            state_st.push(next_state);
            sym_st.push(p.lhs);
            attr_st.push(res);
        } else if (action == "acc") {
            // 接受
            final_val = attr_st.top();
            root = final_val.ast;
            return true;
        } else {
            cerr << "[Error] Unknown action: " << action << endl;
            return false;
        }
    }
    return false;
}

// ============================================================
//  Part 5: 输出与主函数
// ============================================================

void printSteps(const vector<ParseStep>& steps) {
    cout << "\n========== SLR(1) 分析过程（前30步） ==========\n";
    cout << left << setw(6) << "Step" << setw(40) << "State Stack" << setw(25) << "Lookahead" << "Action\n";
    cout << string(80, '-') << "\n";
    for (size_t i = 0; i < min(steps.size(), size_t(30)); ++i) {
        cout << left << setw(6) << steps[i].step
             << setw(40) << steps[i].stack_state
             << setw(25) << steps[i].input
             << steps[i].action << "\n";
    }
    if (steps.size() > 30) cout << "... (" << steps.size() << " steps total)\n";
    cout << "===============================================\n";
}

void printIntermediateCode(ASTNode* node) {
    if (!node) return;
    // 从根节点的code字段无法直接获取，因为最终Prog节点的code存在attr栈顶
    // 这里我们通过全局收集或重构。简单处理：我们在parseAndAnalyze后，
    // 从最终SemanticValue的code字段输出。所以在main里直接输出。
}

int main(int argc, char* argv[]) {
    string grammar_file = "grammar.txt";
    string token_file = "test1.txt";
    string table_file;

    // 参数解析
    if (argc > 1) grammar_file = argv[1];
    int tok_idx = 2;
    if (argc > 3 && string(argv[2]) == "--load-table") {
        table_file = argv[3];
        tok_idx = 4;
    }
    if (argc > tok_idx) token_file = argv[tok_idx];

    cout << "==================================================\n";
    cout << "实验五：SLR(1) 引导的语义分析框架\n";
    cout << "==================================================\n";
    cout << "Grammar: " << grammar_file << "\n";
    cout << "Tokens : " << token_file << "\n";
    if (!table_file.empty()) cout << "SLR Table (from exp4): " << table_file << "\n";
    cout << "\n";

    if (!parseGrammar(grammar_file)) return 1;

    if (!table_file.empty()) {
        // 从实验四导出文件加载 SLR 表
        if (!loadSLRTable(table_file)) {
            cerr << "Failed to load SLR table, falling back to generation.\n";
            canonical_collection = buildCanonicalCollection(transitions);
            computeFirst();
            computeFollow();
            buildSLRTable();
            cout << "SLR(1) 分析表已现场生成。状态数: " << action_table.size() << "\n\n";
        } else {
            cout << "SLR(1) 分析表已从实验四输出加载。状态数: " << action_table.size() << "\n\n";
        }
    } else {
        // 现场生成
        canonical_collection = buildCanonicalCollection(transitions);
        computeFirst();
        computeFollow();
        buildSLRTable();
        cout << "SLR(1) 分析表已现场生成。状态数: " << canonical_collection.size()
             << ", 产生式数: " << productions.size() << "\n\n";
    }

    vector<Token> tokens = loadTokens(token_file);
    if (tokens.empty()) { cerr << "No tokens loaded.\n"; return 1; }

    ASTNode* root = nullptr;
    SymbolTable symtab;
    vector<ParseStep> steps;
    sem_errors.clear();

    SemanticValue final_val;
    bool ok = parseAndAnalyze(tokens, root, symtab, steps, final_val);

    printSteps(steps);

    cout << "\n========== 抽象语法树 (AST) ==========\n";
    if (root) printAST(root);
    else cout << "(AST is null)\n";

    symtab.print();

    cout << "\n========== 语义错误报告 ==========\n";
    if (sem_errors.empty()) {
        cout << "✅ 未检测到语义错误。\n";
    } else {
        for (const auto& err : sem_errors) {
            cout << "❌ Token #" << err.line << ": " << err.msg << "\n";
        }
    }

    if (ok) {
        cout << "\n========== 分析结果 ==========\n";
        cout << "✅ 语法分析与语义分析完成。\n";
        if (!final_val.code.empty()) {
            cout << "\n========== 中间代码（三地址码） ==========\n";
            cout << final_val.code;
            cout << "==========================================\n";
        }
    } else {
        cout << "\n========== 分析结果 ==========\n";
        cout << "❌ 分析失败。\n";
    }

    // 清理
    if (root) delete root;
    return ok ? 0 : 1;
}
