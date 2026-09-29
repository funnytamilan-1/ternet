# Ternet v1.0 Implementation Plan

Ternet v1.0 is a stable programming-language release, not a README-only milestone.

## Compiler and language
- Lexer with complete token/source-location diagnostics
- Parser and AST with deterministic error recovery
- Name resolution and scopes
- Type checking and inference
- Functions, closures, structs, enums, classes and methods
- Traits/interfaces and implementations
- Generics and constraints
- Pattern matching
- Option/Result error model
- Modules and visibility
- Stable language specification

## Runtime and execution
- Stable bytecode/VM contract
- Runtime error model
- Async/await, tasks and scheduler
- Cancellation and channels
- Deterministic tests for concurrency primitives

## Standard library
- core, strings, collections, option/result
- fs, path, io, env, process
- json, toml, csv
- url, networking, HTTP, TLS
- crypto primitives exposed through safe APIs
- time, logging, testing and benchmarking

## Package ecosystem
- `ternet.toml` manifest
- lockfile with deterministic resolution
- dependency graph and version constraints
- local cache
- package registry client
- checksums/integrity verification
- package publishing

## Developer tools
- `tnc build/run/check/test/fmt/lint/doc/bench`
- REPL
- LSP
- formatter and diagnostics
- debugger integration
- generated API documentation
- VS Code extension
- documentation website
- browser playground

## Platform and distribution
- Linux
- Windows
- macOS
- WebAssembly target
- reproducible CI release builds
- install/distribution packages

## Quality gates
A feature is only considered implemented when it has source code, tests, documentation, and CI coverage. Experimental features must remain explicitly marked experimental until their compatibility contract is defined.

## Current implementation status

The repository currently has a working lexer/parser, interpreter/bytecode foundations, local module loading, package metadata commands, webfile support, CMake tests and VS Code language support. The docs site and editor experience are being developed alongside the compiler.

The following remain implementation work rather than claims of completion: full generics/traits, ownership/borrowing, production async runtime, complete standard library, remote package registry/solver, security enforcement, native backend, full LSP/DAP, and cross-platform release automation.

## v1.0 exit criteria
- language specification frozen
- compatibility policy documented
- regression suite green
- release artifacts for supported platforms
- package tooling tested end-to-end
- editor syntax/tooling tested against real `.trn` programs
- documentation site complete
- no README-only feature claims
