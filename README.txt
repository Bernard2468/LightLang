LightLang Compiler - Stage 9
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
- for loops with a required initializer, condition, and update
- break and continue
- compound assignment: +=, -=, *=, /=, %=
- str(...) built-in conversion from any type to string
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

Diagnostics
-----------
Lexical and syntax errors report a line and column and print the offending source
line with a caret under the exact token:

    Syntax error at line 3, column 8 near ';': Expected ')' after value in print statement.
        3 | print(a;
          |        ^

The parser recovers at statement boundaries, so one run reports every syntax error
in the file rather than stopping at the first.

String conversion
-----------------
LightLang does not silently coerce numbers to strings, because that makes
expressions like 1 + 2 + "x" ambiguous. Use str(...) instead:

    int age = 25;
    print("Age: " + str(age));      // Age: 25

    "Age: " + age                   // rejected at compile time

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
make            Build the compiler
make ui         Build, then open the browser playground
make test       Run every stage test suite
make demo       Compile and run the functions example
make clean      Remove build output

Equivalent direct build:

g++ -std=c++17 -Wall -Wextra -Werror -pedantic -Isrc src/main.cpp src/common/*.cpp src/frontend/*.cpp src/semantic/*.cpp src/ir/*.cpp src/backend/*.cpp src/runtime/*.cpp -o lightlang

The -Isrc flag lets each translation unit include its dependencies by phase-qualified
path, for example #include "frontend/Lexer.h".

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

Playground (web UI)
-------------------
Instead of reading pipeline output in a terminal, you can run the compiler from a
browser page that shows every phase side by side:

    make ui

or directly:

    python tools/playground.py [--port 8000] [--no-browser]

This starts a small local server and opens a page where you write LightLang source
on the left and inspect the result on the right, with one tab per compiler phase:
program output, tokens, AST, symbol table, function table, three-address IR,
optimized IR, and target bytecode. Press Run or Ctrl+Enter to compile. The example
programs are loadable from the dropdown.

The playground shells out to this same lightlang executable, so what it displays is
exactly what the command line produces. Build the compiler first.

Notes:
- Only the Python standard library is used; there is nothing to install.
- The server binds to 127.0.0.1 and is not reachable from the network.
- A program that never terminates is stopped after 10 seconds instead of hanging
  the page.

Persistent bytecode
-------------------
LightLang writes bytecode format version 2, which adds function-call,
parameter-binding, and return instructions. The loader remains backward-compatible
with version-1 bytecode files written by Stage 7.

Stage 9 added the TO_STRING instruction for the str() conversion. It is a new opcode
within the existing version 2 format, so previously written version-2 files still
load unchanged.

Exit status
-----------
0 = compilation, inspection, or execution succeeded
1 = source/compiler/bytecode-file error
2 = runtime error while executing valid bytecode

Tests
-----
make test                 Run every stage suite and print an aggregate summary
make test-stage9          Run one stage suite in isolation (3 through 9)

Equivalent direct invocation, from the repository root:

    bash tests/run_all.sh ./lightlang
    bash tests/stage9/run_tests.sh ./lightlang

The suite contains 343 verified assertions across stages 3 to 9. Coverage includes
lexical, syntax, semantic, IR, optimization, bytecode, persistence, recursion, call
frames, return checking, runtime traps, malformed-bytecode validation, loops,
break/continue, compound assignment, the str() builtin, diagnostic columns and
carets, parser error recovery, and common subexpression elimination.

Useful examples
---------------
examples/hello.lw             Basic declarations, strings, arithmetic, and print
examples/control_flow.lw      while loop plus if/else
examples/types_and_scope.lw   Types, widening, shadowing, and string operations
examples/functions.lw         Typed functions, nested calls, and recursion
examples/semantic_error.lw    Intentional semantic error for demonstration
examples/loops_and_strings.lw for loops, break/continue, compound assignment, str()
examples/first_program.lw     Scratch program

Repository layout
-----------------
The source tree mirrors the compiler pipeline: each directory is one phase, so a
file's location states when it runs.

src/main.cpp                 Command-line driver and phase sequencing

src/frontend/                Lexical and syntax analysis
    Token.*                  Token representation
    Lexer.*                  Lexical analyzer
    AST.*                    Abstract Syntax Tree
    Parser.*                 Recursive-descent parser

src/semantic/                Semantic analysis
    SymbolTable.*            Scoped variable symbols
    SemanticAnalyzer.*       Semantic/type and function checking

src/ir/                      Intermediate representation
    IntermediateCode.*       Three-address IR
    IRGenerator.*            AST-to-IR lowering
    Optimizer.*              Conservative IR optimization

src/backend/                 Target-code generation
    Bytecode.*               Target-bytecode representation
    BytecodeGenerator.*      Optimized-IR-to-bytecode lowering
    BytecodeFile.*           Persistent .lbc serialization, loading, validation

src/runtime/                 Execution
    RuntimeValue.*           Typed runtime values
    VirtualMachine.*         Stack-based bytecode VM with call frames

src/common/                  Shared across phases
    Types.*                  Language type definitions (ValueType)
    Diagnostic.*             Source excerpts with a caret for error reporting

docs/                        Project documentation
    GRAMMAR.txt              Complete LightLang grammar
    FUNCTIONS.md             Function design and constraints
    INTERMEDIATE_CODE.md     IR documentation
    OPTIMIZATION.md          Optimization documentation
    BYTECODE_VM.md           Target-code and VM documentation
    BYTECODE_FILE_FORMAT.md  Persistent .lbc specification
    DEMO_GUIDE.md            Project-presentation workflow
    PROJECT_TECHNICAL_REPORT.md  Concise implementation report
    RELEASE_NOTES_STAGE8.md  Stage 8 release notes

tests/                       Test suites, one directory per stage
    run_all.sh               Runs every stage suite and aggregates results
    stage2/ .. stage9/       Stage fixtures and run_tests.sh scripts
    test.lw, test_invalid.lw, test_semantic_invalid.lw
                             Shared fixtures used by several stage suites

tools/                       Developer tooling
    playground.py            Local web-UI server for the compiler
    playground.html          The playground page

examples/                    Demonstration programs
build/                       Object files and dependency files (generated)

Coding style
------------
The C++ source uses `using namespace std;` consistently. Standard-library names are
therefore written as cout, string, vector, unordered_map, and similar names without the
std:: prefix.
