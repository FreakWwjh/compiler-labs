#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <algorithm>
#include <string>

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

vector<Production> productions;
set<string> non_terminals;
set<string> terminals;
string start_symbol;
int augmented_start_prod_id = -1;

bool isTerminal(const string& sym) {
    return non_terminals.find(sym) == non_terminals.end();
}

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
    
    return true;
}

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

vector<ItemSet> buildCanonicalCollection(vector<Transition>& transitions) {
    vector<ItemSet> collection;
    transitions.clear();
    
    ItemSet I0;
    I0.insert({augmented_start_prod_id, 0});
    I0 = closure(I0);
    collection.push_back(I0);
    
    queue<int> q;
    q.push(0);
    
    while (!q.empty()) {
        int idx = q.front(); q.pop();
        ItemSet I = collection[idx]; // 拷贝，避免 push_back 后引用失效

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
            transitions.push_back({idx, X, jdx});
        }
    }
    
    return collection;
}

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

struct Conflict {
    int state;
    string type;
    string desc;
};

bool isAcceptItem(const LR0Item& item) {
    return item.prod_id == augmented_start_prod_id
        && item.dot_pos == (int)productions[item.prod_id].rhs.size();
}

vector<Conflict> detectConflicts(const vector<ItemSet>& collection) {
    vector<Conflict> conflicts;

    for (size_t i = 0; i < collection.size(); ++i) {
        bool has_reduce = false;
        bool has_shift = false;
        vector<int> reduce_prods;

        for (const auto& item : collection[i]) {
            const Production& p = productions[item.prod_id];
            if (item.dot_pos == (int)p.rhs.size()) {
                if (!isAcceptItem(item)) {
                    has_reduce = true;
                    reduce_prods.push_back(item.prod_id);
                }
            } else if (item.dot_pos < (int)p.rhs.size()) {
                string next = p.rhs[item.dot_pos];
                if (isTerminal(next)) {
                    has_shift = true;
                }
            }
        }

        if (has_shift && has_reduce) {
            stringstream ss;
            ss << "State " << i << ": 移进-归约冲突\n";
            for (const auto& item : collection[i]) {
                const Production& p = productions[item.prod_id];
                if (item.dot_pos == (int)p.rhs.size() && !isAcceptItem(item)) {
                    ss << "  [归约] " << itemToString(item) << "\n";
                } else if (item.dot_pos < (int)p.rhs.size() && isTerminal(p.rhs[item.dot_pos])) {
                    ss << "  [移进] " << itemToString(item) << "\n";
                }
            }
            conflicts.push_back({(int)i, "shift-reduce", ss.str()});
        }

        if (reduce_prods.size() > 1) {
            stringstream ss;
            ss << "State " << i << ": 归约-归约冲突\n";
            for (int pid : reduce_prods) {
                ss << "  [归约] " << itemToString({pid, (int)productions[pid].rhs.size()}) << "\n";
            }
            conflicts.push_back({(int)i, "reduce-reduce", ss.str()});
        }
    }

    return conflicts;
}

void printReport(const vector<ItemSet>& collection, const vector<Transition>& transitions, const vector<Conflict>& conflicts) {
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
    cout << "LR(0) 项目集规范族\n";
    cout << "==================================================\n";
    for (size_t i = 0; i < collection.size(); ++i) {
        cout << "State " << i << ":\n";
        for (const auto& item : collection[i]) {
            cout << "  " << itemToString(item) << "\n";
        }
        cout << "Kernel: ";
        bool first = true;
        for (const auto& item : collection[i]) {
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
        cout << t.from << " --" << t.symbol << "--> " << t.to << "\n";
    }
    cout << "\n";
    
    cout << "==================================================\n";
    cout << "LR(0) 冲突检测\n";
    cout << "==================================================\n";
    if (conflicts.empty()) {
        cout << "✅ 无冲突，该文法是 LR(0) 文法。\n";
    } else {
        for (const auto& c : conflicts) {
            cout << c.desc << "\n";
        }
        cout << "\n❌ 文法存在冲突，不是 LR(0) 文法。\n";
    }
}

int main(int argc, char* argv[]) {
    string filename = "grammar_expr.txt";
    if (argc > 1) filename = argv[1];

    if (!parseGrammar(filename)) {
        return 1;
    }

    vector<Transition> transitions;
    vector<ItemSet> collection = buildCanonicalCollection(transitions);
    vector<Conflict> conflicts = detectConflicts(collection);

    printReport(collection, transitions, conflicts);

    return 0;
}
