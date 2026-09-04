LightLang Compiler - Stage 8
============================

LightLang is a pure C++17 educational compiler for a statically typed lightweight
programming language. It implements the major compiler-construction phases, generates
its own stack-based bytecode, persists compiled programs as .lbc files, and executes
them on a C++ virtual machine.

Compiler pipeline
-----------------
1. Source-file loading
2. Lexical analysis
3. Recursive-descent syntax analysis
4. Abstract Syntax Tree (AST) construction
5. Semantic/type analysis
6. Scoped symbol-table and function-table construction
7. Three-address intermediate-code generation
8. Conservative intermediate-code optimization
9. Stack-based target-bytecode generation
10. Persistent .lbc bytecode serialization
11. Bytecode loading and structural validation
12. Execution on the LightLang C++ virtual machine

Supported source-language features
----------------------------------
- int, float, bool, string
- variable declarations and assignment
- +, -, *, /, %
- ==, !=, <, <=, >, >=
- &&, ||, !
- if / else
- while
- print(...)
- // comments
- nested lexical scopes
- int-to-float widening
- common string escapes: \\, \", \n, \t, \r
- typed user-defined functions
- typed parameters
- function calls inside expressions
- return statements
- forward function calls
- recursion and mutual recursion
- compile-time argument-count and argument-type checking
- compile-time return-type and return-path checking
- runtime call-depth protection

Function syntax
---------------
Example:

    func int add(int a, int b) {
        return a + b;
    }

    int result = add(10, 20);
    print(result);

Functions are top-level declarations. Every function has one of the four LightLang
return types and must return a value on every possible execution path. Direct global
variable access from inside functions is intentionally disallowed; parameters and
return values are used for function data exchange.

Build
-----
make

Equivalent direct build:

g++ -std=c++17 -Wall -Wextra -Werror -pedantic main.cpp Token.cpp Lexer.cpp AST.cpp Parser.cpp Types.cpp SymbolTable.cpp SemanticAnalyzer.cpp IntermediateCode.cpp IRGenerator.cpp Optimizer.cpp Bytecode.cpp BytecodeFile.cpp BytecodeGenerator.cpp RuntimeValue.cpp VirtualMachine.cpp -o lightlang

Command-line interface
----------------------
Show help:

    ./lightlang --help

Verbose compile:

    ./lightlang examples/hello.lw

Compile and immediately execute source:

    ./lightlang run examples/functions.lw

Compatible form:

    ./lightlang --run examples/functions.lw

Compile and save persistent bytecode:

    ./lightlang compile examples/functions.lw

Compile and choose the output filename:

    ./lightlang compile examples/functions.lw -o program.lbc

Inspect a compiled bytecode file:

    ./lightlang inspect program.lbc

Execute compiled bytecode without the original source:

    ./lightlang exec program.lbc

Persistent bytecode
-------------------
Stage 8 writes LightLang bytecode format version 2. Version 2 adds function-call,
parameter-binding, and return instructions. The loader remains backward-compatible
with Stage 7 version-1 bytecode files.

Exit status
-----------
0 = compilation, inspection, or execution succeeded
1 = source/compiler/bytecode-file error
2 = runtime error while executing valid bytecode

Tests
-----
make test

The Stage 8 package contains 115 verified tests: 87 regression tests from the earlier
compiler stages and 28 new function/toolchain tests. Coverage includes lexical, syntax,
semantic, IR, optimization, bytecode, persistence, recursion, call frames, return
checking, runtime traps, and malformed-bytecode validation.

Useful examples
---------------
examples/hello.lw             Basic declarations, strings, arithmetic, and print
examples/control_flow.lw      while loop plus if/else
examples/types_and_scope.lw   Types, widening, shadowing, and string operations
examples/functions.lw         Typed functions, nested calls, and recursion
examples/semantic_error.lw    Intentional semantic error for demonstration

Important files
---------------
Token.*                 Token representation
Lexer.*                 Lexical analyzer
AST.*                   Abstract Syntax Tree
Parser.*                Recursive-descent parser
Types.*                 Language type definitions
SymbolTable.*           Scoped variable symbols
SemanticAnalyzer.*      Semantic/type and function checking
IntermediateCode.*      Three-address IR
IRGenerator.*           AST-to-IR lowering
Optimizer.*             Conservative IR optimization
Bytecode.*              Target-bytecode representation
BytecodeGenerator.*     Optimized-IR-to-bytecode lowering
BytecodeFile.*          Persistent .lbc serialization, loading, and validation
RuntimeValue.*          Typed runtime values
VirtualMachine.*        Stack-based bytecode VM with call frames
GRAMMAR.txt              Complete LightLang grammar
FUNCTIONS.md             Function design and constraints
INTERMEDIATE_CODE.md     IR documentation
OPTIMIZATION.md          Optimization documentation
BYTECODE_VM.md           Target-code and VM documentation
BYTECODE_FILE_FORMAT.md  Persistent .lbc specification
DEMO_GUIDE.md            Project-presentation workflow
PROJECT_TECHNICAL_REPORT.md  Concise implementation report

Coding style
------------
The C++ source uses `using namespace std;` consistently. Standard-library names are
therefore written as cout, string, vector, unordered_map, and similar names without the
std:: prefix.
