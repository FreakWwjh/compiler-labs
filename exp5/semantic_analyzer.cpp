#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <stack>
#include <algorithm>
#include <iomanip>
using namespace std;

// ============================================================
//  Part 1: 实验四核心复用 — 文法解析 + LR(0) + FIRST/FOLLOW + SLR表
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
set<string> non_terminals, terminals, all_symbols;
string start_symbol;
int augmented_start_prod_id = -1;

map<string, set<string>> first_sets, follow_sets;
vector<ItemSet> canonical_collection;
vector<Transition> transitions;

vector<map<string, string>> action_table;
vector<map<string, int>> goto_table;

bool isTerminal(const string& sym) {
    return non_terminals.find(sym) == non_terminals.end();
}

// 解析文法文件，格式:  LHS -> RHS1 | RHS2
bool parseGrammar(const string& filename) {
    ifstream in(filename);
    if (!in.is_open()) {
        cerr << "Error: Cannot open grammar file " << filename << endl;
        return false;
    }
    string line;
    int prod_counter = 0;
    bool first_prod = true;
    while (getline(in, line)) {
        size_t s = line.find_first_not_of(" \t\r\n");
        if (s == string::npos) continue;
        size_t e = line.find_last_not_of(" \t\r\n");
        line = line.substr(s, e - s + 1);
        if (line.empty() || line[0] == '#') continue;

        size_t ap1 = line.find("->"), ap2 = line.find("→"), ap;
        int al;
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
            size_t ps = part.find_first_not_of(" \t"), pe = part.find_last_not_of(" \t");
            if (ps != string::npos) alts.push_back(part.substr(ps, pe - ps + 1));
            if (bar == string::npos) break;
            pos = bar + 1;
        }

        for (const string& alt : alts) {
            Production p;
            p.id = prod_counter++;
            p.lhs = lhs;
            stringstream ss(alt);
            string sym;
            while (ss >> sym) p.rhs.push_back(sym);
            if (p.rhs.size() == 1 && p.rhs[0] == "ε") p.rhs.clear();
            if (first_prod) { start_symbol = lhs; first_prod = false; }
            productions.push_back(p);
        }
    }
    in.close();

    for (const auto& p : productions)
        for (const auto& s : p.rhs)
            if (non_terminals.find(s) == non_terminals.end())
                terminals.insert(s);

    if (productions.empty()) return false;

    // 增广文法
    string ns = start_symbol + "'";
    while (non_terminals.count(ns)) ns += "'";
    Production aug;
    aug.id = 0;
    aug.lhs = ns;
    aug.rhs.push_back(start_symbol);
    for (auto& p : productions) p.id += 1;
    productions.insert(productions.begin(), aug);
    augmented_start_prod_id = 0;
    non_terminals.insert(ns);
    for (size_t i = 0; i < productions.size(); ++i) productions[i].id = (int)i;

    all_symbols = terminals;
    all_symbols.insert(non_terminals.begin(), non_terminals.end());
    return true;
}

// LR(0) 闭包
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
                    if (res.find(ni) == res.end()) {
                        res.insert(ni);
                        changed = true;
                    }
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
    for (size_t i = 0; i < col.size(); ++i)
        if (col[i] == items) return (int)i;
    return -1;
}

set<string> getSymbols(const ItemSet& items) {
    set<string> syms;
    for (const auto& item : items) {
        const Production& p = productions[item.prod_id];
        if (item.dot_pos < (int)p.rhs.size())
            syms.insert(p.rhs[item.dot_pos]);
    }
    return syms;
}

vector<ItemSet> buildCanonicalCollection(vector<Transition>& trans) {
    vector<ItemSet> col;
    trans.clear();
    ItemSet I0;
    I0.insert({augmented_start_prod_id, 0});
    I0 = closure(I0);
    col.push_back(I0);
    queue<int> q;
    q.push(0);
    while (!q.empty()) {
        int idx = q.front(); q.pop();
        set<string> syms = getSymbols(col[idx]);
        for (const string& X : syms) {
            ItemSet J = goTo(col[idx], X);
            if (J.empty()) continue;
            int jdx = findItemSet(col, J);
            if (jdx == -1) {
                jdx = (int)col.size();
                col.push_back(J);
                q.push(jdx);
            }
            trans.push_back({idx, X, jdx});
        }
    }
    return col;
}

// FIRST / FOLLOW
set<string> firstOfSeq(const vector<string>& seq) {
    set<string> res;
    for (size_t i = 0; i < seq.size(); ++i) {
        if (isTerminal(seq[i])) { res.insert(seq[i]); return res; }
        const auto& f = first_sets[seq[i]];
        for (const string& s : f) if (s != "ε") res.insert(s);
        if (f.find("ε") == f.end()) return res;
    }
    res.insert("ε");
    return res;
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
            if (p.rhs.empty()) {
                if (first_sets[A].insert("ε").second) changed = true;
                continue;
            }
            for (size_t i = 0; i < p.rhs.size(); ++i) {
                const string& Xi = p.rhs[i];
                if (isTerminal(Xi)) {
                    if (first_sets[A].insert(Xi).second) changed = true;
                    break;
                }
                const auto& xf = first_sets[Xi];
                for (const string& s : xf) if (s != "ε")
                    if (first_sets[A].insert(s).second) changed = true;
                if (xf.find("ε") == xf.end()) break;
                if (i == p.rhs.size() - 1)
                    if (first_sets[A].insert("ε").second) changed = true;
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
                    for (const string& s : fb) if (s != "ε")
                        if (follow_sets[B].insert(s).second) changed = true;
                    if (fb.count("ε"))
                        for (const string& s : follow_sets[A])
                            if (follow_sets[B].insert(s).second) changed = true;
                } else {
                    for (const string& s : follow_sets[A])
                        if (follow_sets[B].insert(s).second) changed = true;
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
                if (item.prod_id == augmented_start_prod_id)
                    action_table[i]["$"] = "acc";
                else {
                    string act = "r" + to_string(item.prod_id);
                    for (const string& a : follow_sets[p.lhs])
                        action_table[i][a] = act;
                }
            } else {
                string ns = p.rhs[item.dot_pos];
                if (isTerminal(ns)) {
                    auto key = make_pair(i, ns);
                    if (tmap.count(key))
                        action_table[i][ns] = "s" + to_string(tmap[key]);
                }
            }
        }
    }
    for (const auto& t : transitions)
        if (!isTerminal(t.symbol))
            goto_table[t.from][t.symbol] = t.to;
}

// 加载实验四导出的 SLR 表文件
bool loadSLRTable(const string& filename) {
    ifstream in(filename);
    if (!in.is_open()) return false;
    action_table.clear();
    goto_table.clear();
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
    if ((int)action_table.size() <= max_state) action_table.resize(max_state + 1);
    if ((int)goto_table.size() <= max_state) goto_table.resize(max_state + 1);
    return true;
}

// ============================================================
//  Part 2: Token / AST / 符号表
// ============================================================

// Token 结构：对应实验二输出的 (TYPE, value) 格式
struct Token {
    string type;   // 种属：ID, INT_NUM, ADD, SEMI 等
    string value;  // 个体值：变量名、数字、运算符字符等
};

// 读取 Token 文件，支持两种格式：
//   实验二标准输出: (TYPE, value)
//   原生简化格式:   TYPE value
vector<Token> loadTokens(const string& filename) {
    vector<Token> tokens;
    ifstream in(filename);
    if (!in.is_open()) {
        cerr << "Error: Cannot open token file " << filename << endl;
        return tokens;
    }
    string line;
    while (getline(in, line)) {
        size_t s = line.find_first_not_of(" \t\r\n");
        if (s == string::npos) continue;
        size_t e = line.find_last_not_of(" \t\r\n");
        line = line.substr(s, e - s + 1);
        if (line.empty() || line[0] == '#') continue;

        Token tok;
        // 尝试解析实验二输出格式: (TYPE, value)
        if (!line.empty() && line.front() == '(' && line.back() == ')') {
            string inner = line.substr(1, line.size() - 2);
            size_t comma = inner.find(',');
            if (comma != string::npos) {
                tok.type = inner.substr(0, comma);
                tok.value = inner.substr(comma + 1);
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
            // 原生简化格式
            stringstream ss(line);
            if (ss >> tok.type >> tok.value) {
                if (tok.value.empty()) tok.value = tok.type;
            } else if (ss >> tok.type) {
                tok.value = tok.type;
            }
        }

        // 实验二到实验五的 token 类型映射
        if (tok.type == "NUM") {
            bool is_float = false;
            for (char c : tok.value)
                if (c == '.' || c == 'e' || c == 'E') { is_float = true; break; }
            tok.type = is_float ? "FLOAT_NUM" : "INT_NUM";
        } else if (tok.type == "FLOAT" && tok.value != "float") {
            tok.type = "FLOAT_NUM";
        } else if (tok.type == "INT" && tok.value != "int") {
            tok.type = "INT_NUM";
        } else if (tok.type == "KEY_INT") tok.type = "INT";
        else if (tok.type == "KEY_FLOAT") tok.type = "FLOAT";
        else if (tok.type == "KEY_PRINT") tok.type = "PRINT";

        // 过滤 scanner 产生的干扰 token（注释中的非法字符）
        if (tok.type == "UNKNOWN" || tok.type == "COMMENT") continue;

        if (!tok.type.empty()) tokens.push_back(tok);
    }
    // 自动追加结束标记
    tokens.push_back({"$", "$"});
    return tokens;
}

// AST 节点类型
enum class ASTNodeType {
    PROGRAM, DECL_LIST, VAR_DECL, ASSIGN_STMT, PRINT_STMT,
    BINARY_OP, IDENTIFIER, LITERAL
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
        default: return "Unknown";
    }
}

// AST 节点
struct ASTNode {
    ASTNodeType type;
    string label;    // 节点标签，如运算符 "+"
    string value;    // 节点值，如变量名 "x"
    string dtype;    // 数据类型，如 "int"
    vector<ASTNode*> children;
    ASTNode(ASTNodeType t, const string& l = "", const string& v = "", const string& d = "")
        : type(t), label(l), value(v), dtype(d) {}
    ~ASTNode() { for (auto c : children) delete c; }
};

// 打印 AST（缩进树形）
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

// 符号表条目
struct SymbolEntry {
    string name;
    string type;   // "int" 或 "float"
};

// 符号表：单作用域（实验五简化版）
class SymbolTable {
    map<string, SymbolEntry> table;
public:
    // 插入变量，返回是否成功（失败 = 重复声明）
    bool insert(const string& name, const string& type) {
        if (table.count(name)) return false;
        table[name] = {name, type};
        return true;
    }
    // 查找变量
    SymbolEntry* lookup(const string& name) {
        auto it = table.find(name);
        if (it != table.end()) return &(it->second);
        return nullptr;
    }
    // 打印符号表
    void print() const {
        cout << "\n========== 符号表 (Symbol Table) ==========\n";
        if (table.empty()) cout << "  (empty)\n";
        for (const auto& kv : table)
            cout << "  " << kv.second.name << " : " << kv.second.type << endl;
        cout << "==========================================\n";
    }
};

// ============================================================
//  Part 3: 语义值 + 语义动作
// ============================================================

// 值类型枚举
enum class ValueType { INT, FLOAT, VOID, ERROR };

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

// 语义值（属性栈中保存的数据）
struct SemanticValue {
    string sym;        // 文法符号名称
    string token_val;  // token 的原始值（如 "x", "5"）
    string addr;       // 地址 / 临时变量名（如 "t1"）
    ValueType vtype;   // 值类型
    ASTNode* ast;      // AST 子树指针
    string code;       // 生成的三地址码片段
    SemanticValue() : vtype(ValueType::VOID), ast(nullptr) {}
};

// 临时变量生成器
string newTemp() {
    static int cnt = 0;
    return "t" + to_string(++cnt);
}

// 语义错误记录
struct SemanticError { int line; string msg; };
vector<SemanticError> sem_errors;

void semanticError(int pos, const string& msg) {
    sem_errors.push_back({pos, msg});
    cerr << "[Semantic Error] at token #" << pos << ": " << msg << endl;
}

// 语义动作执行器
//   参数 prod_id: 产生式编号（对应全局 productions 数组）
//   参数 rhs:     产生式右部各符号的语义值（rhs[0] 为最左符号）
//   参数 token_pos: 当前 token 位置（用于报错定位）
//   参数 symtab:   符号表引用
SemanticValue executeAction(int prod_id, const vector<SemanticValue>& rhs, int token_pos, SymbolTable& symtab) {
    SemanticValue res;

    // 辅助函数：创建二元运算 AST 节点
    auto makeBinOp = [&](const string& op) {
        ASTNode* node = new ASTNode(ASTNodeType::BINARY_OP, op);
        node->children.push_back(rhs[0].ast);
        node->children.push_back(rhs[2].ast);
        return node;
    };

    // 辅助函数：算术运算类型检查
    auto checkArith = [&]() -> ValueType {
        ValueType t1 = rhs[0].vtype;
        ValueType t2 = rhs[2].vtype;
        if (t1 == ValueType::ERROR || t2 == ValueType::ERROR) return ValueType::ERROR;
        if (t1 != t2) {
            semanticError(token_pos, "Type mismatch in expression: " + valueTypeStr(t1) + " vs " + valueTypeStr(t2));
            return ValueType::ERROR;
        }
        return t1;
    };

    switch (prod_id) {
        // 0: S' -> Prog
        case 0:
            res.ast = rhs[0].ast;
            res.code = rhs[0].code;
            break;

        // 1: Prog -> DeclList
        case 1:
            res.ast = new ASTNode(ASTNodeType::PROGRAM, "Prog");
            res.ast->children.push_back(rhs[0].ast);
            res.code = rhs[0].code;
            break;

        // 2: DeclList -> DeclList Decl
        case 2:
            res.ast = rhs[0].ast;
            res.ast->children.push_back(rhs[1].ast);
            res.code = rhs[0].code + rhs[1].code;
            break;

        // 3: DeclList -> Decl
        case 3:
            res.ast = new ASTNode(ASTNodeType::DECL_LIST, "DeclList");
            res.ast->children.push_back(rhs[0].ast);
            res.code = rhs[0].code;
            break;

        // 4: Decl -> VarDecl
        case 4:
        // 5: Decl -> Assign
        case 5:
        // 6: Decl -> Print
        case 6:
            res = rhs[0];
            break;

        // 7: VarDecl -> Type ID SEMI
        case 7: {
            string vtype = rhs[0].token_val;   // "int" or "float"
            string vname = rhs[1].token_val;   // 变量名
            if (!symtab.insert(vname, vtype))
                semanticError(token_pos, "Variable '" + vname + "' redeclared");
            res.ast = new ASTNode(ASTNodeType::VAR_DECL, vtype, vname, vtype);
            res.code = "";
            break;
        }

        // 8: Type -> INT
        case 8:
            res.token_val = "int";
            res.vtype = ValueType::INT;
            break;

        // 9: Type -> FLOAT
        case 9:
            res.token_val = "float";
            res.vtype = ValueType::FLOAT;
            break;

        // 10: Assign -> ID ASG Expr SEMI
        case 10: {
            string vname = rhs[0].token_val;
            SymbolEntry* ent = symtab.lookup(vname);
            if (!ent) {
                semanticError(token_pos, "Variable '" + vname + "' not declared");
                res.vtype = ValueType::ERROR;
            } else {
                res.vtype = strToValueType(ent->type);
                if (res.vtype != rhs[2].vtype && rhs[2].vtype != ValueType::ERROR)
                    semanticError(token_pos, "Type mismatch in assignment to '" + vname + "': expected " + valueTypeStr(res.vtype) + ", got " + valueTypeStr(rhs[2].vtype));
            }
            res.ast = new ASTNode(ASTNodeType::ASSIGN_STMT, "=", vname);
            res.ast->children.push_back(rhs[2].ast);
            res.addr = rhs[2].addr.empty() ? rhs[2].token_val : rhs[2].addr;
            res.code = rhs[2].code;
            if (ent) res.code += ent->name + " = " + res.addr + "\n";
            break;
        }

        // 11: Print -> PRINT Expr SEMI
        case 11:
            res.ast = new ASTNode(ASTNodeType::PRINT_STMT, "print");
            res.ast->children.push_back(rhs[1].ast);
            res.code = rhs[1].code + "print " + rhs[1].addr + "\n";
            break;

        // 12: Expr -> Expr ADD Term
        case 12:
            res.vtype = checkArith();
            res.addr = newTemp();
            res.ast = makeBinOp("+");
            res.code = rhs[0].code + rhs[2].code + res.addr + " = " + rhs[0].addr + " + " + rhs[2].addr + "\n";
            break;

        // 13: Expr -> Term
        case 13:
            res = rhs[0];
            break;

        // 14: Term -> Term MUL Fact
        case 14:
            res.vtype = checkArith();
            res.addr = newTemp();
            res.ast = makeBinOp("*");
            res.code = rhs[0].code + rhs[2].code + res.addr + " = " + rhs[0].addr + " * " + rhs[2].addr + "\n";
            break;

        // 15: Term -> Fact
        case 15:
            res = rhs[0];
            break;

        // 16: Fact -> ID
        case 16: {
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

        // 17: Fact -> INT_NUM
        case 17:
            res.addr = rhs[0].token_val;
            res.vtype = ValueType::INT;
            res.ast = new ASTNode(ASTNodeType::LITERAL, rhs[0].token_val, rhs[0].token_val, "int");
            break;

        // 18: Fact -> FLOAT_NUM
        case 18:
            res.addr = rhs[0].token_val;
            res.vtype = ValueType::FLOAT;
            res.ast = new ASTNode(ASTNodeType::LITERAL, rhs[0].token_val, rhs[0].token_val, "float");
            break;

        // 19: Fact -> LPAR Expr RPAR
        case 19:
            res = rhs[1];
            break;

        default:
            if (!rhs.empty()) res = rhs[0];
            break;
    }
    return res;
}

// ============================================================
//  Part 4: SLR(1) 驱动程序 + 语义分析
// ============================================================

struct ParseStep {
    int step;
    string stack_state;
    string input;
    string action;
};

// 核心驱动函数：读入 Token 流，执行 SLR 分析 + 语义动作
bool parseAndAnalyze(const vector<Token>& tokens, ASTNode*& root,
                     SymbolTable& symtab, vector<ParseStep>& steps,
                     SemanticValue& final_val) {
    stack<int> state_st;    // 状态栈
    stack<string> sym_st;    // 符号栈
    stack<SemanticValue> attr_st;  // 属性栈（实验五新增）

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
            cerr << "[Syntax Error] at token #" << pos
                 << " (" << tok.type << "," << tok.value << "): unexpected token in state " << state << endl;
            return false;
        }

        string action = ait->second;

        // 记录分析步骤（仅前30步在报告中显示）
        {
            ParseStep ps;
            ps.step = ++step_cnt;
            stack<int> tmp = state_st;
            vector<int> stv;
            while (!tmp.empty()) { stv.push_back(tmp.top()); tmp.pop(); }
            reverse(stv.begin(), stv.end());
            for (int s : stv) ps.stack_state += to_string(s) + " ";
            ps.input = lookahead + "(" + tok.value + ")";
            ps.action = action;
            steps.push_back(ps);
        }

        if (action[0] == 's') {
            // ========== 移进（Shift）==========
            int next_state = stoi(action.substr(1));
            state_st.push(next_state);
            sym_st.push(lookahead);

            SemanticValue sv;
            sv.sym = lookahead;
            sv.token_val = tok.value;
            sv.addr = tok.value;
            if (lookahead == "INT_NUM") sv.vtype = ValueType::INT;
            else if (lookahead == "FLOAT_NUM") sv.vtype = ValueType::FLOAT;
            sv.ast = nullptr;
            attr_st.push(sv);
            ++pos;

        } else if (action[0] == 'r') {
            // ========== 规约（Reduce）==========
            int prod_id = stoi(action.substr(1));
            const Production& p = productions[prod_id];
            int rhs_len = (int)p.rhs.size();

            // 从属性栈弹出右部符号的语义值（注意顺序）
            vector<SemanticValue> rhs_vals(rhs_len);
            for (int i = rhs_len - 1; i >= 0; --i) {
                rhs_vals[i] = attr_st.top(); attr_st.pop();
                sym_st.pop();
                state_st.pop();
            }

            // 执行语义动作
            SemanticValue res = executeAction(prod_id, rhs_vals, pos, symtab);
            res.sym = p.lhs;

            // 查 GOTO 表
            int top_state = state_st.top();
            auto git = goto_table[top_state].find(p.lhs);
            if (git == goto_table[top_state].end()) {
                cerr << "[Syntax Error] GOTO missing for " << p.lhs
                     << " from state " << top_state << endl;
                return false;
            }
            state_st.push(git->second);
            sym_st.push(p.lhs);
            attr_st.push(res);

        } else if (action == "acc") {
            // ========== 接受（Accept）==========
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
//  Part 5: 输出格式化 + 主函数
// ============================================================

void printSteps(const vector<ParseStep>& steps) {
    cout << "\n========== SLR(1) 分析过程（前30步） ==========\n";
    cout << left << setw(6) << "Step" << setw(40) << "State Stack"
         << setw(25) << "Lookahead" << "Action\n";
    cout << string(80, '-') << "\n";
    for (size_t i = 0; i < min(steps.size(), size_t(30)); ++i) {
        cout << left << setw(6) << steps[i].step
             << setw(40) << steps[i].stack_state
             << setw(25) << steps[i].input
             << steps[i].action << "\n";
    }
    if (steps.size() > 30)
        cout << "... (" << steps.size() << " steps total)\n";
    cout << "===============================================\n";
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
    if (!table_file.empty())
        cout << "SLR Table (from exp4): " << table_file << "\n";
    cout << "\n";

    // 1. 解析文法
    if (!parseGrammar(grammar_file)) return 1;

    // 2. 生成或加载 SLR 表
    if (!table_file.empty()) {
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
        canonical_collection = buildCanonicalCollection(transitions);
        computeFirst();
        computeFollow();
        buildSLRTable();
        cout << "SLR(1) 分析表已现场生成。状态数: " << canonical_collection.size()
             << ", 产生式数: " << productions.size() << "\n\n";
    }

    // 3. 加载 Token 流
    vector<Token> tokens = loadTokens(token_file);
    if (tokens.empty()) {
        cerr << "No tokens loaded.\n";
        return 1;
    }

    // 4. 执行语法分析 + 语义分析
    ASTNode* root = nullptr;
    SymbolTable symtab;
    vector<ParseStep> steps;
    sem_errors.clear();
    SemanticValue final_val;

    bool ok = parseAndAnalyze(tokens, root, symtab, steps, final_val);

    // 5. 输出结果
    printSteps(steps);

    cout << "\n========== 抽象语法树 (AST) ==========\n";
    if (root) printAST(root);
    else cout << "(AST is null)\n";

    symtab.print();

    cout << "\n========== 语义错误报告 ==========\n";
    if (sem_errors.empty()) {
        cout << "✅ 未检测到语义错误。\n";
    } else {
        for (const auto& err : sem_errors)
            cout << "❌ Token #" << err.line << ": " << err.msg << "\n";
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

    if (root) delete root;
    return ok ? 0 : 1;
}
