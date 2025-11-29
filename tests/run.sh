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

test_negative() {
    cp invoke_forall.h negative/

    for file in negative/*.cpp; do
        echo -n "${file%.cpp}: "
        
        _=$(clang++ -Wall -Wextra -std=c++23 -O2 "$file" 2>&1)
        if [[ $? -ne 0 ]]; then
            echo_ok OK[CE]
        else
            echo_fail FAIL
        fi
    done

    rm negative/invoke_forall.h
}

test_positive() {
    cp invoke_forall.h positive/

    for file in positive/*.cpp; do
        echo -n "${file%.cpp}: "
        
        _=$(clang++ -Wall -Wextra -std=c++23 -O2 "$file" 2>&1)
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

    rm positive/invoke_forall.h
}

cp ../invoke_forall.h .

test_negative
test_positive

rm *.out invoke_forall.h
