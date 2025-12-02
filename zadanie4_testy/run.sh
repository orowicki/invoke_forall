#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

RED="\e[31m"
GREEN="\e[32m"
ENDCOLOR="\e[0m"

echo_ok() {
    echo -e "${GREEN}$1${ENDCOLOR}"
}

echo_fail() {
    echo -e "${RED}$1${ENDCOLOR}"
}

test() {
    for test in 101 102 103 103 105 106 107 201 202 203 204 301 402 403 404 501 502 601; do
        echo -n "test ${test}: "
        
        _=$(clang++ -Wall -Wextra -std=c++23 -O2 -DTEST_NUM=$test invoke_forall_test.cpp 2>&1)
        if [[ $? -ne 0 ]]; then
            echo_fail FAIL[CE]
            continue
        fi

        _=$(./a.out 2>&1)
        if [ $? -ne 0 ]; then
            echo_fail FAIL[RE]
        else
            echo_ok OK
        fi
    done
}

cp ../invoke_forall.h .

test

echo "Should fail:"
echo "* 101-107"
echo "* 202-204"
echo "* 502"

rm *.out invoke_forall.h
