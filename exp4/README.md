# 实验四：SLR(1) 分析表生成

## 一、实验目标

在实验三（LR(0) 项目集规范族）的基础上，引入 **FOLLOW 集** 解决部分移进-归约 / 归约-归约冲突，生成完整的 **SLR(1) 分析表**（ACTION/GOTO 二维表）。

## 二、文件说明

| 文件 | 说明 |
|------|------|
| `slr1.cpp` | 核心实现：文法解析 → LR(0)规范族 → FIRST/FOLLOW → SLR(1)分析表 |
| `Makefile` | 编译规则与测试目标 |
| `grammar_expr.txt` | 标准表达式文法（经典 SLR(1) 示例）|
| `grammar_ext.txt` | 扩展表达式文法（含一元负号 `-F`）|
| `grammar_lr0.txt` | 简单 LR(0) 文法（无冲突）|
| `optional_flex_bison/` | **选做（必做）**：Flex/Bison 自动工具体验 |

## 三、编译与运行

```bash
# 编译
make

# 运行（默认使用 grammar_expr.txt）
./slr1

# 指定文法文件
./slr1 grammar_ext.txt
./slr1 grammar_lr0.txt

# 一键测试所有文法
make test
```

## 四、输出说明

程序依次输出：

1. **增广文法**：自动添加 `S' → S` 后的产生式列表（含编号）
2. **LR(0) 项目集规范族**：每个状态的完整项目集 + Kernel 项目
3. **状态转移图**：状态间按符号的转移关系
4. **FIRST 集 / FOLLOW 集**：各非终结符的计算结果
5. **SLR(1) 分析表**：ACTION 表（终结符 + `$`）与 GOTO 表（非终结符）合并展示
6. **冲突检测报告**：判断该文法是否为 SLR(1) 文法

### 表头图例

- `sX` — 移进（Shift）至状态 X
- `rX` — 按第 X 条产生式归约（Reduce）
- `acc` — 接受（Accept）
- 空白 — 报错（Error）

## 五、算法要点

### 5.1 FIRST 集计算
- 终结符的 FIRST 为自身
- 非终结符 A：遍历以 A 为左部的产生式，按符号串规则迭代至不动点
- 支持 ε 产生式（用 `ε` 表示）

### 5.2 FOLLOW 集计算
- `FOLLOW(S') = {$}`（S' 为增广开始符号）
- 对产生式 `A → αBβ`：`FIRST(β)\{ε} ⊆ FOLLOW(B)`
- 若 `β ⇒* ε` 或 β 为空：`FOLLOW(A) ⊆ FOLLOW(B)`
- 迭代至不动点

### 5.3 SLR(1) 填表规则

| 项目类型 | 操作 |
|---------|------|
| `A → α·aβ`（a 为终结符），goto(Iᵢ, a) = Iⱼ | ACTION[i, a] = `sj` |
| `A → α·`（A ≠ S'），产生式编号 j | 对所有 `x ∈ FOLLOW(A)`：ACTION[i, x] = `rj` |
| `S' → S·` | ACTION[i, `$`] = `acc` |
| goto(Iᵢ, A) = Iⱼ（A 为非终结符）| GOTO[i, A] = j |

### 5.4 冲突检测
- 若同一格子被填入不同操作 → 报告 **移进-归约** 或 **归约-归约** 冲突
- 无冲突则判定为 **SLR(1) 文法**

## 六、测试验证

### 6.1 标准表达式文法（grammar_expr.txt）
```
E -> E + T | T
T -> T * F | F
F -> ( E ) | id
```

期望：12 个状态，无冲突，经典的 SLR(1) 分析表。

### 6.2 扩展表达式文法（grammar_ext.txt）
```
E -> E + T | T
T -> T * F | F
F -> ( E ) | - F | id
```

期望：14 个状态，无冲突，负号作为一元运算符可被 SLR(1) 正确处理。

### 6.3 简单 LR(0) 文法（grammar_lr0.txt）
```
S -> A A
A -> a A | b
```

期望：7 个状态，无冲突，同时也是 LR(0) 文法。

## 七、选做：Flex / Bison 体验

进入 `optional_flex_bison/` 目录，体验自动化词法/语法分析工具：

```bash
cd optional_flex_bison
make        # 编译
make test   # 运行测试用例
```

该示例实现了一个简单的 **表达式计算器**：
- **Flex (`calc.l`)**：将输入字符流识别为 NUMBER、PLUS、MINUS、MUL、DIV、LPAREN、RPAREN 等 Token
- **Bison (`calc.y`)**：定义表达式文法，在归约时实时计算结果

对比手工实现的 SLR(1) 分析器与自动化工具，可直观感受：
- Flex 自动生成 DFA 进行词法分析，无需手写状态机
- Bison 自动处理移进-归约、归约-归约冲突（默认策略 + 优先级声明），自动生成分析表

## 八、作者信息

- 实验四基于实验三（LR(0) 规范族）扩展
- 实现语言：C++17
- 日期：2026/05/30
