CPPC = g++
#CPPC = clang++
CPPFLAGS = -std=c++17 -Wall -Wpedantic -Wextra -Werror -O3

JoyAssembler: JoyAssembler.cpp Types.hh Util.cpp UTF8.cpp Parser.cpp \
              Computation.cpp Log.cpp UnitTests.cpp RepresentationHandlers.cpp \
              Includes.hh
	make unit-tests
	
	./set-build-status.sh failing
	$(CPPC) $(CPPFLAGS) JoyAssembler.cpp -o JoyAssembler
	./set-build-status.sh passing


.PHONY: test
test:
	make UnitTests
	
	make
	./set-build-status.sh failing
	test/test.sh
	./set-build-status.sh passing


.PHONY: unit-tests
unit-tests:
	make UnitTests
	./UnitTests

UnitTests: UnitTests.cpp
	$(CPPC) $(CPPFLAGS) UnitTests.cpp -o UnitTests
