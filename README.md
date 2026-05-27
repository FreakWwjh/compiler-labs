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

docs/exp1/               # 实验一效果参考图（四张目标效果图）
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
- [ ] **实验三**：LR(0) / SLR(1) 语法分析（规划中：项目集规范族、分析表构造）

## 📄 许可证

本项目采用 [MIT License](exp1/LICENSE) 开源。
