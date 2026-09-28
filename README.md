# Ternet

Ternet is an experimental programming language implemented in C++17.

> **Truthful status:** the repository currently contains a small working compiler/TVM baseline. The complete V1 language is specified in `docs/` and is being implemented incrementally. V1 features are not advertised as working until code and tests exist.

## Current working baseline

- `.trn` source files
- lexer with line/column diagnostics
- `tnprint("...")::`
- multiple print statements
- escaped strings
- AST
- bytecode compiler
- TVM execution
- CMake build
- `tnc <file.trn>` CLI
- examples and negative tests

## V1 language direction

Ternet V1 is designed around a distinctive syntax:

```trn
if {0 == 0};
    tnprint("hello world"):
else {};
    tnprint("hello"):
```

The V1 design also targets static typing, inference, ownership/borrowing, traits, generics, pattern matching, async/await, Result/Option, modules, a standard library, package tooling, bytecode verification and a future native backend.

## Documentation

- [V1 specification](docs/V1_SPEC.md)
- [Syntax reference](docs/SYNTAX.md)
- [Type system](docs/TYPES.md)
- [Memory and ownership](docs/MEMORY.md)
- [Bytecode and TVM](docs/BYTECODE.md)
- [Toolchain](docs/TOOLING.md)
- [Standard library](docs/STDLIB.md)
- [Grammar](docs/GRAMMAR.md)
- [Roadmap](docs/ROADMAP.md)
- [Implementation status](docs/STATUS.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Implemented language baseline](docs/LANGUAGE.md)

## Build

```bash
cmake -S . -B build
cmake --build build
```

Run:

```bash
./build/tnc examples/hello.trn
```

## Current language example

```trn
tnprint("Hello, world")::
tnprint("Ternet")::
```

The `::` syntax above is the current implemented baseline. V1's single-colon syntax is specified but not yet implemented.

## Project principles

1. No fake compiler features.
2. No placeholder runtime behavior presented as production functionality.
3. Every implemented feature gets positive and negative tests.
4. Documentation must match the actual compiler.
5. Language changes require explicit compatibility decisions.
