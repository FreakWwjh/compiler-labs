#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <algorithm>
#include <string>
#include <iomanip>

using namespace std;

struct Production {
    int id;
    string lhs;
    vector<string> rhs;
};

struct LR0Item {
    int prod_id;
    int dot_pos;
    
    bool operator<(const LR0Item& other) const {
        if (prod_id != other.prod_id) return prod_id < other.prod_id;
        return dot_pos < other.dot_pos;
    }
    
    bool operator==(const LR0Item& other) const {
        return prod_id == other.prod_id && dot_pos == other.dot_pos;
    }
};

using ItemSet = set<LR0Item>;

struct Transition {
    int from;
    string symbol;
    int to;
};

// ========== 全局数据结构 ==========
vector<Production> productions;
set<string> non_terminals;
set<string> terminals;
set<string> all_symbols;        // 终结符 + 非终结符
string start_symbol;
int augmented_start_prod_id = -1;

map<string, set<string>> first_sets;
map<string, set<string>> follow_sets;

vector<ItemSet> canonical_collection;
vector<Transition> transitions;

// SLR(1) 分析表
vector<map<string, string>> action_table;  // ACTION[state][terminal] -> "s3", "r2", "acc"
vector<map<string, int>> goto_table;       // GOTO[state][non_terminal] -> state

// 冲突记录
struct SLRConflict {
    int state;
    string symbol;
    string existing;
    string incoming;
    string type;
};
vector<SLRConflict> slr_conflicts;

// ========== 基础工具函数 ==========
bool isTerminal(const string& sym) {
    return non_terminals.find(sym) == non_terminals.end();
}

bool isEpsilonProduction(const Production& p) {
    return p.rhs.size() == 1 && p.rhs[0] == "ε";
}

// ========== 文法解析 ==========
bool parseGrammar(const string& filename) {
    ifstream in(filename);
    if (!in.is_open()) {
        cerr << "Error: Cannot open file " << filename << endl;
        return false;
    }
    
    string line;
    int prod_counter = 0;
    bool first_prod = true;
    
    while (getline(in, line)) {
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == string::npos) continue;
        size_t end = line.find_last_not_of(" \t\r\n");
        line = line.substr(start, end - start + 1);
        
        if (line.empty() || line[0] == '#') continue;
        
        size_t arrow_pos1 = line.find("->");
        size_t arrow_pos2 = line.find("→");
        size_t arrow_pos;
        int arrow_len;

        if (arrow_pos1 != string::npos) {
            arrow_pos = arrow_pos1;
            arrow_len = 2;
        } else if (arrow_pos2 != string::npos) {
            arrow_pos = arrow_pos2;
            arrow_len = 3;
        } else {
            cerr << "Error: Invalid production line: " << line << endl;
            continue;
        }

        string lhs = line.substr(0, arrow_pos);
        size_t lhs_end = lhs.find_last_not_of(" \t");
        if (lhs_end != string::npos) lhs = lhs.substr(0, lhs_end + 1);

        non_terminals.insert(lhs);

        string rhs_str = line.substr(arrow_pos + arrow_len);
        size_t rhs_start = rhs_str.find_first_not_of(" \t");
        if (rhs_start != string::npos) rhs_str = rhs_str.substr(rhs_start);
        
        vector<string> alt_rhs;
        size_t pos = 0;
        while (true) {
            size_t bar = rhs_str.find('|', pos);
            string part;
            if (bar == string::npos) {
                part = rhs_str.substr(pos);
            } else {
                part = rhs_str.substr(pos, bar - pos);
            }
            size_t pstart = part.find_first_not_of(" \t");
            size_t pend = part.find_last_not_of(" \t");
            if (pstart != string::npos) {
                part = part.substr(pstart, pend - pstart + 1);
                alt_rhs.push_back(part);
            }
            if (bar == string::npos) break;
            pos = bar + 1;
        }
        
        for (const string& rhs_part : alt_rhs) {
            Production p;
            p.id = prod_counter++;
            p.lhs = lhs;
            
            stringstream ss(rhs_part);
            string sym;
            while (ss >> sym) {
                p.rhs.push_back(sym);
            }
            
            if (first_prod) {
                start_symbol = lhs;
                first_prod = false;
            }
            productions.push_back(p);
        }
    }
    
    in.close();
    
    // 收集终结符
    for (const auto& p : productions) {
        for (const auto& sym : p.rhs) {
            if (non_terminals.find(sym) == non_terminals.end()) {
                terminals.insert(sym);
            }
        }
    }
    
    if (productions.empty()) {
        cerr << "Error: No valid productions found." << endl;
        return false;
    }
    
    // 增广文法
    string new_start = start_symbol + "'";
    while (non_terminals.find(new_start) != non_terminals.end()) {
        new_start += "'";
    }
    
    Production aug;
    aug.id = 0;
    aug.lhs = new_start;
    aug.rhs.push_back(start_symbol);
    
    for (auto& p : productions) {
        p.id += 1;
    }
    productions.insert(productions.begin(), aug);
    augmented_start_prod_id = 0;
    non_terminals.insert(new_start);
    
    for (size_t i = 0; i < productions.size(); ++i) {
        productions[i].id = (int)i;
    }
    
    // 构建 all_symbols
    all_symbols = terminals;
    all_symbols.insert(non_terminals.begin(), non_terminals.end());
    
    return true;
}

// ========== LR(0) 核心算法 ==========
ItemSet closure(const ItemSet& items) {
    ItemSet result = items;
    bool changed = true;
    
    while (changed) {
        changed = false;
        ItemSet current = result;
        for (const auto& item : current) {
            const Production& p = productions[item.prod_id];
            if (item.dot_pos >= (int)p.rhs.size()) continue;
            
            string B = p.rhs[item.dot_pos];
            if (isTerminal(B)) continue;
            
            for (const auto& prod : productions) {
                if (prod.lhs == B) {
                    LR0Item new_item{prod.id, 0};
                    if (result.find(new_item) == result.end()) {
                        result.insert(new_item);
                        changed = true;
                    }
                }
            }
        }
    }
    
    return result;
}

ItemSet goTo(const ItemSet& items, const string& X) {
    ItemSet J;
    for (const auto& item : items) {
        const Production& p = productions[item.prod_id];
        if (item.dot_pos < (int)p.rhs.size() && p.rhs[item.dot_pos] == X) {
            J.insert({item.prod_id, item.dot_pos + 1});
        }
    }
    return closure(J);
}

int findItemSet(const vector<ItemSet>& collection, const ItemSet& items) {
    for (size_t i = 0; i < collection.size(); ++i) {
        if (collection[i] == items) return (int)i;
    }
    return -1;
}

set<string> getSymbols(const ItemSet& items) {
    set<string> symbols;
    for (const auto& item : items) {
        const Production& p = productions[item.prod_id];
        if (item.dot_pos < (int)p.rhs.size()) {
            symbols.insert(p.rhs[item.dot_pos]);
        }
    }
    return symbols;
}

vector<ItemSet> buildCanonicalCollection(vector<Transition>& trans) {
    vector<ItemSet> collection;
    trans.clear();
    
    ItemSet I0;
    I0.insert({augmented_start_prod_id, 0});
    I0 = closure(I0);
    collection.push_back(I0);
    
    queue<int> q;
    q.push(0);
    
    while (!q.empty()) {
        int idx = q.front(); q.pop();
        ItemSet I = collection[idx];

        set<string> symbols = getSymbols(I);
        for (const string& X : symbols) {
            ItemSet J = goTo(I, X);
            if (J.empty()) continue;

            int jdx = findItemSet(collection, J);
            if (jdx == -1) {
                jdx = (int)collection.size();
                collection.push_back(J);
                q.push(jdx);
            }
            trans.push_back({idx, X, jdx});
        }
    }
    
    return collection;
}

// ========== FIRST 集与 FOLLOW 集计算 ==========

// 计算一个符号串的 FIRST 集（不含 ε 则返回，含 ε 则继续）
set<string> getFirstOfSequence(const vector<string>& seq) {
    set<string> result;
    for (size_t i = 0; i < seq.size(); ++i) {
        const string& sym = seq[i];
        if (isTerminal(sym)) {
            result.insert(sym);
            return result;
        }
        // 非终结符
        const auto& f = first_sets[sym];
        for (const string& s : f) {
            if (s != "ε") result.insert(s);
        }
        if (f.find("ε") == f.end()) {
            return result;  // 不含 ε，停止
        }
    }
    // 所有符号都能推 ε
    result.insert("ε");
    return result;
}

void computeFirst() {
    first_sets.clear();
    
    // 初始化：终结符的 FIRST 是它自己
    for (const string& t : terminals) {
        first_sets[t].insert(t);
    }
    // ε 作为特殊符号处理
    first_sets["ε"].insert("ε");
    
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& p : productions) {
            const string& A = p.lhs;
            if (p.rhs.empty()) {
                // A -> ε (空产生式)
                if (first_sets[A].insert("ε").second) changed = true;
                continue;
            }
            
            // 处理 A -> X1 X2 ... Xn
            for (size_t i = 0; i < p.rhs.size(); ++i) {
                const string& Xi = p.rhs[i];
                if (isTerminal(Xi)) {
                    if (first_sets[A].insert(Xi).second) changed = true;
                    break;
                } else {
                    // Xi 是非终结符
                    const auto& xi_first = first_sets[Xi];
                    for (const string& s : xi_first) {
                        if (s != "ε") {
                            if (first_sets[A].insert(s).second) changed = true;
                        }
                    }
                    if (xi_first.find("ε") == xi_first.end()) {
                        break;  // Xi 不能推 ε，停止
                    }
                    // Xi 能推 ε，继续看下一个
                    if (i == p.rhs.size() - 1) {
                        // 全部都能推 ε
                        if (first_sets[A].insert("ε").second) changed = true;
                    }
                }
            }
        }
    }
}

void computeFollow() {
    follow_sets.clear();
    
    // 增广开始符号的 FOLLOW 包含 $
    string augmented_start = productions[augmented_start_prod_id].lhs;
    follow_sets[augmented_start].insert("$");
    
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& p : productions) {
            const string& A = p.lhs;
            for (size_t i = 0; i < p.rhs.size(); ++i) {
                const string& B = p.rhs[i];
                if (isTerminal(B)) continue;
                
                // B 后面是 beta = p.rhs[i+1..]
                if (i + 1 < p.rhs.size()) {
                    vector<string> beta(p.rhs.begin() + i + 1, p.rhs.end());
                    set<string> first_beta = getFirstOfSequence(beta);
                    for (const string& s : first_beta) {
                        if (s != "ε") {
                            if (follow_sets[B].insert(s).second) changed = true;
                        }
                    }
                    if (first_beta.find("ε") != first_beta.end()) {
                        // ε ∈ FIRST(beta)，则 FOLLOW(A) ⊆ FOLLOW(B)
                        for (const string& s : follow_sets[A]) {
                            if (follow_sets[B].insert(s).second) changed = true;
                        }
                    }
                } else {
                    // B 是最后一个符号，FOLLOW(A) ⊆ FOLLOW(B)
                    for (const string& s : follow_sets[A]) {
                        if (follow_sets[B].insert(s).second) changed = true;
                    }
                }
            }
        }
    }
}

// ========== SLR(1) 分析表构造 ==========
void buildSLRTable() {
    int n_states = (int)canonical_collection.size();
    action_table.assign(n_states, map<string, string>());
    goto_table.assign(n_states, map<string, int>());
    slr_conflicts.clear();
    
    // 建立快速查找：trans_key = (from, symbol) -> to
    map<pair<int, string>, int> trans_map;
    for (const auto& t : transitions) {
        trans_map[{t.from, t.symbol}] = t.to;
    }
    
    // 填 ACTION 表
    for (int i = 0; i < n_states; ++i) {
        for (const auto& item : canonical_collection[i]) {
            const Production& p = productions[item.prod_id];
            
            if (item.dot_pos == (int)p.rhs.size()) {
                // 归约项目或接受项目
                if (item.prod_id == augmented_start_prod_id) {
                    // S' -> S.   接受
                    const string& sym = "$";
                    if (action_table[i].count(sym) && action_table[i][sym] != "acc") {
                        slr_conflicts.push_back({i, sym, action_table[i][sym], "acc", "shift-reduce/accept"});
                    } else {
                        action_table[i][sym] = "acc";
                    }
                } else {
                    // A -> α.    归约
                    string action_str = "r" + to_string(item.prod_id);
                    for (const string& a : follow_sets[p.lhs]) {
                        if (action_table[i].count(a) && action_table[i][a] != action_str) {
                            string incoming = action_str;
                            string existing = action_table[i][a];
                            string ctype = (existing[0] == 's') ? "shift-reduce" : "reduce-reduce";
                            slr_conflicts.push_back({i, a, existing, incoming, ctype});
                        } else {
                            action_table[i][a] = action_str;
                        }
                    }
                }
            } else {
                // 点不在末尾
                string next_sym = p.rhs[item.dot_pos];
                if (isTerminal(next_sym)) {
                    auto key = make_pair(i, next_sym);
                    if (trans_map.count(key)) {
                        int j = trans_map[key];
                        string action_str = "s" + to_string(j);
                        if (action_table[i].count(next_sym) && action_table[i][next_sym] != action_str) {
                            string existing = action_table[i][next_sym];
                            slr_conflicts.push_back({i, next_sym, existing, action_str, "shift-reduce"});
                        } else {
                            action_table[i][next_sym] = action_str;
                        }
                    }
                }
            }
        }
    }
    
    // 填 GOTO 表（只针对非终结符）
    for (const auto& t : transitions) {
        if (!isTerminal(t.symbol)) {
            goto_table[t.from][t.symbol] = t.to;
        }
    }
}

// ========== 格式化输出工具 ==========
string itemToString(const LR0Item& item) {
    const Production& p = productions[item.prod_id];
    stringstream ss;
    ss << p.lhs << " -> ";
    if (p.rhs.empty()) {
        ss << "ε";
        if (item.dot_pos == 0) ss << " .";
        return ss.str();
    }
    for (int i = 0; i < (int)p.rhs.size(); ++i) {
        if (i > 0) ss << " ";
        if (i == item.dot_pos) ss << ". ";
        ss << p.rhs[i];
    }
    if (item.dot_pos == (int)p.rhs.size()) {
        ss << " .";
    }
    return ss.str();
}

bool isKernel(const LR0Item& item, int state_idx) {
    if (state_idx == 0 && item.prod_id == augmented_start_prod_id && item.dot_pos == 0) {
        return true;
    }
    if (item.dot_pos > 0) return true;
    return false;
}

bool isAcceptItem(const LR0Item& item) {
    return item.prod_id == augmented_start_prod_id
        && item.dot_pos == (int)productions[item.prod_id].rhs.size();
}

// ========== 报告输出 ==========
void printReport() {
    cout << "==================================================\n";
    cout << "增广文法 (Augmented Grammar)\n";
    cout << "==================================================\n";
    for (const auto& p : productions) {
        cout << p.id << ": " << p.lhs << " -> ";
        for (size_t i = 0; i < p.rhs.size(); ++i) {
            if (i > 0) cout << " ";
            cout << p.rhs[i];
        }
        if (p.rhs.empty()) cout << "ε";
        cout << "\n";
    }
    cout << "\n";
    
    cout << "==================================================\n";
    cout << "LR(0) 项目集规范族 (Canonical Collection)\n";
    cout << "==================================================\n";
    for (size_t i = 0; i < canonical_collection.size(); ++i) {
        cout << "State " << i << ":\n";
        for (const auto& item : canonical_collection[i]) {
            cout << "  " << itemToString(item) << "\n";
        }
        cout << "Kernel: ";
        bool first = true;
        for (const auto& item : canonical_collection[i]) {
            if (isKernel(item, (int)i)) {
                if (!first) cout << ", ";
                cout << itemToString(item);
                first = false;
            }
        }
        cout << "\n\n";
    }
    
    cout << "==================================================\n";
    cout << "状态转移图 (State Transition Graph)\n";
    cout << "==================================================\n";
    vector<Transition> sorted_trans = transitions;
    sort(sorted_trans.begin(), sorted_trans.end(), [](const Transition& a, const Transition& b) {
        if (a.from != b.from) return a.from < b.from;
        if (a.symbol != b.symbol) return a.symbol < b.symbol;
        return a.to < b.to;
    });
    for (const auto& t : sorted_trans) {
        cout << t.from << " --[" << t.symbol << "]--> " << t.to << "\n";
    }
    cout << "\n";
}

void printFirstFollow() {
    cout << "==================================================\n";
    cout << "FIRST 集 与 FOLLOW 集\n";
    cout << "==================================================\n";
    
    // 按字母顺序输出非终结符（排除增广开始符号可选）
    vector<string> nt_list(non_terminals.begin(), non_terminals.end());
    sort(nt_list.begin(), nt_list.end());
    
    cout << left << setw(12) << "Non-Term" << " | "
         << left << setw(35) << "FIRST" << " | "
         << left << setw(35) << "FOLLOW" << "\n";
    cout << string(90, '-') << "\n";
    
    for (const string& nt : nt_list) {
        string first_str, follow_str;
        
        auto it_f = first_sets.find(nt);
        if (it_f != first_sets.end()) {
            for (const string& s : it_f->second) {
                if (!first_str.empty()) first_str += ", ";
                first_str += s;
            }
        }
        
        auto it_fl = follow_sets.find(nt);
        if (it_fl != follow_sets.end()) {
            for (const string& s : it_fl->second) {
                if (!follow_str.empty()) follow_str += ", ";
                follow_str += s;
            }
        }
        
        cout << left << setw(12) << nt << " | "
             << left << setw(35) << first_str << " | "
             << left << setw(35) << follow_str << "\n";
    }
    cout << "\n";
}

void printSLRTable() {
    cout << "==================================================\n";
    cout << "SLR(1) 分析表 (SLR(1) Parsing Table)\n";
    cout << "==================================================\n";
    
    int n_states = (int)canonical_collection.size();
    
    // 整理表头
    vector<string> action_cols;
    vector<string> goto_cols;
    
    // ACTION 列：终结符（排序，$ 放最后）
    vector<string> term_list(terminals.begin(), terminals.end());
    sort(term_list.begin(), term_list.end());
    for (const string& t : term_list) {
        if (t != "ε") action_cols.push_back(t);
    }
    action_cols.push_back("$");
    
    // GOTO 列：非终结符（排除增广开始符号）
    string aug_start = productions[augmented_start_prod_id].lhs;
    vector<string> nt_list(non_terminals.begin(), non_terminals.end());
    sort(nt_list.begin(), nt_list.end());
    for (const string& nt : nt_list) {
        if (nt != aug_start) goto_cols.push_back(nt);
    }
    
    // 计算列宽
    int col_width = 6;
    auto fmt = [&](const string& s) -> string {
        string res = s;
        while ((int)res.length() < col_width) res += " ";
        return res;
    };
    
    // 打印表头
    cout << "      ";
    for (const string& c : action_cols) cout << fmt(c);
    cout << "| ";
    for (const string& c : goto_cols) cout << fmt(c);
    cout << "\n";
    
    cout << "------";
    for (size_t i = 0; i < action_cols.size(); ++i) cout << string(col_width, '-');
    cout << "+-";
    for (size_t i = 0; i < goto_cols.size(); ++i) cout << string(col_width, '-');
    cout << "\n";
    
    // 打印每行
    for (int i = 0; i < n_states; ++i) {
        cout << fmt(to_string(i));
        for (const string& c : action_cols) {
            auto it = action_table[i].find(c);
            if (it != action_table[i].end()) {
                cout << fmt(it->second);
            } else {
                cout << fmt("");
            }
        }
        cout << "| ";
        for (const string& c : goto_cols) {
            auto it = goto_table[i].find(c);
            if (it != goto_table[i].end()) {
                cout << fmt(to_string(it->second));
            } else {
                cout << fmt("");
            }
        }
        cout << "\n";
    }
    cout << "\n";
    
    // 图例
    cout << "图例: sX = 移进至状态 X,  rX = 按第 X 条产生式归约,  acc = 接受\n";
    cout << "      空白 = 报错 (error)\n\n";
}

void printConflicts() {
    cout << "==================================================\n";
    cout << "SLR(1) 冲突检测 (Conflict Detection)\n";
    cout << "==================================================\n";
    if (slr_conflicts.empty()) {
        cout << "✅ 无冲突，该文法是 SLR(1) 文法。\n";
    } else {
        for (const auto& c : slr_conflicts) {
            cout << "❌ State " << c.state << " on symbol '" << c.symbol << "': "
                 << c.type << " conflict\n";
            cout << "   Existing: " << c.existing << ", Incoming: " << c.incoming << "\n";
        }
        cout << "\n⚠️  文法存在冲突，不是 SLR(1) 文法。\n";
    }
    cout << "\n";
}

// 导出 SLR 分析表到文件（供实验五读取）
bool saveSLRTable(const string& out_filename) {
    ofstream out(out_filename);
    if (!out.is_open()) { cerr << "Cannot write " << out_filename << endl; return false; }
    out << "# SLR(1) TABLE\n";
    out << "# Grammar file reference (must match exp5 grammar)\n";
    out << "AUG_START " << productions[augmented_start_prod_id].lhs << "\n";
    out << "PROD_COUNT " << productions.size() << "\n";
    out << "STATES " << canonical_collection.size() << "\n";
    out << "# ACTION table entries: state symbol action\n";
    for (size_t i = 0; i < action_table.size(); ++i) {
        for (const auto& kv : action_table[i]) {
            out << "ACTION " << i << " " << kv.first << " " << kv.second << "\n";
        }
    }
    out << "# GOTO table entries: state non_terminal next_state\n";
    for (size_t i = 0; i < goto_table.size(); ++i) {
        for (const auto& kv : goto_table[i]) {
            out << "GOTO " << i << " " << kv.first << " " << kv.second << "\n";
        }
    }
    out.close();
    return true;
}

// ========== 主函数 ==========
int main(int argc, char* argv[]) {
    string filename = "grammar_expr.txt";
    string out_table;
    if (argc > 1) filename = argv[1];
    if (argc > 3 && string(argv[2]) == "--save-table") out_table = argv[3];

    if (!parseGrammar(filename)) {
        return 1;
    }

    canonical_collection = buildCanonicalCollection(transitions);

    computeFirst();
    computeFollow();
    buildSLRTable();

    printReport();
    printFirstFollow();
    printSLRTable();
    printConflicts();

    if (!out_table.empty()) {
        if (saveSLRTable(out_table)) {
            cout << "\n✅ SLR(1) 分析表已导出到: " << out_table << "\n";
        }
    }

    return 0;
}
