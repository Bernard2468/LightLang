CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -pedantic
SOURCES = main.cpp Token.cpp Lexer.cpp AST.cpp Parser.cpp Types.cpp SymbolTable.cpp SemanticAnalyzer.cpp IntermediateCode.cpp IRGenerator.cpp Optimizer.cpp Bytecode.cpp BytecodeFile.cpp BytecodeGenerator.cpp RuntimeValue.cpp VirtualMachine.cpp
TARGET = lightlang

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

test: $(TARGET)
	bash tests_stage7/run_tests.sh
	bash tests_stage8/run_tests.sh

demo: $(TARGET)
	./$(TARGET) run examples/functions.lw

clean:
	rm -f $(TARGET) lightlang_sanitize *.o *.lbc examples/*.lbc tests_stage8/*.lbc
