# LC4 Simulator & Command-Line Debugger

A C implementation of an LC4 instruction interpreter with an interactive debugger. Load a program, step through instructions, and inspect how registers, condition codes, and memory change.

**Author:** Lulin He  
**Stack:** C11, Make, Python 3 for CLI tests

## Features

- Eight 16-bit registers, a 16-bit PC, NZP condition codes, and 65,536 word-addressed memory locations.
- Instruction execution for arithmetic, logic, comparison, shifts, branches, calls, jumps, loads, stores, constants, TRAP, and RTI.
- Single-step execution and bounded execution with `run N`.
- Register and memory inspection with input validation.
- Signed immediate decoding, 16-bit wrapping, and conditional NZP updates.
- A standalone text-hex loader with comments and address markers.
- CPU regression checks and black-box debugger tests.

## Build and Run

Use a C11 compiler and Make on Linux, WSL, or another compatible environment. Python 3 is needed only for the CLI test suite.

```sh
make
./lc4_debugger examples/arithmetic.hex
```

An optional second argument sets the initial PC and the default load address. Addresses are hexadecimal:

```sh
./lc4_debugger examples/arithmetic.hex 0020
```

Override the compiler when needed:

```sh
make CC=gcc
```

## Demo

The included example computes **5 + 4 + 3 + 2 + 1 = 15**, stores the result at **x2000**, and reads it back into **R3**.

```sh
make
./lc4_debugger examples/sum.hex
```

At the `lc4>` prompt, enter:

```text
regs
mem 0000 10
step
step
step
step
step
regs
run 16
mem 2000 1
quit
```

After five steps, the first loop iteration has added 5 to R0. The next 16 instructions finish the loop, store the sum, and load it back. The final state includes:

```text
PC: x0009
R0: x000F
R1: x0000
R2: x2000
R3: x000F
x2000: x000F
```

`x000F` is hexadecimal for decimal 15. The final instruction is an idle loop, so this example pauses by instruction count rather than automatically halting.

<!-- When the video is ready, replace this comment with:
[Watch the demo](YOUR_VIDEO_URL)
Or add a clickable GIF:
[![LC4 simulator demo](assets/demo.gif)](YOUR_VIDEO_URL)
-->

## Debugger Commands

| Command | Behavior |
|---|---|
| `step` | Execute one instruction and display register state. |
| `run N` | Execute at most N instructions, then pause; N is decimal, from 1 to 1,000,000. |
| `regs` | Display PC, NZP, registers, halted state, and privilege state. |
| `mem ADDRESS N` | Display N memory words; ADDRESS is hexadecimal and N is decimal, from 1 to 256. |
| `help` | Display available commands. |
| `quit` | Exit the debugger. |

Inspection commands do not advance the PC. Execution errors return control to the debugger. End-of-input also exits the debugger.

## Program Format

The public build uses **text hexadecimal input**, rather than the course binary `.obj` format. Each nonempty line contains one word or one address marker:

```text
# Optional comment
@0000
9005 ; CONST R0, #5
9203 ; CONST R1, #3
1401 ; ADD R2, R0, R1
@2000
0000 ; A data word
```

- Words and addresses must be in the range `0000`–`FFFF`; `0x` prefixes are also accepted.
- `@ADDRESS` changes where subsequent words are loaded; it does not change the initial PC.
- `#` and `;` begin comments. Blank lines are ignored.
- Values occupy consecutive word addresses until another address marker appears.
- Later writes to the same address replace earlier values.
- Malformed input and attempts to load past `FFFF` are rejected. A failed load leaves CPU memory unchanged.
- `.hex` files can contain both instructions and data. They are not assembly source files.

## Implementation

| File | Responsibility |
|---|---|
| `lc4_cpu.c` / `lc4_cpu.h` | CPU state, instruction execution, NZP updates, and state inspection. |
| `lc4_hex.c` / `lc4_hex.h` | Standalone text-hex loading and validation. |
| `lc4_debugger.c` | Interactive command loop and bounded execution. |
| `examples/` | Original machine-code examples with assembly annotations. |
| `tests/` | CPU behavior checks and CLI integration tests. |

Each execution step fetches the word at PC, decodes its instruction fields, performs the operation, and commits the resulting PC. Register-writing instructions update NZP from the 16-bit result; comparisons set NZP directly, while stores and ordinary branches preserve it.

## Validation

```sh
make test
```

The C tests check every implemented instruction family, signed and unsigned comparisons, all branch masks, negative offsets, arithmetic wrapping, shifts, load/store behavior, calls and returns, basic TRAP/RTI transitions, error paths, and the sum example. Eight Python tests cover the command loop, bounded execution, hex loading, malformed files, and memory inspection boundaries.

These are focused regression tests, not exhaustive ISA verification or a differential comparison against PennSim. Diagnostics printed during the C error-path tests are expected.

```sh
make clean
```

## Current Scope

- This is an instruction-level interpreter, not a cycle-accurate processor model or a full PennSim replacement.
- NZP and PSR[15] are represented separately. Full PSR and memory protection are not implemented; code, data, and device regions are not enforced.
- There are no simulated keyboard, display, timer, or other memory-mapped devices.
- TRAP saves the return address and enters the target address in privileged mode. RTI returns through R7 and clears privilege. Programs must supply their own handler code; `TRAP x25` is not a built-in halt shortcut.
- Division and remainder use C signed-integer conventions. A zero divisor reports an execution error. Results are retained as 16-bit values.
- A step budget pauses execution; it does not set the CPU's halted flag. The public debugger currently has no command or device that sets that flag.
- The interpreter does not reject every reserved encoding within a supported opcode.
- This public build does not include an assembler, disassembler, binary `.obj` loader, or breakpoints.

## Project Background and Development

This project started with LC4 assembly and object-file coursework at the University of Pennsylvania, then expanded into an instruction interpreter and an interactive debugger. The coursework foundation and simulator extension are described below; their source code has different publication scopes.

### Coursework Foundation — Private Source

Using course-provided interfaces and starter files, I implemented:

- **Assembler:** conversion of arithmetic and logic assembly instructions into 16-bit machine code, with `.CODE`, `.DATA`, `.ADDR`, and `.FILL` handling and binary object-file output.
- **Object loader:** parsing of code, data, and symbol records into an address-sorted linked list that stores memory contents and labels.
- **Disassembler:** translation of machine words into assembly text, use of available labels for instruction targets, and assembly-file export.

These modules established the source-to-machine-code and machine-code-to-assembly workflow. The coursework source and starter files remain private and are not required to build this public repository. The original assembler supports a subset of LC4 instructions; it is not described as a complete assembler.

### Simulator Extension — Public Source

I extended the project with:

- A CPU state model with eight registers, PC, NZP, word-addressed memory, and basic privilege tracking.
- An instruction execution engine for arithmetic, logic, comparison, shifts, branches, calls, jumps, loads, stores, constants, and basic TRAP/RTI behavior.
- Interactive single stepping, bounded execution, register inspection, and memory inspection.
- A standalone text-hex input path so the public build can run without course modules.
- Original example programs and CPU/CLI regression tests, including a loop that computes and stores the sum of 1 through 5.

| Capability | Coursework foundation (private) | Simulator extension (public) |
|---|---|---|
| Assembly to machine code | Subset assembler and object-file output | Not included |
| Binary object loading | Code/data/symbol records and linked-list storage | Replaced by standalone text-hex input |
| Machine code to assembly | Disassembly and assembly export | Not included |
| Execute instructions | Not part of these M11/M12 modules | CPU interpreter |
| Inspect runtime state | Not part of these M11/M12 modules | Registers, NZP, PC, memory, and basic privilege state |
| Interactively control execution | Not part of these M11/M12 modules | `step` and bounded `run N` |

Instruction semantics are based on the [LC4 Instruction Set reference](https://acg.cis.upenn.edu/milom/cis371-Spring13/lab/LC4.pdf).
