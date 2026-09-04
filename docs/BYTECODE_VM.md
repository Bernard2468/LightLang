# LightLang Stage 8: Target Bytecode and Virtual Machine

## Execution model

LightLang lowers optimized three-address IR to stack-based bytecode. Source can be
executed immediately or compiled into a persistent `.lbc` file for later VM execution.

## Bytecode categories

Storage and parameters:
- DECLARE <type> <name>
- BIND_PARAM <type> <name>
- LOAD <variable>
- LOAD_TEMP <temporary>
- STORE <variable>
- STORE_TEMP <temporary>

Constants:
- PUSH_INT <value>
- PUSH_FLOAT <value>
- PUSH_BOOL <value>
- PUSH_STRING <value>

Unary operations:
- POSITIVE
- NEGATE
- NOT

Binary operations:
- ADD
- SUBTRACT
- MULTIPLY
- DIVIDE
- MODULO
- EQUAL
- NOT_EQUAL
- LESS
- LESS_EQUAL
- GREATER
- GREATER_EQUAL
- AND
- OR

Control, functions, and output:
- PRINT
- JUMP <instruction-index>
- JUMP_IF_FALSE <instruction-index>
- CALL <function> <instruction-index> args=<count>
- RETURN <type>
- HALT

## Function call frames

A CALL removes its evaluated arguments from the operand stack, creates a new call frame,
and jumps to the function entry address. BIND_PARAM consumes arguments from that frame
and creates typed parameter variables. RETURN validates/converts the return value,
removes the function frame, restores the caller instruction address, and places the
returned value on the operand stack.

Variables and compiler temporaries are stored per call frame. This is essential for
recursion because separate invocations must not overwrite one another's local values.

## Runtime representation

Values are one of four LightLang types:
- 64-bit signed int
- double-precision float
- bool
- string

## Runtime safety

The VM reports controlled runtime errors for:
- integer overflow;
- division or modulo by zero;
- malformed/out-of-range constants;
- invalid variable or temporary access;
- stack underflow;
- incompatible runtime types;
- invalid jump or call targets;
- parameter/argument mismatches in malformed bytecode;
- a function falling through without RETURN;
- excessive recursion (maximum call depth 1024);
- excessive instruction execution as an infinite-loop guard.

Runtime errors preserve the original LightLang source line carried through AST, IR,
and bytecode generation.
