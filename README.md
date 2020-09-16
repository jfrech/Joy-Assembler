# Joy Assembler
A minimalistic toy assembler written in C++ by Jonathan Frech, August and September 2020.

# Building
Joy Assembler requires the `C++17` standard and is best build using the provided `Makefile`.

**Build: 🟩 passing** (2020-09-16T23:36:03+02:00)

# Usage
Joy Assembler provides a basic command-line interface:
````
./JoyAssembler <input-file.asm> [visualize | step | memory-dump]
````
The optional argument `visualize` allows one to see each instruction's execution, `step` allows to see and step thru (by hitting `enter`) execution. Note that the instruction pointed to is the instruction that _will be executed_ in the next step, not the instruction that has been executed. `memory-dump` mocks any I/O and outputs a step-by-step memory dump to `stdout` whilst executing.

# Architecture
Joy Assembler mimics a 32-bit architecture. It has four 32-bit registers: two general-prupose registers `A` (**a**ccumulation) and `B` (o**b**erand) and two special-prupose registers `PC` (**p**rogram **c**ounter) and `SC` (**s**tack **c**ounter).

Along with the above 32-bit registers, whose values will be called *word* henceforth, a single block of 8-bit *bytes* of memory of a fixed size is provided, addressable as either numeric 4-byte words or individual bytes. Each Joy Assembler instruction consists of a _name_ (represented as a singular op-code byte) paired with possibly four bytes representing an _argument_. All instructions' behavior is specified below.

The simulated architecture can also be slightly modified using _pragmas_ (see below).

# Joy Assembler files
Joy Assembler files usually end in a `.asm` file extension and contain all instructions, static data, and other file inclusions that will be executed by the simulated machine. Except for comments, each line contains either an instruction -- to the simulated machine or assembler -- or a piece of data or is left blank. What follows is the documentation of the previously mentioned features.

# Comments
A comment is defined as any characters from a semicolon (`;`) to the end of the line.

One quirk of implementation is that string or character literals cannot contain an unescaped semicolon, as this would be interpreted as a comment. As such, if a semicolon is needed, please resort to either `\u003b`, `\U0000003b` or `\;` (the last one being specially treated whilst removing comments).
````
; takes one stack argument and returns one stack argument
routine:
    ; load argument
    lsa -8
    ; ...
    ; store argument
    ssa -8
    ret
````

# Definitions
To define a constant value, one can use the _definition operator_ `:=`:
````
n := 5
x := 0b00100000
````

# Pragmas
Pragmas are used to alter the simulated machine's fundamental behavior or capability.
| pragma                         | options                                          | description                                              | default                           |
|--------------------------------|--------------------------------------------------|----------------------------------------------------------|-----------------------------------|
| `pragma_memory-size`           | `mininal`, `dynamic` or an unsigned 32-bit value | set the size in bytes of the available memory block      | `minimal`                         |
| `pragma_memory-mode`           | `little-endian` or `big-endian`                  | set the endianness for all 4-byte memory operations      | `little-endian`                   |
| `pragma_rng-seed`              | an unsigned 32-bit value                         | set the random number generator's seed                   | on default, a random seed is used |
| `pragma_static-program`        | `true` or `false`                                | set all instructions in memory to read-only              | `true`                            |
| `pragma_embed-profiler-output` | `false` or `true`                                | if enabled, the profiler will tacitly output to `stdout` | `false`                           |

# Labels
One can either hard-code program positions to jump to or use an abstraction; _program labels_. A label is defined by an identifier succeeded by a colon (`:`) and accessed by the same identifier preceded by an at sign (`@`). A label definition has to be unique, yet can be used arbitrarily often:
````
ack:
    ; ...
main:
    mov 2
    psh
    mov 3
    psh
    cal @ack
    pop
    ptu
    pop
    hlt
stack:
    data [0xfff]
````

# Data Blocks
A static _data block_ can be defined using `data ..., [optional-size]optional-literal-value, ...` or `data ..., "string-literal", ...`. It defines a statically initialized block of memory. Any numeric literal or string character occupies a word (four bytes), string characters being Unicode-capable:
````
primes:
    data 2, 3, 5, 7, 11, 13, 17 ; ... (finite memory)
global-x:
    data [1]0
global-y:
    data [1]0
buf:
    data [0xff]0xffffffff
msg:
    data "The end is nigh. \u2620\0"
msg2:
    data "I'll say \"goodbye", 0x22, ".", 0b00000000
stack:
    data [0xfff]
````

# Stack
To easily facilitate recursion, Joy Assembler provides native support for a *stack*, that is, a special-purpose region of memory which is usually accessed from the *stack's top* and can grow and shrink. When a stack is desired, its static size has to be specified:
````
stack:
    data [0xfff]
````
A stack is required when using any stack instruction. Stack underflow, overflow and misalignment are strictly enforced. `SC` is initialized with `@stack` when present.

# Instructions
This is the full list of Joy Assembler instructions. Each instruction will be statically loaded into memory using exactly five bytes: a single byte for the instruction's op-code and four further bytes for its arguments, if the instruction allows one, else four zero bytes. When an optional argument is not specified, its default value is used. When an argument is required and not specified or specified and not allowed, an error is reported.

An argument can be specified as either a numeric constant (`0xdeadbeef`, `55`, `0b10001`), a character (`'🐬'`, `'\U0001D6C7'`), a label (`@main`, `@routine`, `@stack`) or a defined constant.

| instruction name  | argument               | mnemonic                            | description                                                                                                                                                         |
|-------------------|------------------------|-------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **nop**           |                        |                                     |                                                                                                                                                                     |
| `nop`             | optional, default: `0` | "**n**o o**p**eration"              | No operation. Shows up in memory view, displaying the given value.                                                                                                  |
| **memory**        |                        |                                     |                                                                                                                                                                     |
| `lda`             | required               | "**l**oa**d** **a**"                | Load the value at the specified memory location into register `A`.                                                                                                  |
| `ldb`             | required               | "**l**oa**d** **b**"                | Load the value at the specified memory location into register `B`.                                                                                                  |
| `sta`             | required               | "**st**ore **a**"                   | Store the value in register `A` to the specified memory location.                                                                                                   |
| `stb`             | required               | "**st**ore **b**"                   | Store the value in register `B` to the specified memory location.                                                                                                   |
| `lia`             | optional, default: `0` | "**l**oad **i**ndirect **a**"       | Load the value at the memory location identified by register `B`, possibly offset by the specified number of bytes, into register `A`.                              |
| `sia`             | optional, default: `0` | "**s**tore **i**ndirect **a**"      | Store the value in register `A` at the memory location identified by register `B`, possibly offset by the specified number of bytes.                                |
| `lpc`             | none                   | "**l**oad **p**rogram **c**ounter"  | Load the value of the program counter register `PC` into the value in register `A`.                                                                                 |
| `spc`             | none                   | "**s**tore **p**rogram **c**ounter" | Store the value in register `A` to the program counter register `PC`.                                                                                               |
| `lya`             | required               | "**l**oad b**y**te **a**"           | Load the byte at the specified memory location into the least significant byte of register `A`, modifying it in-place and keeping the three most significant bytes. |
| `sya`             | required               | "**s**tore b**y**te **a**"          | Store the least significant byte of register `A` to the specified memory location.                                                                                  |
| **jumps**         |                        |                                     |                                                                                                                                                                     |
| `jmp`             | required               | "**j**u**mp**"                      | Jump to the specified program position, regardless of the machine's state.                                                                                          |
| `jn`              | required               | "**j**ump **n**egative"             | Jump to the specified program position if register `A` holds a negative value (`A < 0`), otherwise perform no operation.                                            |
| `jnn`             | required               | "**j**ump **n**on-**n**egative"     | Jump to the specified program position if register `A` holds a non-negative value (`A >= 0`), otherwise perform no operation.                                       |
| `jz`              | required               | "**j**ump **z**ero"                 | Jump to the specified program position if register `A` holds a zero value (`A == 0`), otherwise perform no operation.                                               |
| `jnz`             | required               | "**j**ump **n**on-**z**ero"         | Jump to the specified program position if register `A` holds a non-zero value (`A != 0`), otherwise perform no operation.                                           |
| `jp`              | required               | "**j**ump **p**ositive"             | Jump to the specified program position if register `A` holds a positive value (`A > 0`), otherwise perform no operation.                                            |
| `jnp`             | required               | "**j**ump **n**on-**p**ositive"     | Jump to the specified program position if register `A` holds a non-positive value (`A <= 0`), otherwise perform no operation.                                       |
| `je`              | required               | "**j**ump **e**ven"                 | Jump to the specified program position if register `A` holds an even value (`A & 1 == 0`), otherwise perform no operation.                                          |
| `jne`             | required               | "**j**ump **n**on-**e**ven"         | Jump to the specified program position if register `A` holds an odd value (`A & 1 == 1`), otherwise perform no operation.                                           |
| **stack**         |                        |                                     |                                                                                                                                                                     |
| `cal`             | required               | "**cal**l routine"                  | Push the current program counter onto the stack, increase the stack counter and jump to the specified program position.                                             |
| `ret`             | none                   | "**ret**urn"                        | Decrease the stack counter and jump to the program position identified by the stack value.                                                                          |
| `psh`             | none                   | "**p**u**sh** stack"                | Push the value of register `A` onto the stack and increase the stack counter.                                                                                       |
| `pop`             | none                   | "**pop** stack"                     | Decrease the stack counter and pop the stack value into register `A`.                                                                                               |
| `lsa`             | optional, default: `0` | "**l**oad **s**tack into **a**"     | Peek in the stack, possibly offset by the specified number of bytes, and load the value into register `A`.                                                          |
| `ssa`             | optional, default: `0` | "**s**tore **s**tack from **a**"    | Poke in the stack, possibly offset by the specified number of bytes, and store the value of register `A`.                                                           |
| `lsc`             | none                   | "**l**oad **s**tack **c**ounter"    | Load the value of the stack counter register `SC` into the value in register `A`.                                                                                   |
| `ssc`             | none                   | "**s**tore **s**tack **c**ounter"   | Store the value in register `A` to the stack counter register `SC`.                                                                                                 |
| **reg. `A`**      |                        |                                     |                                                                                                                                                                     |
| `mov`             | required               | "**mov**e immediately"              | Move the specified immediate value into register `A`.                                                                                                               |
| `not`             | none                   | "bitwise **not**"                   | Invert all bits in register `A`, modifying it in-place.                                                                                                             |
| `shl`             | optional, default: `1` | "**sh**ift **l**eft"                | Shift the bits in register `A` by the specified number of places to the left, modifying it in-place. Vacant bit positions are filled with zeros.                    |
| `shr`             | optional, default: `1` | "**sh**ift **r**ight"               | Shift the bits in register `A` by the specified number of players to the right, modifying it in-place. Vacant bit positions are filled with zeros.                  |
| `inc`             | optional, default: `1` | "**inc**rement"                     | Increment the value in register `A` by the specified value, modifying it in-place.                                                                                  |
| `dec`             | optional, default: `1` | "**dec**rement"                     | Decrement the value in register `A` by the specified value, modifying it in-place.                                                                                  |
| `neg`             | none                   | "numeric **neg**ate"                | Numerically negate the value in register `A`, modifying it in-place.                                                                                                |
| **reg. `A`, `B`** |                        |                                     |                                                                                                                                                                     |
| `swp`             | none                   | "**sw**a**p** register values"      | Swap the value of register `A` with the value of register `B`.                                                                                                      |
| `and`             | none                   | "bitwise **and**"                   | Perform a bit-wise `and` operation on the value register `A`, using the value of register `B` as a mask, modifying register `A` in-place.                           |
| `or`              | none                   | "bitwise **or**"                    | Perform a bit-wise `or` operation on the value register `A`, using the value of register `B` as a mask, modifying register `A` in-place.                            |
| `xor`             | none                   | "bitwise **xor**"                   | Perform a bit-wise `xor` operation on the value register `A`, using the value of register `B` as a mask, modifying register `A` in-place.                           |
| `add`             | none                   | "numeric **add**"                   | Add the value of register `B` to the value of register `A`, modifying register `A` in-place.                                                                        |
| `sub`             | none                   | "numeric **sub**tract"              | Subtract the value of register `B` from the value of register `A`, modifying register `A` in-place.                                                                 |
| **i/o**           |                        |                                     |                                                                                                                                                                     |
| `get`             | none                   | "**get** number"                    | Input a numerical value from `stdin` to register `A`.                                                                                                               |
| `gtc`             | none                   | "**g**e**t** **c**haracter"         | Input a `utf-8` encoded character from `stdin` and store the Unicode code point to register `A`.                                                                    |
| `ptu`             | none                   | "**p**u**t** **u**nsigned"          | Output the unsigned numerical value of register `A` to `stdout`.                                                                                                    |
| `pts`             | none                   | "**p**u**t** **s**igned"            | Output the signed numerical value of register `A` to `stdout`.                                                                                                      |
| `ptb`             | none                   | "**p**u**t** **b**its"              | Output the bits of register `A` to `stdout`.                                                                                                                        |
| `ptc`             | none                   | "**p**u**t** **u**nsigned"          | Output the Unicode code point in register `A` to `stdout`, encoded as `utf-8`.                                                                                      |
| **rnd**           |                        |                                     |                                                                                                                                                                     |
| `rnd`             | none                   | "pseudo-**r**a**nd**om number"      | Call the value in register `A` `r`. Set `A` to a discretely uniformly distributed pseudo-random number in the range `[0..r]` (inclusive on both ends).              |
| **hlt**           |                        |                                     |                                                                                                                                                                     |
| `hlt`             | none                   | "**h**a**lt**"                      | Halt the machine.                                                                                                                                                   |

# Example Programs
Automatic tests can be performed by using `make test`, which tests programs in `test/programs`.
