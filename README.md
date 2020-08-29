# Joy Assembler
A minimalistic toy assembler written in C++ by Jonathan Frech, August 2020.

# Building
Joy Assembler requires the `C++17` standard and is best build using the provided `Makefile`.

**Build: 🟩 passing** (2020-08-29T20:06:06Z)

# Usage
Joy Assembler provides a basic command-line interface:
````
./joy-assembler <input-file.asm> [visualize | step | cycles | memory-dump]
````
The optional argument `visualize` allows one to see each instruction's execution, `step` allows to see and step thru (by hitting `enter`) execution. Note that the instruction pointed to is the instruction that _will be executed_ in the next step, not the instruction that has been executed. `cycles` prints the number of execution cycles that were performed. `memory-dump` mocks any I/O and outputs a step-by-step memory dump to `stdout` whilst executing.

# Example Programs
Example Joy Assembler programs can be found in the `test/programs` directory. All can be automatically tested using `make test`.

# Architecture
Joy Assembler mimics a 32-bit architecture. It has four 32-bit registers: two general-prupose registers `A` (**a**ccumulation) and `B` (o**b**erand) and two special-prupose registers `PC` (**p**rogram **c**ounter) and `SC` (**s**tack **c**ounter). Regarding memory, a single block of memory of a fixed size is provided, addressable as either numeric 4-byte values or individual bytes. Each Joy Assembler instruction consists of a _name_ paired with possibly four bytes representing an _argument_. All instructions' behavior is specified below.

The simulated architecture can also be slightly modified using _pragmas_ (see below).

# Instructions
| instruction name          | argument                               | mnemonic                            | description                                                        |
|---------------------------|----------------------------------------|-------------------------------------|--------------------------------------------------------------------|
| **nop**                   |                                        |                                     |                                                                    |
| `nop`                     | -                                      | "**n**o o**p**eration"              | No operation.                                                      |
| **memory**                |                                        |                                     |                                                                    |
| `lda`                     | memory location                        | "**l**oa**d** **a**"                | Load the value at the specified memory location into register `A`. |
| `ldb`                     | memory location                        | "**l**oa**d** **b**"                | Load the value at the specified memory location into register `B`. |
| `sta`                     | memory location                        | "**st**ore **a**"                   | Store the value in register `A` to the specified memory location.  |
| `stb`                     | memory location                        | "**st**ore **b**"                   | Store the value in register `B` to the specified memory location.  |
| `lia`                     | optional offset `o`                    | "**l**oad **i**ndirect **a**"       | Load the value at the memory location identified by register `B`, offset by `o`, into register `A`. If `o` is not specified, an offset of zero is assumed. |
| `sia`                     | optional offset `o`                    | "**s**tore **i**ndirect **a**"      | Store the value in register `A` at the memory location identified by register `B`, offset by `o`. If `o` is not specified, an offset of zero is assumed.   |
| `lpc`                     | -                                      | "**l**oad **p**rogram **c**ounter"  | Set the value of the program counter register `PC` to the value in register `A`. |
| `spc`                     | -                                      | "**s**tore **p**rogram **c**ounter" | Store the value in the program counter register `PC` to register `A`.            |
| `lya`                     | memory location                        | "**l**oad b**y**te **a**"           | Load the byte at the specified memory location into the least significant byte of register `A`, modifying it in-place and keeping the upper three bytes. |
| `sya`                     | memory location                        | "**s**tore b**y**te **a**"          | Store the least significant byte of register `A` to the specified memory location.                                                                       |
| **jumps**                 |                                        |                                     |                                                                    |
| `jmp`                     | program position `@label`              | "**j**u**mp**"                      | Jump to program position `@label`.                                 |
| `jz`                      | program position `@label`              | "**j**ump **z**ero"                 | Jump to program position `@label` if register `A` holds a zero value, otherwise perform no operation.         |
| `jnz`                     | program position `@label`              | "**j**ump **n**on-**z**ero"         | Jump to program position `@label` if register `A` holds a non-zero value, otherwise perform no operation.     |
| `jn`                      | program position `@label`              | "**j**ump **n**egative"             | Jump to program position `@label` if register `A` holds a negative value, otherwise perform no operation.     |
| `jnn`                     | program position `@label`              | "**j**ump **n**on-**n**egative"     | Jump to program position `@label` if register `A` holds a non-negative value, otherwise perform no operation. |
| `je`                      | program position `@label`              | "**j**ump **e**ven"                 | Jump to program position `@label` if register `A` holds an even value.                                        |
| `jne`                     | program position `@label`              | "**j**ump **n**on-**e**ven"         | Jump to program position `@label` if register `A` holds an odd value.                                         |
| **stack**                 |                                        |                                     |                                                                    |
| `cal`                     | program position `@label`              | "**cal**l routine"                  | Push the current program counter onto the stack, increase the stack counter and jump to program position `@label`.            |
| `ret`                     | -                                      | "**ret**urn"                        | Decrease the stack counter and jump to the program position identified by the stack value.                                    |
| `psh`                     | -                                      | "**p**u**sh** stack"                | Push the value of register `A` onto the stack and increase the stack counter.                                                 |
| `pop`                     | -                                      | "**pop** stack"                     | Decrease the stack counter and pop the stack value into register `A`.                                                         |
| `lsa`                     | optional offset `o`                    | "**l**oad **s**tack into **a**"     | Peek in the stack with offset `o` and load the value into register `A`. If no offset is specified, an offset of zero is used. |
| `ssa`                     | optional offset `o`                    | "**s**tore **s**tack from **a**"    | Poke in the stack with offset `o` and store the value of register `A`. If no offset is specified, an offset of zero is used.  |
| `lsc`                     | -                                      | "**l**oad **s**tack **c**ounter"    | Load the value in register `A` into the current stack counter.                                                                |
| `ssc`                     | -                                      | "**s**tore **s**tack **c**ounter"   | Store the current stack counter into register `A`.                                                                            |
| **register `A`**          |                                        |                                     |                                                                    |
| `mov`                     | value `v`                              | "**mov**e immediate"                | Move the immediate value `v` into register `A`.                    |
| `not`                     | -                                      | "bitwise **not**"                   | Invert all bits in register `A`, modifying it in-place.            |
| `shl`                     | optional number of places to shift `n` | "**sh**ift **l**eft"                | Shift the bits in register `A` by `n` places to the left, modifying it in-place. If `n` is not specified, a shift by one place is performed.  |
| `shr`                     | optional number of places to shift `n` | "**sh**ift **r**ight"               | Shift the bits in register `A` by `n` places to the right, modifying it in-place. If `n` is not specified, a shift by one place is performed. |
| `inc`                     | optional increment value `v`           | "**inc**rement"                     | Increment the value in register `A` by `v`, modifying it in-place. If `v` is not specified, an increment of one is performed.                 |
| `dec`                     | optional increment value `v`           | "**dec**rement"                     | Decrement the value in register `A` by `v`, modifying it in-place. If `v` is not specified, a decrement of one is performed.                  |
| `neg`                     | -                                      | "numeric **neg**ate"                | Numerically negate the value in register `A`, modifying it in-place. |
| **registers `A` and `B`** |                                        |                                     |                                                                    |
| `swp`                     | -                                      | "**sw**a**p** register values"      | Swap the value of register `A` with the value of register `B`.     |
| `and`                     | -                                      | "bitwise **and**"                   | Perform a bit-wise `and` operation on the value register `A`, using the value of register `B` as a mask, modifying register `A` in-place. |
| `or`                      | -                                      | "bitwise **or**"                    | Perform a bit-wise `or` operation on the value register `A`, using the value of register `B` as a mask, modifying register `A` in-place.  |
| `xor`                     | -                                      | "bitwise **xor**"                   | Perform a bit-wise `xor` operation on the value register `A`, using the value of register `B` as a mask, modifying register `A` in-place. |
| `add`                     | -                                      | "numeric **add**"                   | Add the value of register `B` to the value of register `A`, modifying register `A` in-place.        |
| `sub`                     | -                                      | "numeric **sub**tract"              | Subtract the value of register `B` from the value of register `A`, modifying register `A` in-place. |
| **input and output**      |                                        |                                     |                                                                    |
| `get`                     | -                                      | "**get** number"                    | Input a numerical value from `stdin` to register `A`.                                            |
| `gtc`                     | -                                      | "**g**e**t** **c**haracter"         | Input a `utf-8` encoded character from `stdin` and store the Unicode code point to register `A`. |
| `ptu`                     | -                                      | "**p**u**t** **u**nsigned"          | Output the unsigned numerical value of register `A` to `stdout`.   |
| `pts`                     | -                                      | "**p**u**t** **s**igned"            | Output the signed numerical value of register `A` to `stdout`.     |
| `ptb`                     | -                                      | "**p**u**t** **b**its"              | Output the bits of register `A` to `stdout`.                       |
| `ptc`                     | -                                      | "**p**u**t** **u**nsigned"          | Output the Unicode code point in register `A` to `stdout`, encoded as `utf-8`.                   |
| **hlt**                   |                                        |                                     |                                                                    |
| `hlt`                     | -                                      | "**h**a**lt**"                      | Halt the machine.                                                  |

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
A static _data block_ can be defined using `data [optional-size]optional-value, [optional-size]optional-value, ...`:
````
primes:
    data 2, 3, 5, 7, 11, 13, 17 ; ... (finite memory)
global-x:
    data [1]0
global-y:
    data [1]0
buf:
    data [0xff]0xffffffff
stack:
    data [0xfff]
````

# Definitions
To define a constant value, one can use the _definition operator_ ` := `:
````
n := 5
````

# Pragmas
Pragmas are used to alter the simulated machine's fundamental behavior or capability.
| pragma               | options                         | description                                                       | default                               |
|----------------------|---------------------------------|-------------------------------------------------------------------|---------------------------------------|
| `pragma_memory-mode` | `little-endian` or `big-endian` | Set the endianness for the operations `lda`, `ldb`, `sta`, `stb`. | `pragma_memory-mode := little-endian` |
| `pragma_memory-size` | a `uint32_t` value              | Define the size of the available memory block.                    | `pragma_memory-mode := 0x10000`       |
