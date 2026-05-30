# 编译原理实验套件

本仓库收录编译原理课程系列实验代码，在 **openEuler aarch64 (鲲鹏)** 环境下开发测试，并提供了 Web 可视化版本用于直观演示。

## 📁 项目结构

```
CompilationWeb/          # Web 可视化演示平台（实验一 DFA 模拟 + 实验二 词法分析）
├── index.html           # 主页面：包含侧边栏导航和所有实验模块的 DOM 结构
├── style.css            # 样式文件：页面切换动画与定制 UI 细节
├── app.js               # 核心交互：控制多级网页的切换与通用 UI 逻辑
├── dfa.js               # 实验一模块：DFA 核心类（解析、校验、生成字符串）及可视化
├── dfa.txt              # 测试用例：标准的 DFA 状态转移五元组定义样例
└── .trae/               # AI 辅助开发目录：存放项目专属开发规范 Skill

exp1/                    # 实验一：C++ 命令行 DFA 模拟器（openEuler 鲲鹏板原生环境）
├── dfa_simulator.cpp    # DFA 模拟器主程序
├── Makefile             # 编译脚本
├── dfa_in*.dfa          # 示例 DFA 配置文件
└── ...

exp2/                    # 实验二：词法分析器 Scanner
├── scanner.cpp          # 词法分析器主程序（C++17，直接编码）
├── scanner.c            # 词法分析器主程序（C99，直接编码）
├── scanner_dfa.cpp      # DFA 驱动版 Scanner（读取实验一 JSON 配置）
├── dfa_lex.json         # 扩展后的 DFA 配置文件（覆盖主文法）
├── gen_dfa.py           # 生成 dfa_lex.json 的脚本
├── Makefile             # 编译脚本（支持 scanner / scanner_c / scanner_dfa）
├── test.src             # 示例源程序
└── ...

exp3/                    # 实验三：LR(0) 项目集规范族生成器
├── lr0.cpp              # LR(0) 规范族自动生成器（Closure + Goto + BFS）
├── Makefile
├── grammar_expr.txt     # 标准表达式文法（存在 LR(0) 冲突）
├── grammar_ext.txt      # 扩展表达式文法（含一元负号）
├── grammar_lr0.txt      # 简单 LR(0) 文法（无冲突）
└── ...

exp4/                    # 实验四：SLR(1) 分析表生成器
├── slr1.cpp             # SLR(1) 分析表生成器（FIRST/FOLLOW + ACTION/GOTO）
├── Makefile
├── grammar_*.txt        # 三套测试文法
├── optional_flex_bison/ # 选做：Flex + Bison 表达式计算器
└── ...

exp5/                    # 实验五：SLR 引导的语义分析框架
├── semantic_analyzer.cpp # 语义分析器（AST + 符号表 + 类型检查 + 三地址码）
├── Makefile
└── ...

docs/exp1/               # 实验一效果参考图（四张目标效果图）
docs/exp3/               # 实验三实验报告
docs/exp4/               # 实验四实验报告与截图
docs/exp5/               # 实验五实验报告
```

## 🚀 快速开始

### Web 可视化版本（推荐）

无需安装，直接在浏览器中打开：

```bash
# 方式1：本地直接打开
cd CompilationWeb
# 用浏览器打开 index.html 即可

# 方式2：本地启动简易 HTTP 服务器（避免浏览器安全限制）
cd CompilationWeb
python3 -m http.server 8080
# 然后访问 http://localhost:8080
```

> 💡 **GitHub Pages 部署**：将 `CompilationWeb/` 目录内容推送到仓库的 `gh-pages` 分支，或在仓库 Settings > Pages 中将源目录设为 `/CompilationWeb`（或根目录），即可在线访问。

### C++ 命令行版本（openEuler / 鲲鹏）

**实验一：DFA 模拟器**

```bash
cd exp1
make clean && make
./dfa_sim dfa_in1.dfa
```

**实验二：词法分析器 Scanner**

```bash
cd exp2
make
./scanner test.src          # 文件扫描模式（直接编码 C++ 版）
./scanner                   # 交互模式（支持模式 1 / 模式 2）
./scanner test.src > tokens.txt   # 输出重定向，供后续语法分析使用
```

**实验二（DFA 驱动版，体现实验一与实验二关联）**

```bash
cd exp2
make scanner_dfa
./scanner_dfa dfa_lex.json test.src   # 读取 DFA 配置文件进行词法分析
```

**实验三：LR(0) 项目集规范族生成器**

```bash
cd exp3
make
./lr0 grammar_expr.txt        # 标准表达式文法（存在 LR(0) 冲突）
./lr0 grammar_lr0.txt         # 简单 LR(0) 文法（无冲突）
```

**实验四：SLR(1) 分析表生成器**

```bash
cd exp4
make
./slr1 grammar_expr.txt       # 标准表达式文法（SLR(1)，无冲突）
./slr1 grammar_ext.txt        # 扩展表达式文法（含一元负号）
make test                     # 一键测试所有文法
```

**实验四选做：Flex / Bison 体验**

```bash
cd exp4/optional_flex_bison
make
make test                     # 运行表达式计算器测试用例
```

**实验五：SLR 引导的语义分析框架**

```bash
cd exp5
make
./semantic_analyzer grammar.txt test1.txt
```

## ✨ 实验功能特性

### 实验一：DFA 模拟器

| 功能 | C++ 版 | Web 版 |
|------|--------|--------|
| DFA 五元组文件输入 | ✅ `.dfa` 矩阵格式 | ✅ 表单输入 + 文件上传 |
| DFA 合法性校验 | ✅ | ✅ |
| 生成长度 ≤ N 的接受字符串 | ✅ BFS | ✅ BFS |
| 字符串接受判定 | ✅ 逐步模拟 | ✅ 逐步模拟 |
| 状态转移图可视化 | ✅ ASCII 字符图 | ✅ **vis-network 图形化** |
| JSON 导出 | ✅ | ✅（内置） |
| 随机字符串生成 | ✅ | - |
| 多字符符号支持 | ✅ | ✅ |

### 实验二：词法分析器 Scanner

| 功能 | C++ / C 版 | Web 版 | 说明 |
|------|-----------|--------|------|
| 关键字识别 | ✅ | ✅ | `int`/`float`/`void`/`if`/`else`/`while`/`return`/`input`/`print` |
| 标识符 / 整数 / 浮点数 | ✅ | ✅ | 支持小数形式与科学计数法 |
| 运算符与界符 | ✅ | ✅ | `+ - * / = < <= == > >= != ; ( ) { } [ ] ,` |
| 交互双模式 | ✅ | - | 模式 1：逐个单词识别；模式 2：整行语句扫描 |
| 文件批量扫描 | ✅ | ✅ | 支持上传 `.src`/`.c`/`.txt` |
| 错误字符标记 | ✅ | ✅ | 对不支持的字符输出 `UNKNOWN` |
| Token 颜色区分 | - | ✅ | 关键字、标识符、数字、运算符、界符分色显示 |
| 源码预览高亮 | - | ✅ | 源码预览区域 |
| C + C++ 双实现 | ✅ | - | 同一 Makefile 支持 `scanner` 与 `scanner_c` |
| **DFA 配置文件驱动** | ✅ | - | `scanner_dfa` 读取实验一格式的 JSON 运行，体现实验关联性 |

### 实验三：LR(0) 项目集规范族生成器

| 功能 | 说明 |
|------|------|
| 文法自动增广 | 自动插入 `S' → S`，统一编号从 0 开始 |
| Closure 闭包计算 | 递归加载非终结符的所有产生式，直至不动点 |
| Goto 状态转移 | 圆点后移 + 闭包，构建状态间转移边 |
| 规范族 BFS 构造 | 从 `I₀` 出发，自动生成全部 LR(0) 项目集 |
| 冲突检测 | 自动识别移进-归约冲突与归约-归约冲突，排除接受项目 |
| 格式化输出 | 增广文法、项目集（含 Kernel 标记）、状态转移图 |

### 实验四：SLR(1) 分析表生成器

| 功能 | 说明 |
|------|------|
| FIRST 集计算 | 不动点迭代，支持 ε 产生式 |
| FOLLOW 集计算 | 基于产生式的不动点迭代，自动处理尾部空串情况 |
| ACTION 表构造 | 移进 `sX`、归约 `rX`、接受 `acc`，按 FOLLOW 约束填表 |
| GOTO 表构造 | 非终结符转移状态自动填入 |
| SLR(1) 冲突检测 | 检测并报告移进-归约 / 归约-归约冲突 |
| 格式化表格输出 | ACTION（终结符 + `$`）与 GOTO（非终结符）合并对齐展示 |
| **选做：Flex/Bison** | 表达式计算器，体验自动化词法/语法分析工具 |

### 实验五：SLR 引导的语义分析框架

| 功能 | 说明 |
|------|------|
| 抽象语法树（AST）| 在归约时构建 AST 节点，支持可视化打印 |
| 符号表管理 | 作用域栈结构，支持变量声明、查询与重复检测 |
| 类型检查 | 表达式类型推导、赋值类型匹配检测 |
| 语义错误报告 | 未声明变量、重复声明、类型不匹配等 |
| 三地址码生成 | 将 AST 翻译为四元式/三地址码序列 |

## 🖼️ 效果预览

目标效果参见 `docs/exp1/` 下的四张参考图：
- 项目结构 & 开发计划
- DFA 定义与状态转移图（未解析状态）
- DFA 定义与状态转移图（解析成功）
- 功能测试（字符串生成 & 验证）

## 🛠️ 环境信息

- **OS**: openEuler 22.03 LTS (aarch64)
- **Compiler**: g++ 10.3.1 (支持 C++17)
- **构建工具**: GNU Make
- **Web**: 现代浏览器（Chrome / Firefox / Edge）

## 📋 当前与后续开发计划

- [x] **实验一**：DFA 模拟与可视化（五元组解析、合法性验证、输入字符串匹配、生成指定长度内的所有接受字符串、自动渲染状态转移图）
- [x] **实验二**：词法分析器 Scanner 实现（关键字 / 标识符 / 整数 / 浮点数 / 运算符 / 界符识别，支持交互双模式与文件批量扫描）
- [x] **实验三**：LR(0) 项目集规范族自动生成器（Closure、Goto、BFS 规范族构造、移进-归约与归约-归约冲突检测）
- [x] **实验四**：SLR(1) 分析表生成器（FIRST/FOLLOW 集计算、ACTION/GOTO 二维表构造、SLR(1) 冲突检测、Flex/Bison 自动工具体验）
- [x] **实验五**：SLR 引导的语义分析框架（AST 构建、符号表管理、类型检查、三地址码生成）

## 📄 许可证

本项目采用 [MIT License](exp1/LICENSE) 开源。
