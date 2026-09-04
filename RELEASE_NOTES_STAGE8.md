# Stage 8 Release Notes

Stage 8 completes the function-capable LightLang language/toolchain.

Added:
- `func` and `return` keywords;
- typed function declarations;
- typed parameters and function-call expressions;
- forward calls, recursion, and mutual recursion;
- compile-time function signature checking;
- guaranteed-return-path analysis;
- function-aware IR (`FUNCTION_BEGIN`, `PARAM`, `ARG`, `CALL`, `RETURN`);
- VM call frames with per-invocation variables and temporaries;
- recursion-depth protection;
- persistent bytecode version 2 with `BIND_PARAM`, `CALL`, and `RETURN`;
- backward loading support for bytecode version 1;
- function examples and defense/demo documentation;
- 28 new automated function/toolchain tests.

Verification status:
- 87/87 previous regression tests pass;
- 28/28 Stage 8 tests pass;
- 115/115 total checks pass;
- the same suites pass under AddressSanitizer and UndefinedBehaviorSanitizer;
- strict `-Wall -Wextra -Werror -pedantic` compilation passes.
