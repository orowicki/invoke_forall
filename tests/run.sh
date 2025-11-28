#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

RED="\e[31m"
GREEN="\e[32m"
ENDCOLOR="\e[0m"

cp ../invoke_forall.h .

for file in *.cpp; do
    echo -n "${file%.cpp}: "
    
    _=$(clang++ -Wall -Wextra -std=c++23 -O2 "$file" 2>&1)

    if [[ $? -ne 0 ]]; then
        echo -e "${RED}FAIL[CE]${ENDCOLOR}"
        continue
    fi

    _=$(./a.out 2>&1)

    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL[RE]${ENDCOLOR}"
    else
        echo -e "${GREEN}OK${ENDCOLOR}"
    fi
done

rm *.out invoke_forall.h
