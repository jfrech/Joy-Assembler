# Joy Assembler
A minimalistic toy assembler written in C++ by Jonathan Frech, August 2020.

# Building
Joy Assembler requires the `C++20` standard and is best build using the provided `Makefile`.

# Usage
Joy Assembler provides a basic command-line interface:
````
./joy-assembler <input-file.asm> [visualize | step | cycles]
````
The optional argument `visualize` allows one to see each instruction's execution, `step` allows to see and step thru (by hitting `enter`) execution. Note that the instruction pointed to is the instruction that _will be executed_ in the next step, not the instruction that has been executed. `cycles` prints the number of execution cycles that were performed.

# Example Programs
Example Joy Assembler programs can be found in the `asm` directory.

# Architecture
Joy Assembler mimics a 32-bit architecture. It has two general-purpose registers `A` and `B` which each hold a (signed) `int32_t`, one program counter register `PC` and a stack counter register `SC` as well as a definable (ref. `pragma_memory-size`) block of memory, addressable as individual bytes. Each Joy Assembler instruction consists of a _name_ paired with four bytes representing an _argument_, which are interpreted or discarded as specified below.

# Instructions
| instruction name | argument                               | mnemonic                            | description                                                        |
|------------------|----------------------------------------|-------------------------------------|--------------------------------------------------------------------|
| `nop`            | -                                      | "**n**o o**p**"                     | No operation.                                                      |
|                  |                                        |                                     |                                                                    |
| `lda`            | memory location                        | "**l**oa**d** **a**"                | Load the value at the specified memory location into register `A`. |
| `ldb`            | memory location                        | "**l**oa**d** **b**"                | Load the value at the specified memory location into register `B`. |
| `sta`            | memory location                        | "**st**ore **a**"                   | Store the value in register `A` to the specified memory location.  |
| `stb`            | memory location                        | "**st**ore **b**"                   | Store the value in register `B` to the specified memory location.  |
| `lia`            | optional offset `o`                    | "**l**oad **i**ndirect **a**"       | Load the value at the memory location identified by register `B`, offset by `o`, into register `A`. If `o` is not specified, an offset of zero is assumed. |
| `sia`            | optional offset `o`                    | "**s**tore **i**ndirect **a**"      | Store the value in register `A` at the memory location identified by register `B`, offset by `o`. If `o` is not specified, an offset of zero is assumed.   |
| `lpc`            | -                                      | "**l**oad **p**rogram **c**ounter"  | Set the value of the program counter register `PC` to the value in register `A`. |
| `spc`            | -                                      | "**s**tore **p**rogram **c**ounter" | Store the value in the program counter register `PC` to register `A`.            |
| `lya`            | memory location                        | "**l**oad b**y**te **a**"           | Load the byte at the specified memory location into the least significant byte of register `A`, modifying it in-place and keeping the upper three bytes. |
| `sya`            | memory location                        | "**s**tore b**y**te **a**"          | Store the least significant byte of register `A` to the specified memory location.                                                                       |
|                  |                                        |                                     |                                                                    |
| `jmp`            | program position `@label`              | "**j**u**mp**"                      | Jump to program position `@label`.                                 |
| `jz`             | program position `@label`              | "**j**ump **z**ero"                 | Jump to program position `@label` if register `A` holds a zero value, otherwise perform no operation.         |
| `jnz`            | program position `@label`              | "**j**ump **n**on-**z**ero"         | Jump to program position `@label` if register `A` holds a non-zero value, otherwise perform no operation.     |
| `jn`             | program position `@label`              | "**j**ump **n**egative"             | Jump to program position `@label` if register `A` holds a negative value, otherwise perform no operation.     |
| `jnn`            | program position `@label`              | "**j**ump **n**on-**n**egative"     | Jump to program position `@label` if register `A` holds a non-negative value, otherwise perform no operation. |
| `je`             | program position `@label`              | "**j**ump **e**ven"                 | Jump to program position `@label` if register `A` holds an even value.                                        |
| `jne`            | program position `@label`              | "**j**ump **n**on-**e**ven"         | Jump to program position `@label` if register `A` holds an odd value.                                         |
|                  |                                        |                                     |                                                                    |
| `cal`            | program position `@label`              | "**cal**l routine"                  | Push the current program counter onto the stack, increase the stack counter and jump to program position `@label`.            |
| `ret`            | -                                      | "**ret**urn"                        | Decrease the stack counter and jump to the program position identified by the stack value.                                    |
| `psh`            | -                                      | "**p**u**sh** stack"                | Push the value of register `A` onto the stack and increase the stack counter.                                                 |
| `pop`            | -                                      | "**pop** stack"                     | Decrease the stack counter and pop the stack value into register `A`.                                                         |
| `lsa`            | optional offset `o`                    | "**l**oad **s**tack into **a**"     | Peek in the stack with offset `o` and load the value into register `A`. If no offset is specified, an offset of zero is used. |
| `ssa`            | optional offset `o`                    | "**s**tore **s**tack from **a**"    | Poke in the stack with offset `o` and store the value of register `A`. If no offset is specified, an offset of zero is used.  |
| `lsc`            | -                                      | "**l**oad **s**tack **c**ounter"    | Load the value in register `A` into the current stack counter.                                                                |
| `ssc`            | -                                      | "**s**tore **s**tack **c**ounter"   | Store the current stack counter into register `A`.                                                                            |
|                  |                                        |                                     |                                                                    |
| `mov`            | value `v`                              | "**mov**e immediate"                | Move the immediate value `v` into register `A`.                    |
| `not`            | -                                      | "bitwise **not**"                   | Invert all bits in register `A`, modifying it in-place.            |
| `shl`            | optional number of places to shift `n` | "**sh**ift **l**eft"                | Shift the bits in register `A` by `n` places to the left, modifying it in-place. If `n` is not specified, a shift by one place is performed.  |
| `shr`            | optional number of places to shift `n` | "**sh**ift **r**ight"               | Shift the bits in register `A` by `n` places to the right, modifying it in-place. If `n` is not specified, a shift by one place is performed. |
| `inc`            | optional increment value `v`           | "**inc**rement"                     | Increment the value in register `A` by `v`, modifying it in-place. If `v` is not specified, an increment of one is performed.                 |
| `dec`            | optional increment value `v`           | "**dec**rement"                     | Decrement the value in register `A` by `v`, modifying it in-place. If `v` is not specified, a decrement of one is performed.                  |
| `neg`            | -                                      | "numeric **neg**ate"                | Numerically negate the value in register `A`, modifying it in-place. |
|                  |                                        |                                     |                                                                    |
| `swp`            | -                                      | "**sw**a**p** register values"      | Swap the value of register `A` with the value of register `B`.     |
| `and`            | -                                      | "bitwise **and**"                   | Perform a bit-wise `and` operation on the value register `A`, using the value of register `B` as a mask, modifying register `A` in-place. |
| `or`             | -                                      | "bitwise **or**"                    | Perform a bit-wise `or` operation on the value register `A`, using the value of register `B` as a mask, modifying register `A` in-place.  |
| `xor`            | -                                      | "bitwise **xor**"                   | Perform a bit-wise `xor` operation on the value register `A`, using the value of register `B` as a mask, modifying register `A` in-place. |
| `add`            | -                                      | "numeric **add**"                   | Add the value of register `B` to the value of register `A`, modifying register `A` in-place.        |
| `sub`            | -                                      | "numeric **sub**tract"              | Subtract the value of register `B` from the value of register `A`, modifying register `A` in-place. |
|                  |                                        |                                     |                                                                    |
| `ptu`            | -                                      | "**p**u**t** **u**nsigned"          | Output the unsigned numerical value of register `A` to `stdout`.   |
| `gtu`            | -                                      | "**g**e**t** **u**nsigned"          | Input an unsigned numerical value from `stdin` to register `A`.    |
| `pts`            | -                                      | "**p**u**t** **s**igned"            | Output the signed numerical value of register `A` to `stdout`.     |
| `gts`            | -                                      | "**g**e**t** **s**igned"            | Input an signed numerical value from `stdin` to register `A`.      |
| `ptb`            | -                                      | "**p**u**t** **b**its"              | Output the bits of register `A` to `stdout`.                       |
| `gtb`            | -                                      | "**g**e**t** **b**its"              | Input bits (represented as characters) `stdin` to register `A`.    |
| `ptc`            | -                                      | "**p**u**t** **u**nsigned"          | Output the Unicode code point in register `A` to `stdout`, encoded as `utf-8`.                   |
| `gtc`            | -                                      | "**g**e**t** **c**haracter          | Input a `utf-8` encoded character from `stdin` and store the Unicode code point to register `A`. |
|                  |                                        |                                     |                                                                    |
| `hlt`            | -                                      | "**h**a**lt**"                      | Halt the machine.                                                  |

# Definitions
To define a constant value, one can use the _definition operator_ ` := `.
## Example
To statically lay out memory or define other constants, a header like the following might be appropriate. `prg+` statically adds the program offset, that is the number of instructions times five (since each instruction is represented using five bytes).
````
addr_value-x := prg+0
addr_value-y := prg+4
addr_value-z := prg+8

value := 0xfc
````

# Data Blocks
A static _data block_ can be defined using `uint[size] default-value`:
````
global-x:
    uint[1] 0
global-y:
    uint[1] 0
stack:
    uint[0xffff] 0
````

# Labels
One can either hard-code program positions to jump to or use a special kind of definition called _program labels_. A label is defined by an identifier succeeded by a colon (`:`) and accessed by the same identifier preceded by an at sign (`@`). A label definition has to be unique, yet can be used arbitrarily often.
## Example
The following Joy Assembler code snippet calculates the number of set bits in register `A` and stores this number in register `A`, using the four bytes memory location `addr_temporary-value-a` and the four bytes at memory location `addr_temporary-value-b` as a temporary value storage.
````
count-bits:
    sta addr_temporary-value-a
    mov 0
    sta addr_temporary-value-b

    count-bits-loop:
        mov 1
        swp
        lda addr_temporary-value-a
        and
        swp

        lda addr_temporary-value-a
        shr
        sta addr_temporary-value-a

        swp
        jz @not-set
            lda addr_temporary-value-b
            inc
            sta addr_temporary-value-b
        not-set:

        lda addr_temporary-value-a
        jnz @count-bits-loop

    lda addr_temporary-value-b

    jmp @back
````

# Pragmas
Pragmas are used to alter the simulated machine's fundamental behavior or capability.
| pragma               | options                         | description                                                       | default                               |
|----------------------|---------------------------------|-------------------------------------------------------------------|---------------------------------------|
| `pragma_memory-mode` | `little-endian` or `big-endian` | Set the endianness for the operations `lda`, `ldb`, `sta`, `stb`. | `pragma_memory-mode := little-endian` |
| `pragma_memory-size` | a `uint32_t` value              | Define the size of the available memory block.                    | `pragma_memory-mode := 0x10000`       |
