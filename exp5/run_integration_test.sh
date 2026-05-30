#!/bin/bash

EXP2_DIR="../exp2"
EXP4_DIR="../exp4"
GRAMMAR="grammar.txt"
TMPDIR="/tmp/exp5_integration_test_$$"
mkdir -p "$TMPDIR"

echo "========================================"
echo "实验五集成自动化测试"
echo "流程: exp2 scanner → exp4 SLR表 → exp5 语义分析"
echo "========================================"
echo ""

# 1. 用实验四导出 SLR 表
echo "[1/6] 从实验四导出 SLR 分析表..."
$EXP4_DIR/slr1 $GRAMMAR --save-table "$TMPDIR/slr_table.txt" > /dev/null
echo "      ✓ SLR表已导出"

# 2. 生成测试源文件
cat > "$TMPDIR/test1.src" << 'EOF'
int x;
x = 5;
print x;
EOF

cat > "$TMPDIR/test2.src" << 'EOF'
int a;
float b;
a = 3.5;
EOF

cat > "$TMPDIR/test3.src" << 'EOF'
int a;
int b;
int c;
c = a + b * 2;
EOF

cat > "$TMPDIR/test4.src" << 'EOF'
int x;
x = (1 + 2) * 3;
EOF

cat > "$TMPDIR/test5.src" << 'EOF'
int x;
y = 10;
EOF

# 3. 定义预期结果
declare -a EXPECT_ERRORS
declare -a EXPECT_CODE
declare -a EXPECT_SYMTAB

EXPECT_ERRORS[1]="未检测到语义错误"
EXPECT_CODE[1]="x = 5"
EXPECT_SYMTAB[1]="x : int"

EXPECT_ERRORS[2]="Type mismatch in assignment to 'a'"
EXPECT_CODE[2]="a = 3.5"
EXPECT_SYMTAB[2]="a : int"

EXPECT_ERRORS[3]="未检测到语义错误"
EXPECT_CODE[3]="t1 = b * 2"
EXPECT_SYMTAB[3]="c : int"

EXPECT_ERRORS[4]="未检测到语义错误"
EXPECT_CODE[4]="t1 = 1 + 2"
EXPECT_SYMTAB[4]="x : int"

EXPECT_ERRORS[5]="Variable 'y' not declared"
EXPECT_CODE[5]=""
EXPECT_SYMTAB[5]="x : int"

PASS=0
FAIL=0

for i in 1 2 3 4 5; do
    echo ""
    echo "[TEST $i] $(head -1 "$TMPDIR/test$i.src")..."
    
    # 用实验二生成 token 文件
    $EXP2_DIR/scanner_dfa $EXP2_DIR/dfa_lex.json "$TMPDIR/test$i.src" > "$TMPDIR/test$i.tokens"
    
    # 用实验五执行语义分析
    ./semantic_analyzer $GRAMMAR --load-table "$TMPDIR/slr_table.txt" "$TMPDIR/test$i.tokens" > "$TMPDIR/test$i.out" 2> "$TMPDIR/test$i.err" || true
    
    # 合并 stdout + stderr 用于断言
    cat "$TMPDIR/test$i.out" "$TMPDIR/test$i.err" > "$TMPDIR/test$i.all"
    
    OK=1
    
    # 断言1: 语义错误报告
    if grep -qF "${EXPECT_ERRORS[$i]}" "$TMPDIR/test$i.all"; then
        echo "      ✓ 语义错误报告正确"
    else
        echo "      ❌ 断言失败: 未找到 '${EXPECT_ERRORS[$i]}'"
        OK=0
    fi
    
    # 断言2: 分析完成
    if grep -qF "语法分析与语义分析完成" "$TMPDIR/test$i.all"; then
        echo "      ✓ 分析完成"
    else
        echo "      ❌ 断言失败: 分析未完成"
        OK=0
    fi
    
    # 断言3: 中间代码片段
    if [ -n "${EXPECT_CODE[$i]}" ]; then
        if grep -qF "${EXPECT_CODE[$i]}" "$TMPDIR/test$i.all"; then
            echo "      ✓ 中间代码包含 '${EXPECT_CODE[$i]}'"
        else
            echo "      ❌ 断言失败: 未找到中间代码 '${EXPECT_CODE[$i]}'"
            OK=0
        fi
    fi
    
    # 断言4: 符号表
    if grep -qF "${EXPECT_SYMTAB[$i]}" "$TMPDIR/test$i.all"; then
        echo "      ✓ 符号表包含 '${EXPECT_SYMTAB[$i]}'"
    else
        echo "      ❌ 断言失败: 符号表中未找到 '${EXPECT_SYMTAB[$i]}'"
        OK=0
    fi
    
    if [ $OK -eq 1 ]; then
        PASS=$((PASS + 1))
        echo "      ✅ TEST $i 通过"
    else
        FAIL=$((FAIL + 1))
        echo "      ❌ TEST $i 失败"
        echo "      --- 实际输出 ---"
        cat "$TMPDIR/test$i.all" | head -20
        echo "      ----------------"
    fi
done

echo ""
echo "========================================"
echo "测试结果: $PASS 通过, $FAIL 失败"
echo "========================================"

# 清理
rm -rf "$TMPDIR"

[ $FAIL -eq 0 ]
