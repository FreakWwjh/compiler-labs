# 实验二：词法分析器 Scanner 实现

## 实验目标

实现一个基于 C++ 的词法分析扫描程序（Scanner），支持对自定义程序语言的源代码进行线性扫描，识别并输出 Token 流。

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
make          # 编译生成 scanner
make test     # 运行内置测试用例
make clean    # 清理编译产物
```

## 文件说明

| 文件 | 说明 |
|------|------|
| `scanner.cpp` | 词法分析器主程序（C++17） |
| `Makefile` | 编译脚本 |
| `test.src` | 示例源程序（函数声明片段） |
| `test_float.src` | 浮点数识别测试 |
| `test_op.src` | 运算符与边界测试 |

## 环境信息

- **OS**: openEuler 22.03 LTS (aarch64)
- **Compiler**: g++ 10.3.1 (支持 C++17)
- **构建工具**: GNU Make
