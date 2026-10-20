#!/bin/sh
# Continuous integration (D-54): .github/workflows/checks.yml runs this as an
# unprivileged user in an Arch container with every tool installed. The GCC build's
# meson test must pass with no test skipped, so a missing tool cannot pass unnoticed;
# a clang build with the address and undefined-behaviour sanitizers then runs the same
# tests (its binary contract is skipped by design, D-47). Needs no display.
#
#   sh tests/ci.sh <scratch directory outside the repository>
set -eu
scratch=$1

meson setup "$scratch/gcc"
meson test -C "$scratch/gcc" --print-errorlogs
if grep -q '"result": "SKIP"' "$scratch/gcc/meson-logs/testlog.json"; then
    echo 'ci: a test was skipped; install its tool (AGENTS.md)' >&2
    exit 1
fi

CC=clang meson setup "$scratch/clang" -Db_sanitize=address,undefined -Db_lundef=false
meson test -C "$scratch/clang" --print-errorlogs
