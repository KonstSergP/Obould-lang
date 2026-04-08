#!/usr/bin/env bash


COMPILER=${1:-}
TEST_FILE=${2:-}

if [[ -z "$COMPILER" || -z "$TEST_FILE" ]]; then
    echo "Usage: $0 <compiler> <test_file>"
    exit 2
fi

if [[ ! -x "$COMPILER" ]]; then
    echo "FAIL: Compiler not found or not executable: $COMPILER"
    exit 2
fi

if [[ ! -f "$TEST_FILE" ]]; then
    echo "FAIL: Test file not found: $TEST_FILE"
    exit 2
fi

TEST_NAME=$(basename "$TEST_FILE" .obl)
TMP_DIR="/tmp/obould_tests"
mkdir -p "$TMP_DIR"
OUT_EXE="$TMP_DIR/$TEST_NAME"


EXPECT_COMPILE_ERROR=0
EXPECT_RUNTIME_ERROR=0

if grep -q "^// @EXPECT_COMPILE_ERROR" "$TEST_FILE"; then
    EXPECT_COMPILE_ERROR=1
fi
if grep -q "^// @EXPECT_RUNTIME_ERROR" "$TEST_FILE"; then
    EXPECT_RUNTIME_ERROR=1
fi


echo "Testing $TEST_NAME ..."

COMPILE_OUT=$("$COMPILER" "$TEST_FILE" -m --link -o "$OUT_EXE" 2>&1)
COMPILE_STATUS=$?

if [[ $EXPECT_COMPILE_ERROR -eq 1 ]]; then
    if [[ $COMPILE_STATUS -eq 0 ]]; then
        echo "FAIL: Expected compilation to fail, but it succeeded!"
        exit 1
    fi
    echo "SUCCESS: Failed to compile exactly as expected."
    exit 0
else
    if [[ $COMPILE_STATUS -ne 0 ]]; then
        echo "FAIL: Compilation failed unexpectedly!"
        echo "$COMPILE_OUT"
        exit 1
    fi
fi


RUNTIME_OUT=$("$OUT_EXE" 2>&1)
RUNTIME_STATUS=$?

if [[ $EXPECT_RUNTIME_ERROR -eq 1 ]]; then
    if [[ $RUNTIME_STATUS -eq 0 ]]; then
        echo "FAIL: Expected runtime crash, but program finished successfully!"
        exit 1
    fi
    echo "SUCCESS: Program crashed at runtime as expected."
    exit 0
else
    if [[ $RUNTIME_STATUS -ne 0 ]]; then
        echo "FAIL: Program crashed unexpectedly at runtime!"
        echo "$RUNTIME_OUT"
        exit 1
    fi
fi


echo "SUCCESS: $TEST_NAME ran successfully."
exit 0
