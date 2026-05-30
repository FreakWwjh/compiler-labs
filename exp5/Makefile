CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2

TARGET = semantic_analyzer
SRC = semantic_analyzer.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TARGET)
	@echo "===== Test 1: 正确程序 ====="
	@./$(TARGET) grammar.txt --load-table slr_table.txt test1.txt | grep -E 'AST|符号表|语义错误|分析结果|中间代码' -A 10
	@echo ""
	@echo "===== Test 2: 类型不匹配 ====="
	@./$(TARGET) grammar.txt --load-table slr_table.txt test2.txt | grep -E 'AST|符号表|语义错误|分析结果|中间代码' -A 10
	@echo ""
	@echo "===== Test 3: 未声明变量 ====="
	@./$(TARGET) grammar.txt --load-table slr_table.txt test3.txt | grep -E 'AST|符号表|语义错误|分析结果|中间代码' -A 10
	@echo ""
	@echo "===== Test 4: 复杂表达式 ====="
	@./$(TARGET) grammar.txt --load-table slr_table.txt test4.txt | grep -E 'AST|符号表|语义错误|分析结果|中间代码' -A 10

clean:
	rm -f $(TARGET)

.PHONY: all test clean
