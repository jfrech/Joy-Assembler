# Joy Assembler
A minimalistic toy assembler written in C++ by Jonathan Frech, August 2020.

# Building
Joy Assembler requires the `C++20` standard and is best build using the provided `Makefile`.

# Usage
Joy Assembler provides a basic command-line interface:
````
./joy-assembler <input-file.asm> [visualize | step]
````
The optional argument `visualize` allows one to see each instruction's execution, `step` allows to see and step thru (by hitting `enter`) execution. Note that the instruction pointed to is the instruction that _will be executed_ in the next step, not the instruction that has been executed.

# Example Programs
Example Joy Assembler programs can be found in the `asm` directory.

# Architecture
Joy Assembler mimics a 32-bit architecture. It has two registers `A` and `B` which each hold a `int32_t` as well as a definable (ref. `pragma_memory-size`) block of memory, addressable as individual bytes. Each Joy Assembler instruction consists of a _name_ paired with four bytes representing an _argument_, which are interpreted or discarded as specified below.

# Instructions
| instruction name | argument                      | description                                                        |
|------------------|-------------------------------|--------------------------------------------------------------------|
| `nop`            | -                             | No operation.                                                      |
| `lda`            | memory location               | Load the value at the specified memory location into register `A`. |
| `ldb`            | memory location               | Load the value at the specified memory location into register `B`. |
| `sta`            | memory location               | Store the value in register `A` to the specified memory location.  |
| `stb`            | memory location               | Store the value in register `B` to the specified memory location.  |
| `jmp`            | program position `@label`     | Jump to program position `@label`.                                 |
| `jnz`            | program position `@label`     | Jump to program position `@label` if register `A` holds a non-zero value, otherwise perform no operation.     |
| `jz`             | program position `@label`     | Jump to program position `@label` if register `A` holds a zero value, otherwise perform no operation.         |
| `jnn`            | program position `@label`     | Jump to program position `@label` if register `A` holds a non-negative value, otherwise perform no operation. |
| `jng`            | program position `@label`     | Jump to program position `@label` if register `A` holds a negative value, otherwise perform no operation.     |
| `mov`            | value `v`                     | Move the immediate value `v` into register `A`.                    |
| `inc`            | -                             | Increment the value in register `A`, modifying it in-place.        |
| `dec`            | -                             | Decrement the value in register `A`, modifying it in-place.        |
| `inv`            | -                             | Invert all bits in register `A`, modifying it in-place.            |
| `shl`            | number of places to shift `n` | Shift the bits in register `A` by `n` places to the left, modifying it in-place. If `n` is not specified or zero, a shift by one place is performed.  |
| `shr`            | number of places to shift `n` | Shift the bits in register `A` by `n` places to the right, modifying it in-place. If `n` is not specified or zero, a shift by one place is performed. |
| `swp`            | -                             | Swap the value of register `A` with the value of register `B`.     |
| `add`            | -                             | Add the value of register `B` to the value of register `A`, modifying register `A` in-place.        |
| `sub`            | -                             | Subtract the value of register `B` from the value of register `A`, modifying register `A` in-place. |
| `and`            | -                             | Perform a bit-wise `and` operation on the value register `A`, using the value of register `B` as a mask, modifying register `A` in-place. |
| `or`             | -                             | Perform a bit-wise `or` operation on the value register `A`, using the value of register `B` as a mask, modifying register `A` in-place.  |
| `put`            | -                             | Output the numerical value of register `A` to `stdout`.            |

# Definitions
To define a constant value, one can use the _definition operator_ ` := `.
## Example
To statically lay out memory or define other constants, a header like the following might be appropriate.
````
addr_value-x := 0
addr_value-y := 4
addr_value-z := 8

value := 0xfc
````

# Labels
One can either hard-code program positions to jump to or use a special kind of definition called _program labels_. A label is defined by an identifier succeeded by a colon (`:`) and accessed by the same identifier preceded by an at sign (`@`). A label definition has to be unique, yet can be used arbitrarily often.
## Example
The following Joy Assembler code snippet calculates the number of set bits in register `A` and stores this number in register `A`, using the four bytes memory location `addr_temporary-value-a` and the four bytes at memory location `addr_temporary-value-b` as a temporary value storage.
````
count-bits:
    sta addr_temporary-value-a
    imm 0
    sta addr_temporary-value-b

    count-bits-loop:
        imm 1
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
````

# Pragmas
Pragmas are used to alter the simulated machine's fundamental behavior or capability.
| pragma               | options                         | description                                                       | default                               |
|----------------------|---------------------------------|-------------------------------------------------------------------|---------------------------------------|
| `pragma_memory-mode` | `little-endian` or `big-endian` | Set the endianness for the operations `lda`, `ldb`, `sta`, `stb`. | `pragma_memory-mode := little-endian` |
| `pragma_memory-size` | a `uint32_t` value              | Define the size of the available memory block.                    | `pragma_memory-mode := 0x10000`       |
