joy-assembler: joy-assembler.cpp Types.hh Util.cpp UTF8.cpp Parse.cpp Computation.cpp Log.cpp
	./set-build-status.sh failing
	g++ -std=c++17 -Wall -Wpedantic -Wextra -Werror joy-assembler.cpp -o joy-assembler
	./set-build-status.sh passing

.PHONY: test
test:
	make
	./set-build-status.sh failing
	test/test.sh
	./set-build-status.sh passing
