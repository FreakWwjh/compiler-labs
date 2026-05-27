CXX = g++
CC = gcc
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
CFLAGS = -std=c99 -O2 -Wall

TARGET_CPP = scanner
TARGET_C = scanner_c

.PHONY: all clean test test-c

all: $(TARGET_CPP) $(TARGET_C)

$(TARGET_CPP): scanner.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TARGET_C): scanner.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET_CPP) $(TARGET_C) *.o

test: $(TARGET_CPP)
	@echo "=== C++ Mode 1 Test ==="
	@echo -e '1\n5\nid if 485 841.6541 www' | ./$(TARGET_CPP)
	@echo ""
	@echo "=== C++ Mode 2 Test ==="
	@echo -e '2\nwhile(true) {int a=0;}' | ./$(TARGET_CPP)
	@echo ""
	@echo "=== C++ File Mode Test ==="
	./$(TARGET_CPP) test.src

test-c: $(TARGET_C)
	@echo "=== C Mode 1 Test ==="
	@echo -e '1\n5\nid if 485 841.6541 www' | ./$(TARGET_C)
	@echo ""
	@echo "=== C Mode 2 Test ==="
	@echo -e '2\nwhile(true) {int a=0;}' | ./$(TARGET_C)
	@echo ""
	@echo "=== C File Mode Test ==="
	./$(TARGET_C) test.src
