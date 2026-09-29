# Ternet Implementation Queue

This is the authoritative engineering checklist. A feature is complete only after source support, positive tests, negative tests, documentation, and a passing build/CI.

## Core language
- [ ] static type checker and inference
- [ ] structs, enums, classes, constructors and methods
- [ ] traits/interfaces and polymorphism
- [ ] generics and constraints
- [ ] Option/Result
- [ ] ownership/borrowing/move semantics
- [ ] decorators/attributes
- [ ] advanced control-flow and pattern matching

## Runtime/compiler
- [ ] complete bytecode compiler
- [ ] bytecode verifier
- [ ] optimization pipeline
- [ ] native backend
- [ ] WASM backend
- [ ] async/await scheduler
- [ ] deterministic runtime/concurrency tests

## Modules/packages
- [ ] robust import/module resolution
- [ ] dependency graph and version solving
- [ ] remote registry client
- [ ] package cache
- [ ] checksums/signatures/integrity verification
- [ ] package publishing

## Standard library
- [ ] collections
- [ ] filesystem/path/io
- [ ] JSON/TOML
- [ ] process/environment
- [ ] networking/HTTP/TLS
- [ ] crypto
- [ ] async primitives/channels
- [ ] logging/testing/benchmark APIs

## Developer experience
- [ ] formatter
- [ ] linter
- [ ] REPL
- [ ] LSP diagnostics/completion/navigation
- [ ] debugger/DAP
- [ ] profiler
- [ ] complete VS Code integration
- [ ] syntax highlighting for the frozen keyword set

## Security and production
- [ ] capability model
- [ ] sandboxed OS access
- [ ] resource limits
- [ ] security regression tests
- [ ] cross-platform CI and release artifacts
- [ ] documentation website and playground

## v1.0 gate
Every item required by the language specification must be implemented and tested before the v1.0 tag. Do not mark unfinished features as implemented merely because syntax or documentation exists.
