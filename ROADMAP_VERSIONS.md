# Ternet Release Roadmap

## v0.1 — Lexer + parser
- Stable token model and source locations
- Complete supported grammar and AST
- Syntax diagnostics
- Lexer/parser regression tests

## v0.2 — Core language
- Core expressions and statements
- Functions, control flow, collections
- Structs/enums/match
- Runtime and static-checking stability

## v0.3 — Modules + package manager
- Modules/namespaces/exports
- Separate compilation boundaries
- Manifest and lockfile
- Dependency resolution, registry, integrity

## v0.4 — Standard library
- Core, strings, collections, option/result
- FS/path/io, JSON/TOML/CSV
- Networking, HTTP/TLS/URL/WebSocket
- Async/sync/channels, crypto, process/env
- Testing, logging and benchmarking APIs

## v0.5 — Generics + traits
- Generic functions/types
- Type inference and constraints
- Traits/interfaces and implementations
- Method resolution and diagnostics

## v0.6 — Async/runtime
- async/await
- Tasks and scheduler/executor
- Cancellation and error propagation
- Synchronization and channels

## v0.7 — Cross-platform
- Windows/Linux/macOS toolchains
- Portable filesystem/process behavior
- Reproducible release builds and CI matrix

## v0.8 — Production tooling
- Formatter and linter
- REPL
- LSP and DAP debugger
- Documentation generation
- Benchmarking and publishing tools

## v0.9 — Release candidate
- Language/API freeze candidate
- Compatibility and security testing
- Performance benchmarks
- RC packages and release automation

## v1.0 — Stable Ternet
- Stable language specification
- Supported platform matrix
- Stable compiler/runtime/package APIs
- Release binaries and documentation
- Compatibility and security process

## Development rule
A feature is only marked implemented when the code, tests, documentation and CI coverage support the claim. No placeholder or README-only features.
