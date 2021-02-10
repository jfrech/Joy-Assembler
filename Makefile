CPPC=c++ -std=c++17 -lstdc++fs
# If `-lstdc++fs` cannot be linked, one may attempt not explicitly linking it:
#CPPC=c++ -std=c++17
CPPFLAGS=-O3 -Wall -Wpedantic -Wextra -Werror -Wswitch-enum

SOURCES=$(wildcard *.cpp *.hpp)

JoyAssembler: Makefile $(SOURCES)
	@./set-build-status.sh failing
	$(CPPC) $(CPPFLAGS) JoyAssembler.cpp -o JoyAssembler
	@./set-build-status.sh passing

.PHONY: install
install:
	make
	
	@printf '\n'
	
	@[ -e /usr/bin/joy-assembler ] \
	    && printf 'already installed\n' \
	    || exit 0
	
	@[ -e /usr/bin/joy-assembler ] \
	    && ! cmp JoyAssembler /usr/bin/joy-assembler >/dev/null \
	    && printf 'to update, use\n' \
	    && printf '    rm -f /usr/bin/joy-assembler' \
	    && printf ' && ln JoyAssembler /usr/bin/joy-assembler\n' \
	    || exit 0
	
	@[ ! -e /usr/bin/joy-assembler ] \
	    && printf 'to install, use\n' \
	    && printf '    ln JoyAssembler /usr/bin/joy-assembler\n' \
	    || exit 0


.PHONY: test
test:
	make UnitTests
	./UnitTests
	
	make
	@./set-build-status.sh failing
	test/test.sh
	@./set-build-status.sh passing

UnitTests: Makefile $(SOURCES)
	$(CPPC) $(CPPFLAGS) UnitTests.cpp -o UnitTests
