```Virtual Machine (VM) & Assembler

A lightweight custom Virtual Machine (vm) and bytecode assembler (vasm) implementation written in C11. This project features a stack-based runtime architecture, automated test runners, dynamic library support, and a dedicated test fixture generation pipeline.
📂 Project Structure
Plaintext

.
├── main.c # Core entry point for the VM execution
├── makefile # Build automation configuration
├── README.md # Project documentation
├── add.vbc # Sample compiled virtual bytecode
│
├── lib/
│ ├── mylib.c # Source for custom shared library functions
│ └── mylib.so # Compiled dynamic shared library (tracked)
│
├── src/
│ ├── vm.c / .h # Core Virtual Machine execution loop and state
│ ├── vasm.c / .h # Assembler logic parsing assembly text into bytecode
│ ├── serialize.c / .h # Binary serialization routines for bytecode (.vbc)
│ └── hashmap.c / .h # Internal utility hashmap implementation
│
└── tests/
├── test_phase1.c # Main suite for running system & validation tests
├── expected.txt # Oracle file containing expected test outputs
├── makeTestCaseFile.sh # Automation script for generating test artifacts
├── tasms/ # Source assembly test files (.tasm) & binaries (.vbc)
└── tests_vasm/ # Integration test files (.vasm)

🛠️ Build and Development

The project uses a structured makefile to handle compilation, dependencies, and testing routines.
Prerequisites

    Compiler: gcc (with support for C11 standard)

    Libraries: Standard dynamic linking library (-ldl) and math library (-lm)

Commands

Build the core VM executable:
Bash

make

This generates the native ./vm binary in the root directory.

Run the test suite:
Bash

make test

This builds a distinct test_runner binary (isolating test logic away from main.c's entry point) and executes validation passes against the files located inside ./tests/tests_vasm/ using the expected criteria in expected.txt.

Clean built artifacts:
Bash

make clean

Clears all compiled object files (.o) sitting across src/ or the root folder, alongside generated binary files (vm, test_runner).
🧪 Testing Architecture

This repository uses a two-stage testing verification approach:

    Dynamic Integration Testing: Managed via tests/test_phase1.c. It ingests your virtual assembly targets, executes them within the virtual runtime framework, and cross-analyzes output states against the definitions inside expected.txt.

    Fixture Generation: The included tests/makeTestCaseFile.sh script automates the generation and updates of binary bytecode templates located within tests/tasms/.

📝 Bytecode Generation & Syntax

The system operates on custom virtual bytecode files (.vbc). These can be generated manually by passing assembler strings (.tasm or .vasm) into the compiler or running the integrated automation tools.

Supported primitives include stack manipulations (push, pop, dup, swap), register interactions, type casting (ftoi, itof), memory block allocation bounds validation (alloc, dealloc), and control jumps (jmp, zjmp).
```
