#!/usr/bin/env bash
# Stage 9: for loops, break/continue, compound assignment, the str() builtin,
# improved diagnostics, and common subexpression elimination.
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
        echo "$output" | tail -40
        FAILED=$((FAILED + 1))
    fi
}

# ----- for loops, break, continue -----
run_command_test "for loop accumulates" 0 $'PROGRAM OUTPUT\n--------------------------------------\n15' \
    "$COMPILER" run tests/stage9/for_sum.lw
run_command_test "continue still runs the for increment" 0 $'PROGRAM OUTPUT\n--------------------------------------\n5' \
    "$COMPILER" run tests/stage9/for_continue.lw
run_command_test "break exits the for loop" 0 $'PROGRAM OUTPUT\n--------------------------------------\n16' \
    "$COMPILER" run tests/stage9/for_break.lw
run_command_test "break and continue inside while" 0 $'PROGRAM OUTPUT\n--------------------------------------\n30' \
    "$COMPILER" run tests/stage9/while_break_continue.lw
run_command_test "nested loops with break and continue" 0 $'PROGRAM OUTPUT\n--------------------------------------\n4' \
    "$COMPILER" run tests/stage9/nested_loops.lw

# ----- compound assignment -----
run_command_test "compound arithmetic operators" 0 $'PROGRAM OUTPUT\n--------------------------------------\n4\nLightLang' \
    "$COMPILER" run tests/stage9/compound_ops.lw

# ----- str() builtin -----
run_command_test "str of every value type" 0 $'25\n3.5\ntrue\nplain\nn=7' \
    "$COMPILER" run tests/stage9/str_builtin.lw

# ----- optimizer: common subexpression elimination -----
run_command_test "CSE preserves semantics across reassignment" 0 $'PROGRAM OUTPUT\n--------------------------------------\n6036' \
    "$COMPILER" run tests/stage9/cse_reassign.lw
run_command_test "CSE removes a redundant computation" 0 "%t4 = %t2 + %t2" \
    "$COMPILER" tests/stage9/cse_redundant.lw
run_command_test "CSE keeps the result correct" 0 $'PROGRAM OUTPUT\n--------------------------------------\n42' \
    "$COMPILER" run tests/stage9/cse_redundant.lw

# A variable declared inside a loop body is re-declared on every iteration.
# This must reset the variable rather than raise a runtime error.
cat > "$TMP_DIR/loop_decl.lw" <<'SRC'
int i = 0;
while (i < 3) {
    int j = i * 2;
    print(j);
    i = i + 1;
}
SRC
run_command_test "declaration inside a loop body" 0 $'PROGRAM OUTPUT\n--------------------------------------\n0\n2\n4' \
    "$COMPILER" run "$TMP_DIR/loop_decl.lw"

# ----- semantic rejections -----
run_command_test "break outside a loop" 1 "A break statement is only valid inside a while or for loop." \
    "$COMPILER" tests/stage9/invalid_break_outside.lw
run_command_test "continue outside a loop" 1 "A continue statement is only valid inside a while or for loop." \
    "$COMPILER" tests/stage9/invalid_continue_outside.lw
run_command_test "for initializer variable is loop scoped" 1 "Variable 'i' has not been declared." \
    "$COMPILER" tests/stage9/invalid_for_scope.lw
run_command_test "for condition must be bool" 1 "The condition of a for statement must have type 'bool'" \
    "$COMPILER" tests/stage9/invalid_for_condition.lw
run_command_test "str cannot be redeclared" 1 "Function 'str' is a built-in conversion and cannot be redeclared." \
    "$COMPILER" tests/stage9/invalid_str_redeclare.lw
run_command_test "str takes exactly one argument" 1 "Built-in function 'str' expects 1 argument(s), but received 2." \
    "$COMPILER" tests/stage9/invalid_str_arity.lw

# str() is the supported conversion; implicit mixed concatenation stays an error.
run_command_test "implicit mixed concatenation still rejected" 1 "requires two numeric operands or two string operands" \
    "$COMPILER" tests/stage9/invalid_implicit_concat.lw

# ----- diagnostics -----
run_command_test "syntax errors report a column" 1 "column" \
    "$COMPILER" tests/stage9/invalid_multi_syntax.lw
run_command_test "syntax errors render a caret" 1 "^" \
    "$COMPILER" tests/stage9/invalid_multi_syntax.lw
run_command_test "lexical error reports column" 1 "Lexical error at line 2, column 11" \
    "$COMPILER" tests/stage9/invalid_lexical_caret.lw

# Parser recovery: a single run must report more than one syntax error.
set +e
recovery_output="$("$COMPILER" tests/stage9/invalid_multi_syntax.lw 2>&1)"
set -e
error_count=$(grep -c '^Syntax error at line' <<< "$recovery_output")
if [[ "$error_count" -ge 2 ]]; then
    echo "PASS: parser recovers and reports multiple syntax errors"
    PASSED=$((PASSED + 1))
else
    echo "FAIL: parser recovers and reports multiple syntax errors"
    echo "  Expected at least 2 syntax errors, found $error_count"
    FAILED=$((FAILED + 1))
fi

# ----- persistence of the new instruction -----
run_command_test "compile str bytecode" 0 "Bytecode saved successfully" \
    "$COMPILER" compile tests/stage9/str_builtin.lw -o "$TMP_DIR/str.lbc"
run_command_test "execute str bytecode" 0 $'25\n3.5\ntrue\nplain\nn=7' \
    "$COMPILER" exec "$TMP_DIR/str.lbc"
run_command_test "loop bytecode round-trip" 0 $'PROGRAM OUTPUT\n--------------------------------------\n15' \
    "$COMPILER" compile tests/stage9/for_sum.lw -o "$TMP_DIR/for.lbc"
run_command_test "execute loop bytecode" 0 $'PROGRAM OUTPUT\n--------------------------------------\n15' \
    "$COMPILER" exec "$TMP_DIR/for.lbc"

# A TO_STRING opcode must survive serialization and reload.
cat > "$TMP_DIR/tostring.lbc" <<'OLD'
LIGHTLANG_BYTECODE 2
COUNT 4
PUSH_INT 0 0 "" "42"
TO_STRING 0 0 "" ""
PRINT 0 0 "" ""
HALT 0 0 "" ""
OLD
run_command_test "TO_STRING loads from bytecode" 0 "42" \
    "$COMPILER" exec "$TMP_DIR/tostring.lbc"

echo
echo "Stage 9 tests passed: $PASSED"
echo "Stage 9 tests failed: $FAILED"

if [[ "$FAILED" -ne 0 ]]; then
    exit 1
fi
