# 实验五：SLR(1) 引导的语义分析框架

## 一、实验目标

在实验四（SLR(1) 语法分析）的基础上，给每个产生式**嵌入语义动作**，让编译器在"读"代码结构的同时，理解代码的"意思"：

- 变量有没有声明过？
- `5` 能不能赋给 `x`？（类型是否匹配）
- 把代码翻译成更简单的中间表示（三地址码）

## 二、实验五的输入与输出

### 输入（3 个）

| 输入项 | 来源 | 本实验中的文件 |
|--------|------|---------------|
| **文法规则** | 实验说明给出 | `grammar.txt` |
| **SLR(1) 分析表** | 实验四输出 | `slr_table.txt`（由 `exp4/slr1 --save-table` 导出） |
| **Token 流** | 实验二输出 | `test1.txt` ~ `test4.txt`（实验二 `scanner_dfa` 的输出格式） |

#### Token 文件格式

实验二 `scanner_dfa` 的标准输出格式为带括号的二元组：

```
(TYPE, value)
```

例如：
```
(INT, int)
(ID, x)
(SEMI, ;)
(NUM, 5)
(ADD, +)
```

程序在末尾自动追加 `$` 结束符，无需手动添加。

### 输出（4 个）

| 输出项 | 说明 |
|--------|------|
| **AST（抽象语法树）** | 代码的树状结构，如 `Program → DeclList → [VarDecl, Assign, Print]` |
| **符号表** | 记录所有变量的名字和类型，如 `x : int` |
| **语义错误报告** | 重复声明、未声明变量、类型不匹配等 |
| **中间代码（三地址码）** | 如 `t1 = b * 2`、`c = t2` |

## 三、与实验二、四的衔接流程

```bash
# 步骤 1：实验二生成 Token 流
cd exp2
./scanner_dfa dfa_lex.json source.src > tokens.txt

# 步骤 2：实验四导出 SLR 分析表
cd exp4
./slr1 grammar.txt --save-table slr_table.txt

# 步骤 3：实验五读取两者，执行语义分析
cd exp5
./semantic_analyzer grammar.txt --load-table slr_table.txt tokens.txt
```

## 四、文件说明

| 文件 | 说明 |
|------|------|
| `semantic_analyzer.cpp` | 核心代码（~600 行）：SLR 表生成/加载 + AST + 符号表 + 语义动作 + 驱动 |
| `grammar.txt` | 实验五文法（变量声明 / 赋值 / print / 表达式） |
| `slr_table.txt` | 实验四导出的 SLR(1) 分析表（ACTION/GOTO） |
| `test1.txt` ~ `test4.txt` | 4 组标准测试 Token 流 |
| `Makefile` | `make` 编译 / `make test` 一键测试 |

## 五、编译与运行

```bash
# 编译
make

# 运行（默认 grammar.txt + test1.txt）
./semantic_analyzer

# 指定文法 + Token 文件
./semantic_analyzer grammar.txt test2.txt

# 从实验四加载 SLR 表（推荐，真正衔接实验四）
./semantic_analyzer grammar.txt --load-table slr_table.txt test1.txt

# 一键运行全部测试
make test
```

## 六、文法规则

```
Prog     → DeclList
DeclList → DeclList Decl | Decl
Decl     → VarDecl | Assign | Print

VarDecl  → Type ID SEMI
Type     → INT | FLOAT

Assign   → ID ASG Expr SEMI
Print    → PRINT Expr SEMI

Expr     → Expr ADD Term | Term
Term     → Term MUL Fact | Fact
Fact     → ID | INT_NUM | FLOAT_NUM | LPAR Expr RPAR
```

**产生式共 20 条**（含自动添加的增广产生式 `S' → Prog`）。

## 七、测试用例

### Test 1：正确程序

**Token 流：**
```
(INT, int) (ID, x) (SEMI, ;) (ID, x) (ASG, =) (NUM, 5) (SEMI, ;)
(PRINT, print) (ID, x) (SEMI, ;)
```

**对应代码：** `int x; x = 5; print x;`

**预期输出：**
- ✅ 无语义错误
- 符号表：`x : int`
- 中间代码：
  ```
  x = 5
  print x
  ```

### Test 2：类型不匹配

**对应代码：** `int a; float b; a = 3.5;`

**预期输出：**
- ❌ `Type mismatch in assignment to 'a': expected int, got float`

### Test 3：未声明变量

**对应代码：** `y = 10;`

**预期输出：**
- ❌ `Variable 'y' not declared`

### Test 4：复杂表达式

**对应代码：** `int a; int b; int c; c = a + b * 2;`

**预期输出：**
- ✅ 无语义错误
- 中间代码：
  ```
  t1 = b * 2
  t2 = a + t1
  c = t2
  ```

## 八、技术要点

### 1. 属性栈（Attribute Stack）

标准 SLR 分析器有**状态栈**和**符号栈**。实验五增加**属性栈**，三者同步操作：

- **移进（Shift）**：终结符的 token 值、类型信息压入属性栈
- **规约（Reduce）**：弹出右部符号的属性值，执行语义动作，计算左部的新属性值，压栈

```cpp
struct SemanticValue {
    string token_val;   // 原始词法值，如 "x"
    string addr;        // 地址/临时变量名，如 "t1"
    ValueType vtype;    // 值类型：INT / FLOAT / ERROR
    ASTNode* ast;       // AST 子树指针
    string code;        // 生成的三地址码片段
};
```

### 2. 符号表（Symbol Table）

本实验使用单作用域符号表（教学简化版）：

```cpp
class SymbolTable {
    map<string, SymbolEntry> table;
public:
    bool insert(string name, string type);  // 插入变量，检测重复声明
    SymbolEntry* lookup(string name);       // 查找变量
};
```

### 3. 语义动作执行时机

在 SLR **规约（Reduce）** 时，根据**产生式编号**调用对应语义动作：

| 产生式 | 语义动作说明 |
|--------|-------------|
| `VarDecl → Type ID SEMI` | 将 `(ID, Type)` 插入符号表；构建 `VAR_DECL` AST 节点 |
| `Assign → ID ASG Expr SEMI` | 查符号表确认 ID 已声明；检查类型一致；生成赋值三地址码 |
| `Expr → Expr ADD Term` | 检查左右类型一致；新建临时变量 `tN`；生成 `tN = Expr.addr + Term.addr` |
| `Fact → ID` | 查符号表获取类型；若未声明则报错 |

### 4. 类型系统（简化）

- 支持类型：`int`、`float`
- 算术运算：要求操作数类型完全相同（暂不支持隐式转换）
- 赋值：要求左右类型完全相同

## 九、作者信息

- 实现语言：C++17
- 日期：2026/05/30
