joy-assembler: joy-assembler.cpp
	g++ -std=c++17 -Wall -Wpedantic -Wextra -Werror joy-assembler.cpp -o joy-assembler

.PHONY: test
test:
	make
	test/test.sh
