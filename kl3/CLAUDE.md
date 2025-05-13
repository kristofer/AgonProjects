# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

Klispc is a LISP interpreter implementation. The project is targeting both native compilation for macOS (ARM) as well as the Agon platform (through agondev).
The codebase implements a small subset of LISP functionality, focusing on core LISP operations and simple arithmetic functions.

## Build Commands

```bash
# Compile for macOS ARM
cc -o kl3a src/kl3.c

# Run the LISP REPL (interactive mode)
./kl3a

# Run a specific LISP file
./kl3a filename.lisp

# Compile for Agon platform
make
```

## Project Structure

The project has the following key components:

1. **Core Interpreter** (`src/kl3.c`): A C implementation of a LISP interpreter with the following features:
   - Cell-based memory model (atoms, numbers, pairs)
   - Built-in functions: CONS, CAR, CDR, ATOM, EQ
   - Special forms: QUOTE, LAMBDA, COND, LABEL, DEFINE
   - Arithmetic functions: ADD, SUB, MUL, DIV, SQRT
   - REPL (Read-Eval-Print Loop) for interactive execution
   - File execution capabilities
   - Error handling with setjmp/longjmp
   - Basic memory management

2. **Test Files**: Several LISP files to test different aspects of the interpreter:
   - `tests.lisp`: General functionality tests
   - `simple_recursion.lisp`: Tests for basic function definition and execution
   - `factorial.lisp`: Recursive function implementation test
   - `number_tests.lisp`: Tests for numeric operations
   - `recursion_test.lisp`: Advanced recursion tests
   - `step_by_step.lisp`: Step-by-step execution examples

## Important Features

1. **Cell Types**:
   - CELL_ATOM: Represents atomic values (symbols)
   - CELL_NUMBER: Represents numeric values (integers)
   - CELL_PAIR: Represents cons cells (pairs)
   - CELL_FUNCTION: Represents built-in functions
   - CELL_SPECIAL: Represents special forms

2. **Error Handling**: The interpreter uses `setjmp`/`longjmp` for non-local exits upon error conditions.

3. **Environment**: The environment is implemented as an association list of symbol-value pairs.

4. **Evaluation**: The core of the interpreter is the `eval` function that implements the evaluation rules of LISP.

5. **Debug Mode**: The interpreter supports a debug mode (`DEBUG` function) that can be toggled from LISP code.

## Common Development Tasks

1. **Running the Interpreter**:
   ```bash
   # Compile and run in interactive mode
   cc -o kl3a src/kl3.c
   ./kl3a

   # Run a specific LISP file
   ./kl3a tests.lisp
   ```

2. **Testing**:
   ```bash
   # Run the built-in tests
   ./kl3a

   # Run specific test files
   ./kl3a tests.lisp
   ./kl3a factorial.lisp
   ```

3. **Deploying to Agon**:
   ```bash
   # Build for Agon
   make

   # Copy to Agon emulator's SD card (kinda sorta)
   cp tests.lisp ../../sdcard/tests.lisp
   cp bin/kl3.c.bin ../../sdcard/bin/klc.bin
   ```
