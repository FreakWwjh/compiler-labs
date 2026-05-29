.PHONY: all clean test

TARGET = calc

all: $(TARGET)

calc.tab.c calc.tab.h: calc.y
	bison -d calc.y

lex.yy.c: calc.l calc.tab.h
	flex calc.l

$(TARGET): lex.yy.c calc.tab.c
	gcc -o $@ $^

clean:
	rm -f $(TARGET) calc.tab.c calc.tab.h lex.yy.c

test: $(TARGET)
	@echo "Testing 1+2*3 ..."
	@echo "1+2*3" | ./$(TARGET)
	@echo ""
	@echo "Testing (4+5)*6 ..."
	@echo "(4+5)*6" | ./$(TARGET)
	@echo ""
	@echo "Testing 10/2-3 ..."
	@echo "10/2-3" | ./$(TARGET)
