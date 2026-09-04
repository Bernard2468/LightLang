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
        echo "  Expected text:   $expected_text"
        echo "  Output:"
        echo "$output" | tail -20
        FAILED=$((FAILED + 1))
    fi
}

# Stage 3 valid semantic cases.
run_test "test.lw" 0 "Semantic analysis completed successfully."
run_test "tests_stage3/valid_widening.lw" 0 "Semantic analysis completed successfully."
run_test "tests_stage3/valid_shadowing.lw" 0 "Semantic analysis completed successfully."
run_test "tests_stage3/valid_operators.lw" 0 "Semantic analysis completed successfully."
run_test "tests_stage3/valid_bool_conditions.lw" 0 "Semantic analysis completed successfully."

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
run_test "tests/valid_empty.lw" 0 "Front-end compilation completed successfully."
run_test "tests/valid_expressions.lw" 0 "Front-end compilation completed successfully."
run_test "tests/valid_nested.lw" 0 "Front-end compilation completed successfully."
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

echo
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [[ "$FAILED" -ne 0 ]]; then
    exit 1
fi
