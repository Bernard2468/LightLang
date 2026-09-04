# LightLang Project Demonstration Guide

## 1. Build

```bash
make
```

## 2. Show ordinary compilation phases

```bash
./lightlang examples/control_flow.lw
```

Point out tokens, AST, symbol table, function table, IR, optimized IR, and bytecode.

## 3. Demonstrate functions and recursion

```bash
./lightlang run examples/functions.lw
```

Explain that the parser recognizes typed function declarations/calls, semantic analysis
checks signatures and return paths, IR generates ARG/CALL/RETURN operations, and the VM
uses independent call frames for recursive invocations.

## 4. Compile once, execute later

```bash
./lightlang compile examples/functions.lw -o functions.lbc
./lightlang inspect functions.lbc
./lightlang exec functions.lbc
```

The final command uses only the bytecode file.

## 5. Demonstrate compile-time errors

```bash
./lightlang tests/stage8/invalid_arg_type.lw
./lightlang tests/stage8/invalid_missing_return.lw
```

## 6. Demonstrate runtime protection

```bash
./lightlang run tests/stage8/runtime_call_depth.lw
```

The VM stops uncontrolled recursion with a deterministic runtime error.

## 7. Run verification

```bash
make test
```

The package verifies 87 earlier regression tests plus 28 Stage 8 function/toolchain
tests, for 115 passing checks in total.
