#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <queue>
#include <algorithm>
#include <cctype>

using namespace std;

struct DFA {
    vector<string> alphabet;           // 字符集
    int stateCount;                    // 状态数量
    int startState;                    // 开始状态
    set<int> acceptStates;             // 接受状态集
    map<int, map<string, int>> transitions; // 状态转换表: state -> (symbol -> nextState)
};

// 去除字符串首尾空白
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// 分割字符串
vector<string> split(const string& s) {
    vector<string> tokens;
    stringstream ss(s);
    string token;
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// 读取DFA文件
bool loadDFA(const string& filename, DFA& dfa) {
    ifstream fin(filename);
    if (!fin.is_open()) {
        cerr << "错误：无法打开文件 " << filename << endl;
        return false;
    }

    vector<string> lines;
    string line;
    while (getline(fin, line)) {
        line = trim(line);
        if (!line.empty()) lines.push_back(line);
    }
    fin.close();

    if (lines.size() < 5) {
        cerr << "错误：DFA文件格式不正确，行数不足。" << endl;
        return false;
    }

    // 第1行：字符集
    // 如果行内包含空格，按空格分割（支持多字符符号如 ==, ++）
    // 否则每个非空白字符作为一个符号（兼容 ab 写法表示 a, b）
    bool hasSpace = false;
    for (char c : lines[0]) {
        if (isspace(static_cast<unsigned char>(c))) { hasSpace = true; break; }
    }
    if (hasSpace) {
        vector<string> alphaTokens = split(lines[0]);
        dfa.alphabet = alphaTokens;
    } else {
        for (char c : lines[0]) {
            if (!isspace(static_cast<unsigned char>(c))) {
                dfa.alphabet.push_back(string(1, c));
            }
        }
    }

    if (dfa.alphabet.empty()) {
        cerr << "错误：字符集为空。" << endl;
        return false;
    }

    // 第2行：状态数量
    dfa.stateCount = stoi(lines[1]);
    if (dfa.stateCount <= 0) {
        cerr << "错误：状态数量必须为正整数。" << endl;
        return false;
    }

    // 第3行：开始状态
    dfa.startState = stoi(lines[2]);

    // 第4行：接受状态集
    vector<string> acceptTokens = split(lines[3]);
    for (const string& tok : acceptTokens) {
        dfa.acceptStates.insert(stoi(tok));
    }

    // 检查转移表行数
    int transRows = dfa.stateCount;
    int expectedLines = 4 + transRows;
    if ((int)lines.size() < expectedLines) {
        cerr << "错误：转移表行数不足，期望 " << transRows << " 行，实际 " 
             << (lines.size() - 4) << " 行。" << endl;
        return false;
    }

    // 读取转移表: 状态编号从1开始
    for (int i = 0; i < transRows; i++) {
        int stateId = i + 1;
        vector<string> rowTokens = split(lines[4 + i]);
        if ((int)rowTokens.size() != (int)dfa.alphabet.size()) {
            cerr << "错误：状态 " << stateId << " 的转移表列数不正确，期望 " 
                 << dfa.alphabet.size() << "，实际 " << rowTokens.size() << "。" << endl;
            return false;
        }
        for (size_t j = 0; j < dfa.alphabet.size(); j++) {
            int nextState = stoi(rowTokens[j]);
            dfa.transitions[stateId][dfa.alphabet[j]] = nextState;
        }
    }

    return true;
}

// 校验DFA
bool validateDFA(const DFA& dfa) {
    bool valid = true;
    cout << "\n========== DFA 校验 ==========" << endl;

    // 1. 开始状态唯一性（文件中只有一个开始状态）
    cout << "[1] 开始状态: " << dfa.startState << " (唯一)" << endl;

    // 2. 开始状态是否在状态集中
    if (dfa.startState < 1 || dfa.startState > dfa.stateCount) {
        cerr << "  -> 错误：开始状态 " << dfa.startState << " 不在状态集 [1, " 
             << dfa.stateCount << "] 中！" << endl;
        valid = false;
    } else {
        cout << "  -> 通过：开始状态包含在状态集中。" << endl;
    }

    // 3. 接受状态集是否为空
    if (dfa.acceptStates.empty()) {
        cerr << "  -> 错误：接受状态集为空！" << endl;
        valid = false;
    } else {
        cout << "[2] 接受状态集: ";
        for (int s : dfa.acceptStates) cout << s << " ";
        cout << "(非空)" << endl;
    }

    // 4. 接受状态集是否包含在状态集中
    bool allAcceptValid = true;
    for (int s : dfa.acceptStates) {
        if (s < 1 || s > dfa.stateCount) {
            cerr << "  -> 错误：接受状态 " << s << " 不在状态集 [1, " 
                 << dfa.stateCount << "] 中！" << endl;
            allAcceptValid = false;
            valid = false;
        }
    }
    if (allAcceptValid && !dfa.acceptStates.empty()) {
        cout << "  -> 通过：所有接受状态均包含在状态集中。" << endl;
    }

    // 5. 检查转移表完整性
    bool transComplete = true;
    for (int s = 1; s <= dfa.stateCount; s++) {
        auto it = dfa.transitions.find(s);
        if (it == dfa.transitions.end()) {
            cerr << "  -> 错误：状态 " << s << " 缺少转移定义。" << endl;
            transComplete = false;
            valid = false;
            continue;
        }
        for (const string& sym : dfa.alphabet) {
            if (it->second.find(sym) == it->second.end()) {
                cerr << "  -> 错误：状态 " << s << " 缺少字符 '" << sym << "' 的转移。" << endl;
                transComplete = false;
                valid = false;
            }
        }
    }
    if (transComplete) {
        cout << "[3] 转移表完整性: 通过" << endl;
    }

    cout << "================================" << endl;
    if (valid) {
        cout << "DFA 校验通过！" << endl;
    } else {
        cout << "DFA 校验未通过，请检查配置文件。" << endl;
    }
    cout << endl;
    return valid;
}

// 输出长度 <= N 的所有规则字符串
void generateStrings(const DFA& dfa, int maxLen) {
    cout << "\n========== 生成规则字符串（长度 <= " << maxLen << "）==========" << endl;
    
    if (maxLen < 0) {
        cout << "最大长度不能为负数。" << endl;
        return;
    }

    // BFS: (当前状态, 已构造的字符串)
    // 为了按字典序输出，我们按长度分层，每层内按字典序
    vector<string> result;
    
    // 检查空串
    if (dfa.acceptStates.count(dfa.startState)) {
        result.push_back("<空串>");
    }

    queue<pair<int, string>> q;
    q.push({dfa.startState, ""});

    while (!q.empty()) {
        auto [state, str] = q.front();
        q.pop();
        if ((int)str.length() >= maxLen) continue;
        
        for (const string& sym : dfa.alphabet) {
            auto it = dfa.transitions.find(state);
            if (it == dfa.transitions.end()) continue;
            auto it2 = it->second.find(sym);
            if (it2 == it->second.end()) continue;
            int nextState = it2->second;
            string nextStr = str + sym;
            if (dfa.acceptStates.count(nextState)) {
                result.push_back(nextStr);
            }
            q.push({nextState, nextStr});
        }
    }

    if (result.empty()) {
        cout << "无规则字符串。" << endl;
    } else {
        // 去重并排序（BFS本身可能产生重复路径到同一状态的同一字符串，但这里字符串唯一）
        // 实际上BFS按字符串扩展，每个字符串路径唯一，所以不会重复
        for (const string& s : result) {
            cout << s << endl;
        }
        cout << "共 " << result.size() << " 个规则字符串。" << endl;
    }
    cout << "=====================================================" << endl;
}

// 模拟DFA判定字符串
bool simulateDFA(const DFA& dfa, const string& input, bool verbose = true) {
    if (verbose) {
        cout << "\n---------- DFA 模拟运行 ----------" << endl;
        cout << "输入字符串: \"" << input << "\"" << endl;
        cout << "初始状态: q" << dfa.startState << endl;
    }

    int currentState = dfa.startState;
    string processed = "";

    for (size_t i = 0; i < input.length(); i++) {
        string sym(1, input[i]);
        
        // 检查字符是否在字符集中
        bool inAlphabet = false;
        for (const string& a : dfa.alphabet) {
            if (a == sym) {
                inAlphabet = true;
                break;
            }
        }
        if (!inAlphabet) {
            if (verbose) {
                cout << "步骤 " << (i+1) << ": 字符 '" << input[i] 
                     << "' 不在字符集中，DFA 崩溃（拒绝）。" << endl;
            }
            return false;
        }

        auto it = dfa.transitions.find(currentState);
        if (it == dfa.transitions.end() || it->second.find(sym) == it->second.end()) {
            if (verbose) {
                cout << "步骤 " << (i+1) << ": 状态 q" << currentState 
                     << " 对字符 '" << input[i] << "' 无转移定义，DFA 崩溃（拒绝）。" << endl;
            }
            return false;
        }

        int nextState = it->second.at(sym);
        if (verbose) {
            cout << "步骤 " << (i+1) << ": δ(q" << currentState << ", '" 
                 << input[i] << "') = q" << nextState << endl;
        }
        currentState = nextState;
        processed += input[i];
    }

    bool accepted = dfa.acceptStates.count(currentState);
    if (verbose) {
        cout << "结束状态: q" << currentState;
        if (accepted) {
            cout << " (接受状态)" << endl;
            cout << "结果: Accepted（属于该 DFA 语言集）" << endl;
        } else {
            cout << " (非接受状态)" << endl;
            cout << "结果: Rejected（不属于该 DFA 语言集）" << endl;
        }
        cout << "------------------------------------" << endl;
    }
    return accepted;
}

// 功能4：ASCII 状态转移图可视化
void drawAsciiGraph(const DFA& dfa) {
    cout << "\n========== DFA ASCII 状态转移图 ==========" << endl;
    cout << "\n  图例:" << endl;
    cout << "    [N]    表示状态 N" << endl;
    cout << "    [N](*) 表示接受状态 N" << endl;
    cout << "    [N](S) 表示开始状态 N" << endl;
    cout << "    [N](S*)(*) 表示既是开始又是接受状态 N" << endl;
    cout << "    --x--> 表示在字符 x 上的转移\n" << endl;

    for (int s = 1; s <= dfa.stateCount; s++) {
        string label = "[" + to_string(s) + "]";
        if (s == dfa.startState && dfa.acceptStates.count(s)) {
            label = "[" + to_string(s) + "](S*)(*)";
        } else if (s == dfa.startState) {
            label = "[" + to_string(s) + "](S)";
        } else if (dfa.acceptStates.count(s)) {
            label = "[" + to_string(s) + "](*)";
        }

        auto it = dfa.transitions.find(s);
        if (it == dfa.transitions.end()) continue;

        bool first = true;
        for (const auto& kv : it->second) {
            const string& sym = kv.first;
            int next = kv.second;
            if (first) {
                cout << "  " << label;
                first = false;
            } else {
                cout << string(label.length() + 2, ' ');
            }
            cout << " --" << sym << "--> [" << next << "]" << endl;
        }
        if (first) {
            cout << "  " << label << " (无出边)" << endl;
        }
    }
    cout << "===========================================" << endl;
}

// 功能5：导出 DFA 为 JSON 格式
void exportToJson(const DFA& dfa, const string& outFile) {
    ofstream fout(outFile);
    if (!fout.is_open()) {
        cerr << "错误：无法创建文件 " << outFile << endl;
        return;
    }

    fout << "{\n";

    // alphabet
    fout << "    \"alphabet\": [";
    for (size_t i = 0; i < dfa.alphabet.size(); i++) {
        fout << "\"" << dfa.alphabet[i] << "\"";
        if (i + 1 < dfa.alphabet.size()) fout << ", ";
    }
    fout << "],\n";

    // states
    fout << "    \"states\": [";
    for (int i = 1; i <= dfa.stateCount; i++) {
        fout << i;
        if (i < dfa.stateCount) fout << ", ";
    }
    fout << "],\n";

    // start_state
    fout << "    \"start_state\": " << dfa.startState << ",\n";

    // accept_states
    fout << "    \"accept_states\": [";
    size_t idx = 0;
    for (int s : dfa.acceptStates) {
        fout << s;
        if (++idx < dfa.acceptStates.size()) fout << ", ";
    }
    fout << "],\n";

    // transitions
    fout << "    \"transitions\": {\n";
    for (int s = 1; s <= dfa.stateCount; s++) {
        fout << "        \"" << s << "\": {";
        auto it = dfa.transitions.find(s);
        if (it != dfa.transitions.end()) {
            size_t j = 0;
            for (const auto& kv : it->second) {
                fout << "\"" << kv.first << "\": " << kv.second;
                if (++j < it->second.size()) fout << ", ";
            }
        }
        fout << "}";
        if (s < dfa.stateCount) fout << ",";
        fout << "\n";
    }
    fout << "    }\n";
    fout << "}\n";

    fout.close();
    cout << "DFA 已成功导出到: " << outFile << endl;
}

// 交互式菜单
void interactiveMenu(const DFA& dfa) {
    int choice;
    while (true) {
        cout << "\n================ DFA 模拟器菜单 ================" << endl;
        cout << "1. 输出长度 ≤ N 的所有规则字符串" << endl;
        cout << "2. 判定字符串是否属于 DFA 语言集" << endl;
        cout << "3. 随机生成字符串并判定" << endl;
        cout << "4. 可视化：ASCII 状态转移图" << endl;
        cout << "5. 导出 DFA 为 JSON 格式" << endl;
        cout << "0. 退出程序" << endl;
        cout << "================================================" << endl;
        cout << "请选择功能: ";
        cin >> choice;

        if (choice == 0) {
            cout << "再见！" << endl;
            break;
        } else if (choice == 1) {
            int N;
            cout << "请输入最大长度 N: ";
            cin >> N;
            generateStrings(dfa, N);
        } else if (choice == 2) {
            string input;
            cout << "请输入待判定的字符串（输入 'exit' 退出到菜单）: ";
            cin >> input;
            if (input == "exit") continue;
            simulateDFA(dfa, input);
        } else if (choice == 3) {
            int len;
            cout << "请输入随机字符串长度: ";
            cin >> len;
            if (len < 0) {
                cout << "长度不能为负数。" << endl;
                continue;
            }
            string randomStr;
            for (int i = 0; i < len; i++) {
                randomStr += dfa.alphabet[rand() % dfa.alphabet.size()];
            }
            cout << "随机生成字符串: \"" << randomStr << "\"" << endl;
            simulateDFA(dfa, randomStr);
        } else if (choice == 4) {
            drawAsciiGraph(dfa);
        } else if (choice == 5) {
            string outFile;
            cout << "请输入导出文件名（默认 dfa_export.json）: ";
            cin >> outFile;
            if (outFile.empty()) outFile = "dfa_export.json";
            exportToJson(dfa, outFile);
        } else {
            cout << "无效选择，请重新输入。" << endl;
        }
    }
}

int main(int argc, char* argv[]) {
    string filename;
    if (argc >= 2) {
        filename = argv[1];
    } else {
        filename = "dfa_in1.dfa";
    }

    cout << "=====================================================" << endl;
    cout << "    DFA 模拟器 - 编译原理实验一" << endl;
    cout << "    运行环境: " << "openEuler aarch64 (鲲鹏开发板)" << endl;
    cout << "=====================================================" << endl;
    cout << "正在加载 DFA 配置文件: " << filename << endl;

    DFA dfa;
    if (!loadDFA(filename, dfa)) {
        return 1;
    }

    if (!validateDFA(dfa)) {
        return 1;
    }

    // 显示DFA基本信息
    cout << "\n========== DFA 基本信息 ==========" << endl;
    cout << "字符集: ";
    for (const string& s : dfa.alphabet) cout << "\"" << s << "\" ";
    cout << endl;
    cout << "状态集: {1, 2, ..., " << dfa.stateCount << "}" << endl;
    cout << "开始状态: " << dfa.startState << endl;
    cout << "接受状态集: ";
    for (int s : dfa.acceptStates) cout << s << " ";
    cout << endl;
    cout << "状态转换表:" << endl;
    cout << "状态\t";
    for (const string& s : dfa.alphabet) cout << s << "\t";
    cout << endl;
    for (int i = 1; i <= dfa.stateCount; i++) {
        cout << i << "\t";
        for (const string& s : dfa.alphabet) {
            cout << dfa.transitions.at(i).at(s) << "\t";
        }
        cout << endl;
    }
    cout << "====================================" << endl;

    interactiveMenu(dfa);
    return 0;
}
