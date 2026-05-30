CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2

TARGET = semantic_analyzer
SRC = semantic_analyzer.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TARGET)
	@echo "===== Test 1: basic correct (native format) ====="
	./$(TARGET) grammar.txt test1.txt
	@echo ""
	@echo "===== Test 2: type mismatch (native format) ====="
	./$(TARGET) grammar.txt test2.txt
	@echo ""
	@echo "===== Test 3: expression + undeclared (native format) ====="
	./$(TARGET) grammar.txt test3.txt
	@echo ""
	@echo "===== Test 4: parenthesized expression (native format) ====="
	./$(TARGET) grammar.txt test4.txt
	@echo ""
	@echo "===== Test 5: from exp2 scanner output + exp4 SLR table ====="
	./$(TARGET) grammar.txt --load-table slr_table.txt test_from_exp2.txt
	@echo ""
	@echo "===== Test 6: type mismatch from exp2 scanner output ====="
	./$(TARGET) grammar.txt test_from_exp2_typeerr.txt
	@echo ""
	@echo "===== Test 7: expression from exp2 scanner output + exp4 SLR table ====="
	./$(TARGET) grammar.txt --load-table slr_table.txt test_from_exp2_expr.txt

clean:
	rm -f $(TARGET)

.PHONY: all test clean
