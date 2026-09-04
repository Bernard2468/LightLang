#!/usr/bin/env bash
set -u

COMPILER="${1:-./lightlang}"
TMP_DIR="$(mktemp -d ./.lltmp.XXXXXX)"
trap 'rm -rf "$TMP_DIR"' EXIT
PASSED=0
FAILED=0

run_command_test() {
    local name="$1"
    local expected_status="$2"
    local expected_text="$3"
    shift 3

    set +e
    local output
    output="$($@ 2>&1)"
    local status=$?
    set -e

    if [[ "$status" -eq "$expected_status" ]] && grep -Fq "$expected_text" <<< "$output"; then
        echo "PASS: $name"
        PASSED=$((PASSED + 1))
    else
        echo "FAIL: $name"
        echo "  Expected status: $expected_status"
        echo "  Actual status:   $status"
        echo "  Expected text: $expected_text"
        echo "$output" | tail -80
        FAILED=$((FAILED + 1))
    fi
}

# New function-language behavior.
run_command_test "basic function" 0 $'PROGRAM OUTPUT\n--------------------------------------\n12' \
    "$COMPILER" run tests/stage8/basic_functions.lw
run_command_test "nested calls" 0 $'PROGRAM OUTPUT\n--------------------------------------\n14' \
    "$COMPILER" run tests/stage8/nested_calls.lw
run_command_test "recursive factorial" 0 $'PROGRAM OUTPUT\n--------------------------------------\n720' \
    "$COMPILER" run tests/stage8/recursion.lw
run_command_test "typed functions" 0 $'5.0\nHello, LightLang\ntrue' \
    "$COMPILER" run tests/stage8/typed_functions.lw
run_command_test "forward function call" 0 $'PROGRAM OUTPUT\n--------------------------------------\n21' \
    "$COMPILER" run tests/stage8/forward_call.lw
run_command_test "mutual recursion" 0 $'PROGRAM OUTPUT\n--------------------------------------\ntrue\ntrue' \
    "$COMPILER" run tests/stage8/mutual_recursion.lw
run_command_test "int to float parameter widening" 0 $'PROGRAM OUTPUT\n--------------------------------------\n4.5' \
    "$COMPILER" run tests/stage8/widen_parameter.lw
run_command_test "function local scopes" 0 $'PROGRAM OUTPUT\n--------------------------------------\n10\n6' \
    "$COMPILER" run tests/stage8/local_scope.lw
run_command_test "function table output" 0 "FUNCTION TABLE" \
    "$COMPILER" tests/stage8/basic_functions.lw
run_command_test "function IR call" 0 "%t1 = call add, 2" \
    "$COMPILER" tests/stage8/basic_functions.lw
run_command_test "function bytecode call" 0 "CALL add" \
    "$COMPILER" tests/stage8/basic_functions.lw

# Persistence of function-capable bytecode v2.
run_command_test "compile recursive bytecode" 0 "Bytecode saved successfully: $TMP_DIR/recursion.lbc" \
    "$COMPILER" compile tests/stage8/recursion.lw -o "$TMP_DIR/recursion.lbc"
if [[ -f "$TMP_DIR/recursion.lbc" ]] && grep -Fq "LIGHTLANG_BYTECODE 2" "$TMP_DIR/recursion.lbc"; then
    echo "PASS: bytecode v2 header"
    PASSED=$((PASSED + 1))
else
    echo "FAIL: bytecode v2 header"
    FAILED=$((FAILED + 1))
fi
run_command_test "execute recursive bytecode" 0 $'PROGRAM OUTPUT\n--------------------------------------\n720' \
    "$COMPILER" exec "$TMP_DIR/recursion.lbc"

# Backward read compatibility with the Stage 7 bytecode v1 representation.
cat > "$TMP_DIR/legacy_v1.lbc" <<'OLD'
LIGHTLANG_BYTECODE 1
COUNT 3
PUSH_STRING 0 1 "" "\"legacy-ok\""
PRINT 0 1 "" ""
HALT 0 0 "" ""
OLD
run_command_test "legacy bytecode v1 read" 0 "legacy-ok" \
    "$COMPILER" exec "$TMP_DIR/legacy_v1.lbc"

# Function semantic/syntax errors.
run_command_test "duplicate function rejection" 1 "Function 'f' is already declared." \
    "$COMPILER" tests/stage8/invalid_duplicate_function.lw
run_command_test "unknown function rejection" 1 "Function 'missing' has not been declared." \
    "$COMPILER" tests/stage8/invalid_unknown_function.lw
run_command_test "argument count rejection" 1 "expects 2 argument(s), but received 1" \
    "$COMPILER" tests/stage8/invalid_arg_count.lw
run_command_test "argument type rejection" 1 "expects type 'int', but received 'string'" \
    "$COMPILER" tests/stage8/invalid_arg_type.lw
run_command_test "return outside function rejection" 1 "return statement is only valid inside a function" \
    "$COMPILER" tests/stage8/invalid_return_outside.lw
run_command_test "return type rejection" 1 "this return expression has type 'string'" \
    "$COMPILER" tests/stage8/invalid_return_type.lw
run_command_test "missing return path rejection" 1 "must return a value of type 'int' on every execution path" \
    "$COMPILER" tests/stage8/invalid_missing_return.lw
run_command_test "duplicate parameter rejection" 1 "Parameter 'x' is duplicated" \
    "$COMPILER" tests/stage8/invalid_duplicate_parameter.lw
run_command_test "nested function rejection" 1 "Function declarations are only allowed at top level." \
    "$COMPILER" tests/stage8/invalid_nested_function.lw
run_command_test "global access restriction" 1 "cannot read global variable 'global'" \
    "$COMPILER" tests/stage8/invalid_global_access.lw

# Runtime call-stack protection.
run_command_test "call depth protection" 2 "maximum function call depth exceeded" \
    "$COMPILER" run tests/stage8/runtime_call_depth.lw

# Persisted-bytecode validation for the new instructions.
cat > "$TMP_DIR/bad_call_count.lbc" <<'BAD'
LIGHTLANG_BYTECODE 2
COUNT 2
CALL 1 1 "not-a-count" "f"
HALT 0 0 "" ""
BAD
run_command_test "reject malformed CALL count" 1 "has an invalid argument count" \
    "$COMPILER" inspect "$TMP_DIR/bad_call_count.lbc"

cat > "$TMP_DIR/bad_param_type.lbc" <<'BAD'
LIGHTLANG_BYTECODE 2
COUNT 2
BIND_PARAM 0 1 "decimal" "x"
HALT 0 0 "" ""
BAD
run_command_test "reject malformed parameter type" 1 "has unknown type 'decimal'" \
    "$COMPILER" inspect "$TMP_DIR/bad_param_type.lbc"

echo
echo "Stage 8 new tests passed: $PASSED"
echo "Stage 8 new tests failed: $FAILED"

if [[ "$FAILED" -ne 0 ]]; then
    exit 1
fi
