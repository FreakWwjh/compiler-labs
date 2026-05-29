CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2

TARGET = semantic_analyzer
SRC = semantic_analyzer.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TARGET)
	@echo "===== Test 1: basic correct ====="
	./$(TARGET) grammar.txt test1.txt
	@echo ""
	@echo "===== Test 2: type mismatch ====="
	./$(TARGET) grammar.txt test2.txt
	@echo ""
	@echo "===== Test 3: expression + undeclared ====="
	./$(TARGET) grammar.txt test3.txt
	@echo ""
	@echo "===== Test 4: parenthesized expression ====="
	./$(TARGET) grammar.txt test4.txt

clean:
	rm -f $(TARGET)

.PHONY: all test clean
