CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2
TARGET = dfa_sim
SRC = dfa_simulator.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET) dfa_in1.dfa

run2: $(TARGET)
	./$(TARGET) dfa_in2_binary.dfa

.PHONY: all clean run run2
