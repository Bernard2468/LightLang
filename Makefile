# LightLang compiler build
#
# Layout:
#   src/frontend  lexical + syntax analysis   (Token, Lexer, AST, Parser)
#   src/semantic  semantic analysis           (SymbolTable, SemanticAnalyzer)
#   src/ir        intermediate code + opt     (IntermediateCode, IRGenerator, Optimizer)
#   src/backend   target code generation      (Bytecode, BytecodeFile, BytecodeGenerator)
#   src/runtime   execution                   (RuntimeValue, VirtualMachine)
#   src/common    shared vocabulary types     (Types)

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -pedantic -Isrc

# g++ appends .exe on Windows; without this make would relink on every run.
ifeq ($(OS),Windows_NT)
    EXE := .exe
else
    EXE :=
endif

TARGET  := lightlang$(EXE)
BUILDIR := build

SOURCES := $(wildcard src/*.cpp) $(wildcard src/*/*.cpp)
OBJECTS := $(patsubst src/%.cpp,$(BUILDIR)/%.o,$(SOURCES))
DEPS    := $(OBJECTS:.o=.d)

STAGES := 3 4 5 6 7 8 9

PYTHON ?= python

.PHONY: all test demo ui clean $(addprefix test-stage,$(STAGES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@

# -MMD -MP emits a .d file per object so that editing a header rebuilds
# every translation unit that includes it.
$(BUILDIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

# Run the full suite. Each stage script takes the compiler path as $1 and
# resolves its .lw fixtures relative to the repository root.
test: $(TARGET)
	@bash tests/run_all.sh ./$(TARGET)

$(addprefix test-stage,$(STAGES)): test-stage%: $(TARGET)
	@bash tests/stage$*/run_tests.sh ./$(TARGET)

demo: $(TARGET)
	./$(TARGET) run examples/functions.lw

# Local web playground: write LightLang source in the browser and see every
# compiler phase. Serves on 127.0.0.1 only and drives this same executable.
ui: $(TARGET)
	$(PYTHON) tools/playground.py

# Note: examples/*.lbc are committed demonstration artifacts and are
# deliberately NOT removed here - deleting them would dirty the working tree.
clean:
	rm -rf $(BUILDIR)
	rm -f $(TARGET) lightlang lightlang.exe lightlang_sanitize
	rm -f *.lbc tests/stage8/*.lbc tests/stage9/*.lbc
	rm -rf .lltmp.*
