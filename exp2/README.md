# 实验二：词法分析器 Scanner 实现

## 实验目标

实现词法分析扫描程序（Scanner），支持对自定义程序语言的源代码进行线性扫描，识别并输出 Token 流。

提供 **C++** 与 **C** 两种命令行实现，并与实验一 Web 可视化平台联动。

## 支持的词法单元

| Token 类型 | 说明 | 示例 |
|-----------|------|------|
| `INT` / `FLOAT` / `VOID` / `IF` / `ELSE` / `WHILE` / `RETURN` / `INPUT` / `PRINT` | 关键字（保留字） | `int`, `while` |
| `ID` | 标识符（字母/下划线开头） | `a`, `num123`, `_tmp` |
| `NUM` | 整数常量 | `0`, `123` |
| `FLOAT` | 浮点数常量（小数形式 / 科学计数法） | `123.45`, `.66`, `1e-3`, `9.8E7` |
| `ADD` | 加号 `+` | |
| `SUB` | 减号 `-` | |
| `MUL` | 乘号 `*` | |
| `DIV` | 除号 `/` | |
| `LT` / `LE` / `EQ` / `GT` / `GE` / `NE` | 关系运算符 | `<`, `<=`, `==`, `>`, `>=`, `!=` |
| `ASG` | 赋值号 `=` | |
| `SEMI` | 分号 `;` | |
| `LPAR` / `RPAR` | 左右圆括号 `(`, `)` | |
| `LBR` / `RBR` | 左右花括号 `{`, `}` | |
| `LBK` / `RBK` | 左右方括号 `[`, `]` | |
| `CMA` | 逗号 `,` | |

> 空白符（空格、制表符、换行）自动跳过，不生成 Token。

## 程序运行模式

### 模式 1：逐个符号串识别

先输入 `1`，再输入符号串个数 `n`，随后输入 `n` 个用空格分隔的符号串，程序输出每个串对应的类型名。

```bash
$ ./scanner
1
5
id if 485 841.6541 www
```

输出：
```
ID
IF
NUM
FLOAT
ID
```

### 模式 2：语句词法分析

先输入 `2`，随后输入一行语句，程序对该语句进行词法分析并输出每个单词的类型。

```bash
$ ./scanner
2
while(true) {int a=0;}
```

输出：
```
WHILE
LPAR
ID
RPAR
LBR
INT
ID
ASG
NUM
SEMI
RBR
```

### 文件输入模式（推荐用于批量测试）

```bash
$ ./scanner test.src
```

输出格式为 `(TYPE, value)`，可直接重定向为后续语法分析的输入：

```bash
$ ./scanner test.src > tokens.txt
```

## 编译与运行

```bash
cd exp2
make           # 编译 C++ 版 (scanner) 和 C 版 (scanner_c)
make test      # 测试 C++ 版本
make test-c    # 测试 C 版本
make clean     # 清理编译产物
```

### C++ 版本

```bash
make scanner
./scanner test.src          # 文件扫描模式
./scanner                   # 交互模式（支持模式 1 / 模式 2）
```

### C 版本

```bash
make scanner_c
./scanner_c test.src
./scanner_c
```

> 两种实现功能完全一致，C 版本更贴近底层字符处理，便于理解词法分析的基本原理。

### DFA 驱动版本（体现实验一与实验二的关联）

这是实验回顾中强调的**核心要求**：实验二的 Scanner **读取实验一格式的 DFA 配置文件**作为词法规则，运行时完全依据状态转移表进行最长匹配词法分析。

```bash
make scanner_dfa
./scanner_dfa dfa_lex.json test.src       # 文件模式
./scanner_dfa dfa_lex.json                # 交互模式（从 stdin 读源码）
```

**`dfa_lex.json`** 是一个扩展后的 DFA 配置文件（与实验一 JSON 格式兼容），覆盖了主文法的全部字符：
- 标识符、整数、浮点数（含科学计数法）
- 关系运算符（`<=`, `==`, `>=`, `!=`）
- 四则运算与界符

通过修改该 JSON 即可扩展或修改词法规则，无需重新编译 Scanner。

## 文件说明

| 文件 | 说明 |
|------|------|
| `scanner.cpp` | 词法分析器主程序（C++17，直接编码） |
| `scanner.c` | 词法分析器主程序（C99，直接编码） |
| `scanner_dfa.cpp` | **DFA 驱动版 Scanner**（读取 JSON 配置文件） |
| `dfa_lex.json` | 扩展后的 DFA 配置文件（实验一格式兼容） |
| `gen_dfa.py` | 生成 `dfa_lex.json` 的脚本（便于调整 DFA） |
| `Makefile` | 编译脚本（支持 scanner / scanner_c / scanner_dfa） |
| `test.src` | 示例源程序（函数声明片段） |
| `test_float.src` | 浮点数识别测试 |
| `test_op.src` | 运算符与边界测试 |

## Web 可视化（选做）

实验一已搭建的 `CompilationWeb/` 可视化平台已扩展支持实验二：

- 在线输入源代码，实时进行词法分析
- Token 流以表格形式展示（类型、值、位置），带颜色区分
- 支持上传 `.src`/`.c`/`.txt` 源文件
- 源码预览区域带语法高亮

打开方式：

```bash
cd CompilationWeb
# 直接用浏览器打开 index.html
# 或启动本地服务器
python3 -m http.server 8080
```

## 环境信息

- **OS**: openEuler 22.03 LTS (aarch64)
- **Compiler**: g++ 10.3.1 / gcc 10.3.1
- **构建工具**: GNU Make
- **Web**: 现代浏览器（Chrome / Firefox / Edge）
