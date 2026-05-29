# 实验五：SLR(1) 引导的语义分析框架

## 实验目标

在实验四（SLR(1) 分析表生成）的基础上，集成**语义动作**，构建**抽象语法树（AST）**，维护**符号表**，并完成**类型检查**与初步语义验证。

## 核心内容

1. **文法扩展**：在产生式中嵌入语义动作
2. **属性计算**：表达式类型、临时变量地址传递
3. **符号表管理**：变量声明、作用域查询、重复声明检测
4. **错误检测**：未声明变量、类型不匹配
5. **中间代码生成**：三地址码（为实验六做准备）

## 文件说明

| 文件 | 说明 |
|------|------|
| `semantic_analyzer.cpp` | 核心实现：SLR表生成 + AST + 符号表 + 语义动作 + 驱动 |
| `grammar.txt` | 实验五文法（声明、赋值、print、表达式） |
| `test1.txt` ~ `test4.txt` | Token 流测试用例 |
| `Makefile` | 编译与一键测试 |

## 编译与运行

```bash
# 编译
make

# 运行（默认 grammar.txt + test1.txt）
./semantic_analyzer

# 指定文法和token文件
./semantic_analyzer grammar.txt test2.txt

# 一键运行全部测试
make test
```

## Token 文件格式

每行两个字段（空格分隔）：
```
种属 个体值
```

例如：
```
INT int
ID x
SEMI ;
INT_NUM 5
ADD +
```

文件末尾自动追加 `$` 结束符，无需手动添加。

## 输出说明

程序依次输出：

1. **SLR(1) 分析表构建信息**
2. **分析过程**：状态栈、 lookahead、ACTION（前30步）
3. **抽象语法树（AST）**：树形文本表示
4. **符号表**：变量名、类型、作用域
5. **语义错误报告**：重复声明、未声明、类型不匹配
6. **中间代码**：三地址码形式（如 `t1 = a + b`）

## 文法规则

```
Prog -> DeclList
DeclList -> DeclList Decl | Decl
Decl -> VarDecl | Assign | Print
VarDecl -> Type ID SEMI
Type -> INT | FLOAT
Assign -> ID ASG Expr SEMI
Print -> PRINT Expr SEMI
Expr -> Expr ADD Term | Term
Term -> Term MUL Fact | Fact
Fact -> ID | INT_NUM | FLOAT_NUM | LPAR Expr RPAR
```

## 测试用例

### Test 1：正确程序
输入：`int x; x = 5; print x;`
- 预期：无错误，AST 完整，符号表含 `x:int`
- 中间代码：`x = 5` / `print x`

### Test 2：类型不匹配
输入：`int a; float b; a = 3.5;`
- 预期：报错 `Type mismatch in assignment to 'a': expected int, got float`

### Test 3：表达式 + 未声明
输入：`int a; int b; int c; c = a + b * 2; d = 1;`
- 预期：检测到 `d` 未声明；生成 `t1 = b * 2` / `t2 = a + t1` / `c = t2`

### Test 4：括号表达式
输入：`int x; x = (1 + 2) * 3;`
- 预期：无错误，AST 正确反映括号优先级

## 技术要点

### 属性栈（Attribute Stack）
在标准 SLR 的状态栈、符号栈旁，维护**属性栈**，保存每个符号的语义值：
- 终结符移进时：记录 token 值、字面量类型
- 非终结符规约时：由 `executeAction` 计算并压入新属性值

### 符号表（Symbol Table）
- 采用**栈式作用域链**：支持嵌套作用域（当前测试用例为单作用域）
- `insert`：插入变量，检测重复声明
- `lookup`：从内层到外层查找变量

### 语义动作执行时机
在 SLR 规约（Reduce）时，根据**产生式编号**调用对应语义动作：
- 变量声明：插入符号表，构建 `VAR_DECL` AST 节点
- 赋值：查表、类型检查，生成三地址码
- 表达式：类型推导、新建临时变量、递归拼接代码

### 类型系统（简化）
- `int` 与 `float` 为基本类型
- 算术运算要求操作数类型一致（暂不支持隐式类型转换）
- 赋值要求左右类型一致

## 作者信息

- 实验五基于实验四（SLR(1) 分析表）扩展
- 实现语言：C++17
- 日期：2026/05/30
