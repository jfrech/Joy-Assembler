CPPC = g++ #clang++

CPPFLAGS = -std=c++17 -lstdc++fs -O3 \
    -Wall -Wpedantic -Wextra -Werror -Wswitch-enum

SOURCES = $(wildcard *.cpp *.hpp)

JoyAssembler: Makefile $(SOURCES)
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

UnitTests: $(SOURCES)
	$(CPPC) $(CPPFLAGS) UnitTests.cpp -o UnitTests
