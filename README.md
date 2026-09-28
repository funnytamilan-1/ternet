# Ternet

Ternet is an experimental, statically structured, object-oriented programming language implemented in C++17.

## v0.2 development baseline

Implemented now:
- `.trn` source files
- lexer with line/column diagnostics
- `tnprint("...")::`
- compiler pipeline: source -> tokens -> AST -> bytecode -> TVM
- multiple print statements
- escaped strings
- CMake build
- CLI executable `tnc`

Planned v1.0 language surface:
- variables and primitive types
- expressions and operators
- arrays
- if/else and loops
- functions and return values
- classes, objects, constructors and inheritance
- exceptions
- modules/imports
- FileLib, NetLib, GitLib, AILib and GameLib
- package management
- bytecode verification and a richer TVM
- compiler and runtime tests

## Build

```bash
cmake -S . -B build
cmake --build build
```

Run:

```bash
./build/tnc examples/hello.trn
```

## Language example

```trn
tnprint("Hello, world")::
tnprint("Ternet")::
```

Ternet is under active development. Features are documented only after they are implemented.
