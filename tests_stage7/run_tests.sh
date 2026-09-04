#!/usr/bin/env bash
set -u

COMPILER="${COMPILER:-./lightlang}"
PASSED=0
FAILED=0

run_test() {
    local file="$1"
    local expected_status="$2"
    local expected_text="$3"
    local output
    local status

    output="$($COMPILER "$file" 2>&1)"
    status=$?

    if [[ "$status" -eq "$expected_status" ]] && grep -Fq "$expected_text" <<< "$output"; then
        echo "PASS: $file"
        PASSED=$((PASSED + 1))
    else
        echo "FAIL: $file"
        echo "  Expected status: $expected_status"
        echo "  Actual status:   $status"
        echo "  Expected text:"
        echo "$expected_text"
        echo "  Output:"
        echo "$output" | tail -40
        FAILED=$((FAILED + 1))
    fi
}

run_exec_test() {
    local file="$1"
    local expected_status="$2"
    local expected_text="$3"
    local output
    local status

    output="$($COMPILER --run "$file" 2>&1)"
    status=$?

    if [[ "$status" -eq "$expected_status" ]] && grep -Fq "$expected_text" <<< "$output"; then
        echo "PASS: --run $file"
        PASSED=$((PASSED + 1))
    else
        echo "FAIL: --run $file"
        echo "  Expected status: $expected_status"
        echo "  Actual status:   $status"
        echo "  Expected text:"
        echo "$expected_text"
        echo "  Output:"
        echo "$output" | tail -60
        FAILED=$((FAILED + 1))
    fi
}

# Stage 3 valid semantic cases.
run_test "test.lw" 0 "Intermediate-code generation completed successfully."
run_test "tests_stage3/valid_widening.lw" 0 "Intermediate-code generation completed successfully."
run_test "tests_stage3/valid_shadowing.lw" 0 "Intermediate-code generation completed successfully."
run_test "tests_stage3/valid_operators.lw" 0 "Intermediate-code generation completed successfully."
run_test "tests_stage3/valid_bool_conditions.lw" 0 "Intermediate-code generation completed successfully."

# Stage 3 invalid semantic cases.
run_test "tests_stage3/invalid_duplicate.lw" 1 "is already declared in this scope"
run_test "tests_stage3/invalid_undeclared_assignment.lw" 1 "has not been declared"
run_test "tests_stage3/invalid_undeclared_read.lw" 1 "has not been declared"
run_test "tests_stage3/invalid_initializer_type.lw" 1 "Cannot initialize variable 'x'"
run_test "tests_stage3/invalid_assignment_type.lw" 1 "Cannot assign a value of type 'float'"
run_test "tests_stage3/invalid_if_condition.lw" 1 "condition of an if statement must have type 'bool'"
run_test "tests_stage3/invalid_while_condition.lw" 1 "condition of a while statement must have type 'bool'"
run_test "tests_stage3/invalid_plus_mixed.lw" 1 "Operator '+' requires two numeric operands or two string operands"
run_test "tests_stage3/invalid_minus_string.lw" 1 "Operator '-' requires numeric operands"
run_test "tests_stage3/invalid_modulo_float.lw" 1 "Operator '%' requires int operands"
run_test "tests_stage3/invalid_logical_numeric.lw" 1 "Operator '&&' requires bool operands"
run_test "tests_stage3/invalid_not_int.lw" 1 "Operator '!' requires a bool operand"
run_test "tests_stage3/invalid_comparison_string.lw" 1 "Operator '<' requires numeric operands"
run_test "tests_stage3/invalid_equality_mismatch.lw" 1 "cannot compare values of type 'string' and 'int'"
run_test "tests_stage3/invalid_scope_escape.lw" 1 "Variable 'local' has not been declared"
run_test "tests_stage3/invalid_narrowing.lw" 1 "Cannot initialize variable 'i'"

# Regression tests from lexical and syntax stages.
run_test "tests/valid_empty.lw" 0 "Intermediate-code generation completed successfully."
run_test "tests/valid_expressions.lw" 0 "Intermediate-code generation completed successfully."
run_test "tests/valid_nested.lw" 0 "Intermediate-code generation completed successfully."
run_test "tests/invalid_bad_statement.lw" 1 "Compilation stopped: syntax error detected."
run_test "tests/invalid_empty_initializer.lw" 1 "Compilation stopped: syntax error detected."
run_test "tests/invalid_empty_print.lw" 1 "Compilation stopped: syntax error detected."
run_test "tests/invalid_missing_brace.lw" 1 "Compilation stopped: syntax error detected."
run_test "tests/invalid_missing_paren.lw" 1 "Compilation stopped: syntax error detected."
run_test "tests/invalid_no_identifier.lw" 1 "Compilation stopped: syntax error detected."
run_test "tests/lex_single_and.lw" 1 "Compilation stopped: lexical error detected."
run_test "tests/lex_single_or.lw" 1 "Compilation stopped: lexical error detected."
run_test "tests/lex_unknown_char.lw" 1 "Compilation stopped: lexical error detected."
run_test "tests/lex_unterminated_string.lw" 1 "Compilation stopped: lexical error detected."

# Stage 4 IR-specific cases.
run_test "tests_stage4/ir_precedence_collision.lw" 0 $'%t1 = 3 * 4\n%t2 = 2 + %t1\ndeclare int x\nx = %t2'
run_test "tests_stage4/ir_if_else.lw" 0 $'ifFalse flag goto L1\nprint "yes"\ngoto L2\nL1:\nprint "no"\nL2:'
run_test "tests_stage4/ir_while.lw" 0 $'L1:\n%t1 = x > 0\nifFalse %t1 goto L2\n%t2 = x - 1\nx = %t2\ngoto L1\nL2:'
run_test "tests_stage4/ir_shadow_initializer.lw" 0 $'%t1 = x + 1\ndeclare int x$1\nx$1 = %t1\nprint x$1\nprint x'
run_test "tests_stage4/ir_defaults.lw" 0 $'declare int i\ni = 0\ndeclare float f\nf = 0.0\ndeclare bool b\nb = false\ndeclare string s\ns = ""'
run_test "tests_stage4/ir_unary.lw" 0 $'%t1 = -x\ndeclare int y\ny = %t1'
run_test "tests_stage4/ir_if_no_else.lw" 0 $'ifFalse %t1 goto L1\nprint x\nL1:'
run_test "tests_stage4/ir_string_concat.lw" 0 $'%t1 = first + second\ndeclare string name\nname = %t1\nprint name'

# Stage 5 optimizer-specific cases.
run_test "tests_stage5/opt_constant_fold.lw" 0 $'OPTIMIZED THREE-ADDRESS CODE\n--------------------------------------\ndeclare int x\nx = 14\nprint 14'
run_test "tests_stage5/opt_float_widening.lw" 0 $'declare float f\nf = 2.0\ndeclare float y\ny = 0.5\nprint 0.5'
run_test "tests_stage5/opt_string_concat.lw" 0 $'declare string s\ns = "LightLang"\nprint "LightLang"'
run_test "tests_stage5/opt_true_if.lw" 0 $'declare int x\nx = 4\nprint "yes"\n--------------------------------------\nIntermediate-code optimization completed successfully.'
run_test "tests_stage5/opt_false_if.lw" 0 $'declare bool flag\nflag = false\nprint "done"\n--------------------------------------\nIntermediate-code optimization completed successfully.'
run_test "tests_stage5/opt_division_zero.lw" 0 $'%t1 = 10 / 0\ndeclare int x\nx = %t1\nprint x'
run_test "tests_stage5/opt_integer_division.lw" 0 $'declare int x\nx = 2\nprint 2'
run_test "tests_stage5/opt_while_conservative.lw" 0 $'L1:\n%t1 = x > 0\nifFalse %t1 goto L2'
run_test "tests_stage5/opt_algebraic.lw" 0 $'%t2 = x\ndeclare int y\ny = %t2'
run_test "tests_stage5/opt_bool.lw" 0 $'declare bool a\na = true\nprint true'
run_test "tests_stage5/opt_mixed_comparison.lw" 0 $'declare bool result\nresult = true\nprint true'
run_test "tests_stage5/opt_overflow_preserved.lw" 0 '%t1 = 9223372036854775807 + 1'
run_test "tests_stage5/opt_reassignment.lw" 0 $'x = 5\ndeclare int y\ny = 6\nprint 6'
run_test "tests_stage5/opt_trap_preserved.lw" 0 $'%t1 = 10 / 0\ndeclare int x\nx = 0\nprint 0'

# Stage 6 numeric-representation semantic guards.
run_test "tests_stage6/invalid_int_range.lw" 1 "Integer literal is outside the supported 64-bit signed range"
run_test "tests_stage6/invalid_float_range.lw" 1 "Float literal is outside the supported finite range"

# Stage 6 bytecode and VM cases.
run_test "tests_stage6/run_full.lw" 0 $'TARGET BYTECODE\n--------------------------------------\n0000  DECLARE int x'
run_test "tests_stage6/run_full.lw" 0 '0109  HALT'
run_test "tests_stage5/opt_division_zero.lw" 0 "Target-code generation completed successfully."
run_exec_test "tests_stage6/run_full.lw" 0 $'PROGRAM OUTPUT\n--------------------------------------\n6\n12.0\naaa\ntrue\n3\n2\n-3\ntrue\ndone'
run_exec_test "tests_stage4/ir_shadow_initializer.lw" 0 $'PROGRAM OUTPUT\n--------------------------------------\n11\n10'
run_exec_test "tests_stage4/ir_defaults.lw" 0 $'PROGRAM OUTPUT\n--------------------------------------\n0\n0.0\nfalse\n'
run_exec_test "tests_stage6/run_widening.lw" 0 $'PROGRAM OUTPUT\n--------------------------------------\n0.0'
run_exec_test "tests_stage6/run_false_branch.lw" 0 $'PROGRAM OUTPUT\n--------------------------------------\nno'
run_exec_test "tests_stage6/run_comparisons.lw" 0 $'PROGRAM OUTPUT\n--------------------------------------\ntrue\ntrue\ntrue'
run_exec_test "tests_stage5/opt_division_zero.lw" 2 "Runtime error at line 1: division by zero."
run_exec_test "tests_stage5/opt_overflow_preserved.lw" 2 "Runtime error at line 1: integer overflow during addition."

# Stage 7 persistent-bytecode and CLI cases.
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

run_command_test() {
    local name="$1"
    local expected_status="$2"
    local expected_text="$3"
    shift 3
    local output
    local status

    output="$("$@" 2>&1)"
    status=$?

    if [[ "$status" -eq "$expected_status" ]] && grep -Fq "$expected_text" <<< "$output"; then
        echo "PASS: $name"
        PASSED=$((PASSED + 1))
    else
        echo "FAIL: $name"
        echo "  Expected status: $expected_status"
        echo "  Actual status:   $status"
        echo "  Expected text:"
        echo "$expected_text"
        echo "  Output:"
        echo "$output" | tail -60
        FAILED=$((FAILED + 1))
    fi
}

# Default .lbc output path.
cp examples/hello.lw "$TMP_DIR/default_output.lw"
run_command_test "compile default .lbc" 0 "Bytecode saved successfully: $TMP_DIR/default_output.lbc" \
    "$COMPILER" compile "$TMP_DIR/default_output.lw"
if [[ -f "$TMP_DIR/default_output.lbc" ]] && grep -Fq "LIGHTLANG_BYTECODE 2" "$TMP_DIR/default_output.lbc"; then
    echo "PASS: persisted bytecode header"
    PASSED=$((PASSED + 1))
else
    echo "FAIL: persisted bytecode header"
    FAILED=$((FAILED + 1))
fi

# Custom output path, inspection, and execution without consulting source code.
run_command_test "compile custom .lbc" 0 "Bytecode saved successfully: $TMP_DIR/control.lbc" \
    "$COMPILER" compile examples/control_flow.lw -o "$TMP_DIR/control.lbc"
run_command_test "inspect .lbc" 0 "Bytecode file loaded successfully: $TMP_DIR/control.lbc" \
    "$COMPILER" inspect "$TMP_DIR/control.lbc"
run_command_test "inspect contains HALT" 0 "HALT" \
    "$COMPILER" inspect "$TMP_DIR/control.lbc"
run_command_test "execute .lbc" 0 $'PROGRAM OUTPUT\n--------------------------------------\nsum verified\n15' \
    "$COMPILER" exec "$TMP_DIR/control.lbc"

# Round-trip quoted/escaped strings through the on-disk representation.
run_command_test "compile string roundtrip" 0 "Bytecode saved successfully: $TMP_DIR/strings.lbc" \
    "$COMPILER" compile tests_stage7/roundtrip_strings.lw -o "$TMP_DIR/strings.lbc"
run_command_test "execute string roundtrip" 0 $'hello world\nline1\nline2\t"quoted"' \
    "$COMPILER" exec "$TMP_DIR/strings.lbc"

# Bytecode file validation failures.
cat > "$TMP_DIR/bad_signature.lbc" <<'BAD'
NOT_LIGHTLANG 1
COUNT 1
HALT 0 0 "" ""
BAD
run_command_test "reject bad signature" 1 "Bytecode file error: invalid file signature." \
    "$COMPILER" exec "$TMP_DIR/bad_signature.lbc"

cat > "$TMP_DIR/bad_version.lbc" <<'BAD'
LIGHTLANG_BYTECODE 99
COUNT 1
HALT 0 0 "" ""
BAD
run_command_test "reject unsupported version" 1 "Bytecode file error: unsupported bytecode version 99." \
    "$COMPILER" exec "$TMP_DIR/bad_version.lbc"

cat > "$TMP_DIR/bad_opcode.lbc" <<'BAD'
LIGHTLANG_BYTECODE 1
COUNT 1
EXPLODE 0 0 "" ""
BAD
run_command_test "reject unknown opcode" 1 "Bytecode file error: unknown opcode 'EXPLODE'." \
    "$COMPILER" exec "$TMP_DIR/bad_opcode.lbc"

cat > "$TMP_DIR/bad_jump.lbc" <<'BAD'
LIGHTLANG_BYTECODE 1
COUNT 2
JUMP 99 1 "" ""
HALT 0 0 "" ""
BAD
run_command_test "reject invalid jump" 1 "has an out-of-range jump target" \
    "$COMPILER" exec "$TMP_DIR/bad_jump.lbc"

cat > "$TMP_DIR/no_halt.lbc" <<'BAD'
LIGHTLANG_BYTECODE 1
COUNT 1
PUSH_INT 0 1 "" "5"
BAD
run_command_test "reject missing HALT" 1 "bytecode program must end with HALT" \
    "$COMPILER" exec "$TMP_DIR/no_halt.lbc"

cat > "$TMP_DIR/trailing_data.lbc" <<'BAD'
LIGHTLANG_BYTECODE 1
COUNT 1
HALT 0 0 "" ""
EXTRA
BAD
run_command_test "reject trailing bytecode data" 1 "unexpected content after the declared instructions" \
    "$COMPILER" inspect "$TMP_DIR/trailing_data.lbc"

run_command_test "missing bytecode file" 1 "Bytecode file error: could not open" \
    "$COMPILER" exec "$TMP_DIR/does_not_exist.lbc"
run_command_test "help command" 0 "lightlang exec <program.lbc>" \
    "$COMPILER" --help

cat > "$TMP_DIR/header_trailing.lbc" <<'BAD'
LIGHTLANG_BYTECODE 1 EXTRA
COUNT 1
HALT 0 0 "" ""
BAD
run_command_test "reject header trailing data" 1 "unexpected data in file header" \
    "$COMPILER" inspect "$TMP_DIR/header_trailing.lbc"

cat > "$TMP_DIR/count_trailing.lbc" <<'BAD'
LIGHTLANG_BYTECODE 1
COUNT 1 EXTRA
HALT 0 0 "" ""
BAD
run_command_test "reject count trailing data" 1 "unexpected data after instruction count" \
    "$COMPILER" inspect "$TMP_DIR/count_trailing.lbc"

echo
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [[ "$FAILED" -ne 0 ]]; then
    exit 1
fi
