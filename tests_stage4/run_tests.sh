#!/usr/bin/env bash
set -u

COMPILER="./lightlang"
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

echo
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [[ "$FAILED" -ne 0 ]]; then
    exit 1
fi
