joy-assembler: joy-assembler.cpp Types.hh Util.cpp UTF8.cpp Parse.cpp Computation.cpp Log.cpp UnitTests.cpp
	make unit-tests
	
	./set-build-status.sh failing
	g++ -std=c++17 -Wall -Wpedantic -Wextra -Werror joy-assembler.cpp -o joy-assembler
	./set-build-status.sh passing

.PHONY: test
test:
	make unit-tests
	
	make
	./set-build-status.sh failing
	test/test.sh
	./set-build-status.sh passing

.PHONY: unit-tests
unit-tests: UnitTests.cpp
	g++ -std=c++17 -Wall -Wpedantic -Wextra -Werror UnitTests.cpp -o UnitTests
	./UnitTests
	rm UnitTests
