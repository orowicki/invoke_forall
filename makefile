.PHONY: all clean

all: invoke_forall.h
    clang++ -Wall -Wextra -std=c++23 -O2 *.cpp

clean:
	rm *.out
