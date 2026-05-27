CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

TARGET = scanner
SRCS = scanner.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) *.o

test: $(TARGET)
	@echo "=== Mode 1 Test ==="
	@echo -e '1\n5\nid if 485 841.6541 www' | ./$(TARGET)
	@echo ""
	@echo "=== Mode 2 Test ==="
	@echo -e '2\nwhile(true) {int a=0;}' | ./$(TARGET)
	@echo ""
	@echo "=== File Mode Test ==="
	./$(TARGET) test.src
