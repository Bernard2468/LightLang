#!/usr/bin/env bash
# Runs every LightLang stage suite and reports an aggregate result.
#
# Usage:  bash tests/run_all.sh [path-to-compiler]
#
# The stage scripts resolve their .lw fixtures relative to the repository
# root, so this script cd's there first and accepts being invoked from
# anywhere.
set -u

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT" || exit 1

COMPILER="${1:-./lightlang}"

if [[ ! -x "$COMPILER" && ! -x "$COMPILER.exe" ]]; then
    echo "error: compiler not found or not executable: $COMPILER" >&2
    echo "hint: run 'make' first." >&2
    exit 1
fi

STAGES=(3 4 5 6 7 8 9)
TOTAL_PASS=0
TOTAL_FAIL=0
FAILED_STAGES=()

for stage in "${STAGES[@]}"; do
    script="tests/stage${stage}/run_tests.sh"
    [[ -f "$script" ]] || { echo "error: missing $script" >&2; exit 1; }

    echo "=============================================="
    echo " Stage $stage"
    echo "=============================================="

    output="$(bash "$script" "$COMPILER" 2>&1)"
    echo "$output"

    pass=$(grep -c '^PASS' <<< "$output")
    fail=$(grep -c '^FAIL' <<< "$output")
    TOTAL_PASS=$((TOTAL_PASS + pass))
    TOTAL_FAIL=$((TOTAL_FAIL + fail))
    (( fail > 0 )) && FAILED_STAGES+=("$stage")
    echo
done

echo "=============================================="
echo " SUMMARY"
echo "=============================================="
echo "  passed: $TOTAL_PASS"
echo "  failed: $TOTAL_FAIL"

if (( TOTAL_FAIL > 0 )); then
    echo "  failing stages: ${FAILED_STAGES[*]}"
    exit 1
fi

echo "  all suites green"
