# Ternet

Ternet is an experimental programming language implemented in C++17.

> **Truthful status:** the repository currently contains a small working parser + reference VM baseline. The V1 language, secure capability model, standard-library architecture and Docker/OCI-style tooling are specified and are being implemented incrementally.

## Current working baseline
- `.trn` source files
- lexer with line/column diagnostics
- `tnprint("...")::`
- multiple print statements
- escaped strings
- AST
- reference VM execution
- CMake build
- `tnc <file.trn>` CLI
- examples and negative tests

## V1 direction
Ternet targets a general-purpose systems language with easy-to-read syntax, static typing, inference, ownership/borrowing, traits, generics, pattern matching, async/await, Result/Option, a bytecode VM and a future native backend.

Security is a first-class toolchain concern: capability-gated OS/network/process access, bytecode verification, dependency integrity, reproducible builds, resource limits and explicit unsafe boundaries.

The developer workflow is designed to scale from a small script to backend/services and containerized deployments through a unified `tnc` toolchain.

## Documentation
- [V1 specification](docs/V1_SPEC.md)
- [Syntax reference](docs/SYNTAX.md)
- [Type system](docs/TYPES.md)
- [Memory and ownership](docs/MEMORY.md)
- [Bytecode and TVM](docs/BYTECODE.md)
- [Toolchain](docs/TOOLING.md)
- [Toolchain design](docs/TOOLCHAIN_DESIGN.md)
- [Standard library](docs/STDLIB.md)
- [Standard library catalog](docs/STDLIB_CATALOG.md)
- [Security model](SECURITY.md)
- [Container tooling](docs/CONTAINER.md)
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

The examples above use the currently implemented syntax. V1 remains a separate target specification; unsupported V1 features are not advertised as implemented.

## Project principles
1. No fake compiler features.
2. No placeholder runtime behavior presented as production functionality.
3. Every implemented feature gets positive and negative tests.
4. Documentation must match the actual compiler.
5. Language changes require explicit compatibility decisions.
6. Security features are not considered implemented until enforcement and tests exist.