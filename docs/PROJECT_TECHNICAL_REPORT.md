# Design and Implementation of a Compiler for a Lightweight Programming Language

## Implementation Summary

LightLang is a statically typed educational language and compiler implemented entirely
in C++17. The compiler was built as a sequence of independently testable phases:
lexical analysis, recursive-descent parsing, AST construction, semantic analysis,
symbol management, three-address-code generation, optimization, target-bytecode
generation, persistent bytecode serialization, and execution on a custom virtual
machine.

The language supports four primitive types (`int`, `float`, `bool`, and `string`),
variables, expressions, assignment, conditional statements, loops, printing, lexical
scopes, typed functions, parameters, return values, forward calls, and recursion.

## Compiler Front End

The lexer scans source text character by character and produces tokens while recording
source-line metadata. The parser is hand written using recursive descent and encodes
operator precedence directly in its grammar functions. Valid syntax is represented as
an Abstract Syntax Tree.

Semantic analysis checks declarations, lexical scope, assignment compatibility,
operators, numeric ranges, function signatures, argument counts/types, return types,
and guaranteed return paths. Safe implicit widening from `int` to `float` is supported.

## Intermediate Representation and Optimization

The AST is lowered into three-address code containing explicit temporaries, labels,
branches, function parameters, argument passing, calls, and returns. The optimizer uses
conservative transformations including constant folding/propagation, algebraic and
Boolean simplification, constant branch elimination, and dead temporary cleanup.
Potentially trapping operations are retained rather than incorrectly folded.

## Target Code and Virtual Machine

Optimized IR is translated into a stack-based bytecode instruction set. The VM stores
typed runtime values and performs checked arithmetic, typed variable storage, branching,
printing, and function execution. Each function invocation creates an independent call
frame containing local variables, compiler temporaries, arguments, and a return address.
This design supports recursion without local-state collisions.

Compiled target code can be saved in the versioned `.lbc` format and executed later
without the source file. The Stage 8 loader reads both the current version-2 format and
legacy version-1 bytecode.

## Reliability and Verification

The compiler is built with C++17 using `-Wall -Wextra -Werror -pedantic`. The completed
Stage 8 test set contains 115 passing checks: 87 regression tests from earlier stages
plus 28 new tests for functions, recursion, persistence, call frames, semantic errors,
and malformed target code. AddressSanitizer and UndefinedBehaviorSanitizer are also
used for final runtime verification.
