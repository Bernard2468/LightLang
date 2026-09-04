# LightLang Functions

## Syntax

```text
func int add(int a, int b) {
    return a + b;
}
```

The keyword `func` is followed by the return type, function name, parameter list, and
body. Supported return and parameter types are `int`, `float`, `bool`, and `string`.

## Calls

A function call is an expression and can therefore appear in declarations, arithmetic,
comparisons, nested calls, return expressions, and `print` statements.

```text
int x = add(2, 3);
print(add(x, 10));
```

## Static checks

The semantic analyzer verifies:

- unique function names;
- unique parameter names within a function;
- existence of every called function;
- exact argument count;
- argument type compatibility;
- safe `int` to `float` widening;
- return-expression type compatibility;
- a return value on every possible function path;
- `return` is not used outside a function;
- nested function declarations are rejected.

Function signatures are collected before bodies are analyzed. This enables forward
calls, direct recursion, and mutual recursion.

## Scope rule

Function parameters and local variables use lexical scope. For this lightweight
language, direct access to top-level variables from a function is intentionally
forbidden. This keeps initialization order deterministic and makes function interfaces
explicit through parameters and return values.

## Runtime model

Each call creates a VM call frame containing:

- parameter/local variable storage;
- compiler-temporary storage;
- evaluated arguments;
- the caller return address;
- the current argument-binding position.

This frame model makes recursion safe: each recursive invocation gets independent local
variables and temporaries even though all invocations execute the same bytecode.

The VM limits call depth to 1024 frames and reports a controlled runtime error if the
limit is exceeded.
