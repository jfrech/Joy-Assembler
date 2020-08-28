joy-assembler: joy-assembler.cpp types.hh Util.cpp UTF8.cpp
	g++ -std=c++17 -Wall -Wpedantic -Wextra -Werror joy-assembler.cpp -o joy-assembler

.PHONY: test
test:
	make
	test/test.sh
