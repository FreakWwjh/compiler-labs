# Flex / Bison 自动工具体验（实验四选做）

## 概述

本目录提供一个完整可运行的 **Flex + Bison** 示例——简单的算术表达式计算器。

通过对比手工编写的 SLR(1) 分析器与自动化工具，体会编译器生成器的工作机制。

## 文件说明

| 文件 | 说明 |
|------|------|
| `calc.l` | Flex 词法规则文件：定义正则表达式与返回的 Token 类型 |
| `calc.y` | Bison 语法规则文件：定义上下文无关文法与语义动作 |
| `Makefile` | 编译规则：调用 bison / flex / gcc 生成可执行文件 |

## 编译运行

```bash
make         # 生成可执行文件 calc
make test    # 自动运行三组测试用例
make clean   # 清理生成文件
```

## 交互式运行

```bash
./calc
# 输入表达式后按回车，例如：
# 1+2*3
# = 7
# (4+5)*6
# = 54
# Ctrl+D 退出
```

## 技术要点

### Flex (calc.l)

- 使用正则表达式匹配数字 `[0-9]+`、运算符 `+ - * /`、括号 `(` `)`
- 返回的 Token 定义在 `calc.tab.h`（由 Bison 生成）
- 空白字符被忽略，换行符作为表达式结束标记

### Bison (calc.y)

- 文法支持 `expr → expr + term | expr - term | term` 等表达式规则
- 使用 `%left` 声明运算符优先级与结合性，自动解决移进-归约冲突
- `%union` 与 `%token <num>` 实现语义值传递（整型求值）
- 在归约时执行语义动作（`$$ = $1 + $3` 等）

### 与手工 SLR(1) 的对比

| 特性 | 手工 SLR(1) | Flex/Bison |
|------|------------|------------|
| 词法分析 | 手写 DFA / 状态机 | Flex 正则自动生成 DFA |
| 语法分析 | 手写 Closure/Goto/填表 | Bison 自动计算 LR(1)/LALR 分析表 |
| 冲突解决 | 手动用 FOLLOW 集判断 | 声明优先级/结合性自动解决 |
| 语义动作 | 自行设计属性栈 | Bison 自动维护值栈 (`$$`, `$1`...) |
| 可维护性 | 修改文法需重写大量代码 | 修改 `.y`/`.l` 后重新生成即可 |

## 参考

- [Flex Manual](https://westes.github.io/flex/manual/)
- [Bison Manual](https://www.gnu.org/software/bison/manual/)
