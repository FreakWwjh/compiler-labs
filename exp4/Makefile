CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

TARGET = slr1

.PHONY: all clean test test-expr test-ext test-lr0

all: $(TARGET)

$(TARGET): slr1.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) *.o

test: test-expr test-ext test-lr0

test-expr: $(TARGET)
	@echo "=== 标准表达式文法（SLR(1) 经典示例） ==="
	./$(TARGET) grammar_expr.txt
	@echo ""

test-ext: $(TARGET)
	@echo "=== 扩展表达式文法（含一元负号） ==="
	./$(TARGET) grammar_ext.txt
	@echo ""

test-lr0: $(TARGET)
	@echo "=== 简单 LR(0) 文法（无冲突） ==="
	./$(TARGET) grammar_lr0.txt
