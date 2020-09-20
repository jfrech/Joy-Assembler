@ECHO OFF

REM The operating system "Windows 10" as well as the tools used in this script
REM are not free software. Sell your soul at your own risk.
REM
REM To build Joy Assembler on Windows 10, install Visual Studio:
REM     https://visualstudio.microsoft.com/downloads/
REM In particular, the C++ compiler should be installed.
REM When the C++ compiler is installed, open the developer command prompt
REM located at
REM     Start Menu > Visual Studio 2019 > Developer Command Prompt for VS 2019,
REM navigate to the Joy Assembler source directory and build it:
REM     C:\...> Windows-Makefile
REM
REM For a more detailed walk-through I refer to Microsoft's documentation:
REM     https://docs.microsoft.com/en-us/cpp/build/
REM         walkthrough-compiling-a-native-cpp-program-on-the-command-line?
REM         view=vs-2019

REM build
cl /EHsc JoyAssembler.cpp /std:c++17 /DNO_ANSI_COLORS
del JoyAssembler.obj

REM test
JoyAssembler test\programs\test-000_multiply.asm
