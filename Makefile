JoyAssembler: JoyAssembler.cpp Types.hh Util.cpp UTF8.cpp Parser.cpp Computation.cpp Log.cpp UnitTests.cpp
	make UnitTests
	
	./set-build-status.sh failing
	g++ -std=c++17 -Wall -Wpedantic -Wextra -Werror JoyAssembler.cpp -o JoyAssembler
	./set-build-status.sh passing

.PHONY: test
test:
	make UnitTests
	
	make
	./set-build-status.sh failing
	test/test.sh
	./set-build-status.sh passing

UnitTests: UnitTests.cpp
	g++ -std=c++17 -Wall -Wpedantic -Wextra -Werror UnitTests.cpp -o UnitTests
	./UnitTests
	#rm UnitTests
