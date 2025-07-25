# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

QuickJS is a small and embeddable JavaScript engine that aims to support the latest ECMAScript specification. This is a fork of the original QuickJS project by Fabrice Bellard and Charlie Gordon, maintained by Ben Noordhuis and Saúl Ibarra Corretgé.

## Build System and Common Commands

QuickJS uses CMake as its primary build system with a helper Makefile for convenience:

### Build Commands
- `make` - Build everything (qjs, qjsc executables and test tools)
- `make debug` - Build debug version without optimizations  
- `make clean` - Clean build artifacts
- `make distclean` - Remove entire build directory
- `make install` - Install binaries and headers

### Testing Commands
- `make test` - Run the built-in test suite using tests.conf
- `make test262` - Run the full test262 ECMAScript conformance suite
- `make test262-fast` - Run a faster subset of test262 tests
- `make test262-update` - Run test262 and update the error/pass report
- `make microbench` - Run performance microbenchmarks
- `make ctest` - Run C compilation syntax checks
- `make cxxtest` - Run C++ compilation syntax checks

### Development Tools
- `make stats` - Show memory statistics (qjs -qd)
- `make fuzz` - Build and run fuzzer with sanitizers
- `make amalgam` - Create single-file amalgamated build
- `make codegen` - Generate C code from JavaScript modules using qjsc

### CMake Options
Key build options (can be set via environment variables):
- `BUILD_TYPE=Debug|Release` - Build configuration
- `QJS_BUILD_EXAMPLES=ON` - Build example programs
- `QJS_BUILD_CLI_STATIC=ON` - Build static qjs executable
- `QJS_ENABLE_ASAN=ON` - Enable AddressSanitizer
- `QJS_BUILD_LIBC=ON` - Include standard library in main library
- `QJS_ENABLE_JIT=ON` - Enable JIT compilation support (default: ON)

## Architecture

### Core Components
- **quickjs.c/quickjs.h** - Main JavaScript engine implementation
- **quickjs-libc.c/quickjs-libc.h** - Standard library modules (std, os, bjson)
- **qjs.c** - Interactive REPL and script runner
- **qjsc.c** - JavaScript to C bytecode compiler
- **run-test262.c** - Test262 ECMAScript conformance test runner

### JIT Compilation (Experimental)
- **quickjs-jit.h/quickjs-jit.c** - SLJIT-based JIT compilation support
- **quickjs-jit-opcodes.h** - Bytecode to machine code translation mappings
- **third-party/sljit/** - SLJIT library for cross-platform JIT compilation
- JIT compilation is enabled by default, triggered when functions are called frequently

### Key Libraries
- **libregexp.c/libregexp.h** - Regular expression engine
- **libunicode.c/libunicode.h** - Unicode support and tables
- **cutils.c/cutils.h** - Common C utilities
- **xsum.c/xsum.h** - Checksum utilities

### Generated Code
The `gen/` directory contains C code generated from JavaScript sources:
- `gen/repl.c` - REPL functionality compiled from repl.js
- `gen/standalone.c` - Standalone runner from standalone.js
- Generated example code (hello.c, test_fib.c, etc.)

### Test Structure
- `tests/` - Built-in test suite (JavaScript files)
- `test262/` - ECMAScript conformance test suite (git submodule)
- `tests.conf` - Configuration for built-in tests  
- `test262.conf` - Full test262 configuration
- `test262-fast.conf` - Faster test262 subset

### Examples
- `examples/` - Sample C programs using QuickJS API
- `examples/fib.c` - Fibonacci module example
- `examples/point.c` - Point object example  
- JavaScript module examples (hello.js, test_fib.js)

## Development Workflow

1. Make changes to core engine (quickjs.c) or standard library (quickjs-libc.c)
2. Run `make` to build
3. Test with `make test` for basic functionality
4. For ECMAScript compliance: `make test262-fast` or `make test262`
5. Generate updated code if needed: `make codegen`
6. Run syntax checks: `make ctest` and `make cxxtest`

## File Organization Notes

- Main engine code is in root directory
- Documentation website source is in `docs/`
- Tests are split between `tests/` (built-in) and `test262/` (conformance)
- Build outputs go to `build/` directory
- Unicode tables are generated, not manually edited

## Personal Memories

- me
- this is a new memory