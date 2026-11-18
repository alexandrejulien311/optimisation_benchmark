#!/usr/bin/env bash
# Runs the benchmark and compares its table with the expected output in README.md.
#
# Usage: check_output.sh <path-to-benchmark-executable> [--exact]
#   default : compares every column except "f - f*". Those tiny values may differ
#             in the last digits between compilers and standard libraries.
#   --exact : compares the whole output, character for character. Use it only with
#             the reference toolchain (MSYS2 CLANG64, clang + libc++).
set -euo pipefail

exe="$1"
mode="${2:-}"

"$exe" | tr -d '\r' > actual_full.txt
sed -n '/^Function/,/^Report/p' README.md | tr -d '\r' > expected_full.txt

if [ "$mode" = "--exact" ]; then
    cp actual_full.txt actual.txt
    cp expected_full.txt expected.txt
else
    # Drop the last column (f - f*) of the result rows, which end in e+NN / e-NN.
    strip='/e[+-][0-9]+$/ s/[[:space:]]+[^[:space:]]+$//'
    sed -E "$strip" actual_full.txt > actual.txt
    sed -E "$strip" expected_full.txt > expected.txt
fi

if diff -u expected.txt actual.txt; then
    echo "Output matches README.md${mode:+ ($mode)}."
else
    echo "Output differs from README.md (see diff above). Full output:"
    cat actual_full.txt
    exit 1
fi
